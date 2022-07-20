#include "pch.h"
#include "FltPortReceiver.h"

namespace cmd {
namespace win {

		//
		//
		//
		void HandleTraits::cleanup(ResourceType& rsrc) noexcept
		{
			if (!::CloseHandle(rsrc))
				error::win::WinApiError(SL, "Can't close handle").log();
		}

		//
		//FltReceiverPort::FltReceiverPort
		//

		FltPortReceiver::FltPortReceiver(const std::wstring& _portName, size_t _threadsCount, bool _fReplyMode, const std::string& _threadName)
			: portName(_portName)
			, threadsCount(_threadsCount)
			, fReplyMode(_fReplyMode)
			, threadName(_threadName)
		{
		}

		bool FltPortReceiver::resizeMessage(OverlappedContext* pOvlpCtx)
		{
			DWORD nDataSize = 0;
			DWORD err = ERROR_SUCCESS;
			if (!::GetOverlappedResult(m_pCompletionPort, LPOVERLAPPED(pOvlpCtx), &nDataSize, TRUE))
				err = GetLastError();
			if (err != ERROR_INSUFFICIENT_BUFFER)
			{
				error::win::WinApiError(SL, err, "Fail to get overlapped result").log();
				return false;
			}

			if (nDataSize < pOvlpCtx->nDataSize || nDataSize > c_nMaxMsgSize)
			{
				LOGWRN("Overlapped data is too large <" << nDataSize << ">");
				return false;
			}

			pOvlpCtx->nDataSize = nDataSize;
			pOvlpCtx->pData = reallocMem(pOvlpCtx->pData, pOvlpCtx->nDataSize);
			if (pOvlpCtx->pData == nullptr)
				error::BadAlloc(SL, "Can't allocate memory for driver port message").throwException();
			LOGLVL(Trace, "Message resized to <" << nDataSize << "> bytes");

			return true;			
		}

		void FltPortReceiver::pumpMessage(OverlappedContext* pOvlpCtx)
		{
			ZeroMemory(&pOvlpCtx->pOvlp, sizeof(OVERLAPPED));
			// Pump messages into queue of completion port.
			HRESULT hr = ::FilterGetMessage(m_pConnectionPort,
				PFILTER_MESSAGE_HEADER(pOvlpCtx->pData.get()), DWORD(pOvlpCtx->nDataSize), &pOvlpCtx->pOvlp);
			if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
				error::win::WinApiError(SL, hr, "FilterGetMessage failed").throwException();
		}

		void FltPortReceiver::replyMessage(PFILTER_MESSAGE_HEADER pMessage, NTSTATUS nStatus)
		{
			if (fReplyMode)
			{
				//  Reply the worker thread status to the filter
				//  This is important because the filter will also wait for the thread 
				//  in case that the thread is killed before telling filter 
				//  the parse is done or aborted.
				FILTER_REPLY_HEADER replyMsg = {};
				replyMsg.Status = nStatus;
				replyMsg.MessageId = pMessage->MessageId;
				HRESULT hr = ::FilterReplyMessage(m_pConnectionPort, &replyMsg, sizeof(replyMsg));
				if (FAILED(hr))
				{
					if (hr != ERROR_FLT_NO_WAITER_FOR_REPLY) // Driver exit with timeout
						error::win::WinApiError(SL, hr, "Failed to reply to the driver").throwException();
				}
			}
			else
				return;
		}

		void FltPortReceiver::replyDataMessage(PFILTER_MESSAGE_HEADER pMessage, ReplyBuffer& replyMessage, NTSTATUS nStatus)
		{
			if (!fReplyMode)
				return;

			replyMessage.header.MessageId = pMessage->MessageId;
			replyMessage.header.Status = nStatus;
			HRESULT hr = ::FilterReplyMessage(m_pConnectionPort, (FILTER_REPLY_HEADER*)&replyMessage, (DWORD)replyMessage.size);
			if (FAILED(hr))
			{
				if (hr != ERROR_FLT_NO_WAITER_FOR_REPLY) // Driver exit with timeout
					error::win::WinApiError(SL, hr, "Failed to reply to the driver").throwException();
			}
		}

		void FltPortReceiver::parseEventsThread()
		{
			// TODO: add thread guard here
			CMD_TRY
			{
				if (!::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST))
					error::win::WinApiError(SL, "Can't set thread priority").log();
				sys::setThreadName(threadName);
				parseEventsThreadInt();
			}
			CMD_PREPARE_CATCH
			catch (error::Exception& e)
			{
				e.log(SL, threadName + " fail to parse event");
			}
			catch (...)
			{
				error::RuntimeError(SL, "Unknown error").log();
			}
		}

		void FltPortReceiver::parseEventsThreadInt()
		{
			LOGLVL(Detailed, "Event's receiving thread is started");

			while (!m_fTerminate)
			{
#ifdef ENABLE_EVENT_TIMINGS
				using namespace std::chrono;
				auto t0 = steady_clock::now();
#endif		
				//  Get overlapped structure asynchronously, the overlapped structure 
				//  was previously pumped by FilterGetMessage(...)
				DWORD nOutSize = 0;
				ULONG_PTR key = {};
				LPOVERLAPPED pOvlp = nullptr;


				if (!::GetQueuedCompletionStatus(m_pCompletionPort, &nOutSize, &key, &pOvlp, INFINITE))
				{
					//  The completion port handle associated with it is closed 
					//  while the call is outstanding, the function returns FALSE, 
					//  *lpOverlapped will be NULL, and GetLastError will return ERROR_ABANDONED_WAIT_0
					auto ec = GetLastError();
					if (HRESULT_FROM_WIN32(ec) == E_HANDLE || // Completion port becomes unavailable
						ec == ERROR_ABANDONED_WAIT_0 ||	// Completion port was closed
						ec == ERROR_OPERATION_ABORTED) // Rised when CancelIoEx() called
						break;

					if (ec != ERROR_INSUFFICIENT_BUFFER)
						error::win::WinApiError(SL, ec, "Can't receive driver message").throwException();

					LOGLVL(Trace, "Message buffer too small. Try to resize.");
					auto pOvlpCtx = (OverlappedContext*)pOvlp;
					if (!resizeMessage(pOvlpCtx))
					{
						auto pMessage = PFILTER_MESSAGE_HEADER(pOvlpCtx->pData.get());
						replyMessage(pMessage, S_FALSE);
					}

					pumpMessage(pOvlpCtx);
					continue;
				}
#ifdef ENABLE_EVENT_TIMINGS
				auto t1 = steady_clock::now();
#endif

				//  Recover message structure from overlapped structure.
				auto pMessage = PFILTER_MESSAGE_HEADER(((OverlappedContext*)pOvlp)->pData.get());
				Byte* pData = (Byte*)pMessage + sizeof(FILTER_MESSAGE_HEADER);
				Size nDataSize = nOutSize;

				// Collect statistics
				{
					std::scoped_lock lock(m_mtxStatistic);
					m_nMsgCount++;
					m_nMsgSize = nDataSize / m_nMsgCount + (m_nMsgCount - 1) * m_nMsgSize / m_nMsgCount;
				}

				ReplyBuffer reply;
				reply.size = sizeof(reply);
				bool result = false;

#ifdef ENABLE_EVENT_TIMINGS
				std::pair<Size, Size> times;
				HandlerContext hctx{ pData, nDataSize, reply.buffer, sizeof(reply.buffer), times };
				result = handler(hctx);
				auto t2 = steady_clock::now();
#else			
				HandlerContext hctx{ pData, nDataSize, reply.buffer, sizeof(reply.buffer) };
				result = handler(hctx);
#endif
				if(hctx.fHasResponce)
					replyDataMessage(pMessage, reply, (result) ? S_OK : S_FALSE);
				else
					replyMessage(pMessage, (result) ? S_OK : S_FALSE);

				//  If finalized flag is set from main thread, then it would break the while loop.
				if (m_fTerminate)
					break;

				// After we process the message, pump an overlapped structure into completion port again.
				pumpMessage((OverlappedContext*)pOvlp);
#ifdef ENABLE_EVENT_TIMINGS
				auto t3 = steady_clock::now();
				milliseconds totalTime(duration_cast<milliseconds>(t3 - t0));
				milliseconds receiveTime(duration_cast<milliseconds>(t1 - t0));
				milliseconds parseTime(duration_cast<milliseconds>(t2 - t1));
				milliseconds pumpTime(duration_cast<milliseconds>(t3 - t2));
				LOGLVL(Debug, "Message statistic: total <" << totalTime.count() <<
					">, receive <" << receiveTime.count() << ">, parse <" << parseTime.count() <<
					"> [lbvs <" << times.first << ">, queue <" << times.second << ">], pump <" <<
					pumpTime.count() << ">, msg size <" << nDataSize << ">");
#endif
			}

			LOGLVL(Detailed, "Event's receiving thread is finished");
		}

		//
		//FltReceiverPort::Start
		//

		void FltPortReceiver::Start(Handler& _handler)
		{
			m_nMsgCount = 0;
			m_nMsgSize = 0;
			m_fTerminate = false;

			// Prepare the communication port.
			HRESULT hr = ::FilterConnectCommunicationPort(portName.data(), 0, nullptr, 0,
				nullptr, &m_pConnectionPort);
			if (FAILED(hr))
			{	// TODO: write new Error handler for HRESULT
				// https://blogs.msdn.microsoft.com/oldnewthing/20061103-07/?p=29133
				std::stringstream errorMessage;
				errorMessage  << "Can't open driver connection port<" << string::convertWCharToUtf8(portName) << ">";
				error::win::WinApiError(SL, hr, errorMessage.str().data()).throwException();			
			}

			// Create the IO completion port for asynchronous message passing. 
			m_pCompletionPort.reset(::CreateIoCompletionPort(m_pConnectionPort, nullptr, 0,
				DWORD(threadsCount)));
			if (!m_pCompletionPort)
				error::win::WinApiError(SL, "Can't open driver completion port").throwException();

			// Create listening threads.
			m_pOvlpCtxes.resize(threadsCount);
			for (auto& pCtx : m_pOvlpCtxes)
			{
				pCtx.nDataSize = c_nDefMsgSize;
				pCtx.pData = allocMem(pCtx.nDataSize);
				if (pCtx.pData == nullptr)
					error::BadAlloc(SL, "Can't allocate memory for port message").throwException();
				pumpMessage(&pCtx);
			}

			// Start all threads.
			if (!m_pThreadPool.empty())
				error::InvalidUsage(SL, "Threads pool is not empty").throwException();

			handler = _handler;

			for (Size i = 0; i < threadsCount; ++i)
				m_pThreadPool.push_back(std::thread([this]() { this->parseEventsThread(); }));
		}

		//
		//FltReceiverPort::Stop
		//

		void FltPortReceiver::Stop()
		{
			m_fTerminate = true;

			//  Wake up the listening thread if it is waiting for message 
			//  via GetQueuedCompletionStatus() 
			if (m_pConnectionPort)
				CancelIoEx(m_pConnectionPort, NULL);

			// Wait all threads termination
			for (auto& pThread : m_pThreadPool)
				if (pThread.joinable())
					pThread.join();
			m_pThreadPool.clear();

			m_pOvlpCtxes.clear();
			m_pConnectionPort.reset();
			m_pCompletionPort.reset();
		}

}
}