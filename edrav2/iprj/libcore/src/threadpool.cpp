//
// edrav2.libcore
// 
// Author: Denis Bogdanov (25.02.2019)
// Reviewer: Yury Podpruzhnikov (27.02.2019)
//
///
/// @file ThreadPool implementation
/// @addtogroup threadpool
/// @{
///
#include "pch_win.h"
#include <threadpool.hpp>
#include <system.hpp>
#include <command.hpp>

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "thrdpool"

namespace cmd {

///////////////////////////////////////////////////////////////////////
//
// ThreadPool::Timer::Impl
//
///////////////////////////////////////////////////////////////////////

class TimerContext : public ThreadPool::ITimerContext,
	public std::enable_shared_from_this<TimerContext>
{
	boost::asio::steady_timer m_timer;
	ThreadPool::TimerHandler m_fnHandler;
	ThreadPool::ImplPtr m_pPool;

	std::mutex m_mtxData;
	std::chrono::milliseconds m_tDuration;
	bool m_fStop = false;

public:

	//
	//
	//
	TimerContext(ThreadPool::ImplPtr pPool, boost::asio::io_context& ioCtx, 
		ThreadPool::TimerHandler fnHandler, Time nDuration) : m_pPool(pPool),
		m_timer(ioCtx), m_fnHandler(fnHandler), 
		m_tDuration(std::chrono::milliseconds(nDuration))
	{
	}
	
	//
	//
	//
	virtual ~TimerContext()
	{
		cancel();
	}

	//
	//
	//
	void callHandler(std::weak_ptr<TimerContext> pThis, const boost::system::error_code& ec)
	{
		if (ec == boost::asio::error::operation_aborted) return;
		auto pThisLock = pThis.lock();
		if (!pThisLock) return;

		{
			std::scoped_lock lock(m_mtxData);
			if (m_fStop) return;
		}

		bool fStop = !m_fnHandler();

		// Reshedule task
		{
			std::scoped_lock lock(m_mtxData);
			if (!m_fStop) m_fStop = fStop;
			if (m_fStop) return;
			m_timer.expires_after(m_tDuration);
		}
		
		m_timer.async_wait(std::bind(&TimerContext::callHandler, this, pThis,
			std::placeholders::_1));
	}

	//
	//
	//
	virtual Size cancel() override
	{
		{
			std::scoped_lock lock(m_mtxData);
			m_fStop = true;
			m_tDuration = std::chrono::milliseconds(1);
		}
		return m_timer.cancel();
	}

	//
	//
	//
	virtual void reschedule(Time nMsDuration = c_nMaxTime, bool fStartNow = false) override
	{
		std::scoped_lock lock(m_mtxData);
		{
			if (nMsDuration != c_nMaxTime)
				m_tDuration = std::chrono::milliseconds(nMsDuration);
			m_fStop = false;
			if (fStartNow)
				m_timer.expires_after(std::chrono::milliseconds(1));
			else
				m_timer.expires_after(m_tDuration);
		}
		m_timer.async_wait(std::bind(&TimerContext::callHandler, this, weak_from_this(),
			std::placeholders::_1));
	}

	//
	//
	//
	bool isActive() override
	{
		std::scoped_lock lock(m_mtxData);
		return !m_fStop;
	}
};

///////////////////////////////////////////////////////////////////////
//
// ThreadPool::Impl
//
///////////////////////////////////////////////////////////////////////

//
//
//
class ThreadPool::Impl
{
private:
	boost::asio::io_context m_ioCtx;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_ioWork;

	std::vector<std::thread> m_threads;
	std::mutex m_mtxThreads;
	std::string m_sName;

	std::atomic_bool m_fStop = false;
	std::atomic_bool m_fBreakPostedHandlers = false;
	Priority m_ePriority = Priority::Normal;

	//
	//
	//
	void startPoolThread()
	{
		// This deleter detaches thread from object marking it as finished
		LOGLVL(Debug, "Pool thread is started <" << m_sName << ">");

		sys::setThreadName(m_sName);

#ifdef PLATFORM_WIN
		if (m_ePriority == Priority::High)
		{
			if (!::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST))
				error::win::WinApiError(SL, FMT("Can't set thread priority <" << m_sName << ">")).log();
		}
		else if (m_ePriority == Priority::Low)
		{
			if (!::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST))
				error::win::WinApiError(SL, FMT("Can't set thread priority <" << m_sName << ">")).log();
		}
#else
	#error Please implement priority setting for current platform
#endif
		while (!m_fStop)
		{
			try
			{
				m_ioCtx.run();
				LOGLVL(Debug, "Pool thread is finished <" << m_sName << ">");
				deregisterThread(std::this_thread::get_id());
				return; // run() exited normally
			}
			catch (error::PoolThreadStopped& )
			{
				LOGLVL(Detailed, "Pool thread is redused <" << m_sName << ">");
				deregisterThread(std::this_thread::get_id());
				return;
			}
			catch (error::Exception& ex)
			{	
				ex.log(SL, FMT("Exception occured in a thread in the pool < " << m_sName << ">"));
			}
			catch (...)
			{
				error::RuntimeError().log(SL, FMT("Unknown exception occured in a thread in the pool <" << m_sName << ">"));
			}
		}
	}

	//
	//
	//
	void deregisterThread(std::thread::id id)
	{
		std::scoped_lock lock(m_mtxThreads);
		for (auto it = m_threads.begin(); it != m_threads.end(); ++it)
		{
			if (it->get_id() != id) continue;
			it->detach();
			m_threads.erase(it);
			return;
		}

		// We can have thread-id which is not found because we can remove it 
		// in the stop() and wait on it
	}
	
public:

	//
	//
	//
	Impl(const std::string& sName, Size nThreadsCount, Priority ePriority) :
		m_ePriority(ePriority),
		m_ioWork(boost::asio::make_work_guard(m_ioCtx)),
		m_sName(sName)
	{
		if (nThreadsCount > 0)
			addThreads(nThreadsCount);
	}

	//
	//
	//
	Size addThreads(Size nThreadsCount)
	{
		std::scoped_lock lock(m_mtxThreads);
		if (m_fStop)
			error::InvalidUsage(FMT("The pool is being stopped <" << m_sName << ">")).throwException();
		for (Size n = 0; n < nThreadsCount; ++n)
			m_threads.push_back(std::thread(&Impl::startPoolThread, this));
		return m_threads.size();
	}

	//
	//
	//
	void stop(bool fForce)
	{
		m_fStop = true;
		m_ioWork.reset(); // Allow run() to exit after finishing current work

		if (fForce)
			m_fBreakPostedHandlers = true;

		if (fForce && !m_ioCtx.stopped())
			m_ioCtx.stop();

		// Wait threads termination
		while (true)
		{
			std::thread currThread;
			{
				std::scoped_lock lock(m_mtxThreads);
				if (m_threads.empty()) break;
				currThread.swap(m_threads.back());
				m_threads.pop_back();
			}
			if (currThread.joinable())
				currThread.join();
		}
	}

	//
	//
	//
	void deleteThread()
	{
		if (getThreadsCount() == 0) return;
		boost::asio::post(m_ioCtx, []() { error::PoolThreadStopped().throwException(); });
	}

	//
	//
	//
	Size getThreadsCount()
	{
		std::scoped_lock lock(m_mtxThreads);
		return m_threads.size();
	}

	//
	//
	//
	Priority getPriority()
	{
		return m_ePriority;
	}

	//
	//
	//
	void run(Handler fnHandler)
	{
		// Add force stop check
		auto fnCheckingHandler = [fnHandler, this]()
		{
			if (this->m_fBreakPostedHandlers) return;
			fnHandler();
		};

		if (getThreadsCount() == 0) addThreads(1);
		boost::asio::post(m_ioCtx, fnCheckingHandler);
	}

	// 
	//
	//
	TimerContextPtr run(TimerHandler fnHandler, Time nDuration, bool fStartNow, ImplPtr pImpl)
	{
		if (getThreadsCount() == 0) addThreads(1);
		auto pCtx = std::make_shared<TimerContext>(pImpl, m_ioCtx, fnHandler, nDuration);
		pCtx->reschedule(c_nMaxTime, fStartNow);
		return pCtx;
	}
};

///////////////////////////////////////////////////////////////////////
//
// ThreadPool wrapper
//
///////////////////////////////////////////////////////////////////////

//
//
//
ThreadPool::ThreadPool(const std::string& sName, Size nThreadsCount, Priority ePriority)
{
	m_pImpl = std::make_shared<ThreadPool::Impl>(sName, nThreadsCount, ePriority);
}

//
//
//
ThreadPool::ThreadPool(ThreadPool&&) noexcept = default;

//
//
//
ThreadPool& ThreadPool::operator=(ThreadPool&&) noexcept = default;

//
//
//
ThreadPool::~ThreadPool()
{
	if (!m_pImpl) return;
	stop();
}

//
//
//
void ThreadPool::checkImplIsValid(SourceLocation sl)
{
	if (!m_pImpl) 
		error::InvalidUsage(sl, "Access to empty ThreadPool").throwException();
}

//
//
//
void ThreadPool::stop(bool fForce)
{
	checkImplIsValid(SL);
	m_pImpl->stop(fForce);
}

//
//
//
Size ThreadPool::addThreads(Size nThreadsCount)
{
	checkImplIsValid(SL);
	return m_pImpl->addThreads(nThreadsCount);
}

//
//
//
void ThreadPool::deleteThread()
{
	checkImplIsValid(SL);
	m_pImpl->deleteThread();
}

//
//
//
Size ThreadPool::getThreadsCount()
{
	checkImplIsValid(SL);
	return m_pImpl->getThreadsCount();
}

//
//
//
ThreadPool::Priority ThreadPool::getPriority()
{
	checkImplIsValid(SL);
	return m_pImpl->getPriority();
}

//
//
//
void ThreadPool::runHandler(ThreadPool::Handler fnHandler)
{
	checkImplIsValid(SL);
	m_pImpl->run(fnHandler);
}

//
//
//
ThreadPool::TimerContextPtr ThreadPool::runHandler(Time nDuration, bool fStartNow, TimerHandler fnHandler)
{
	checkImplIsValid(SL);
	return m_pImpl->run(fnHandler, nDuration, fStartNow, m_pImpl);
}

//
//
//
void ThreadPool::run(ObjPtr<ICommand> pCmd)
{
	runHandler([pCmd]() {(void)pCmd->execute(); });
}

//
//
//
ThreadPool::TimerContextPtr ThreadPool::runWithDelay(Time nDuration, ObjPtr<ICommand> pCmd)
{
	return runHandler(nDuration, false, [pCmd]() {return bool(pCmd->execute()); });
}

///////////////////////////////////////////////////////////////////////
//
// TaskContext wrapper
//
///////////////////////////////////////////////////////////////////////

class TaskContext : public ObjectBase<ClassId(0x67ED6013)>,
	public ITaskContext
{
	ThreadPool::TimerContextPtr m_pTimerContext;

public:
	virtual ~TaskContext() override {}

	//
	//
	//
	virtual Size cancel() override
	{
		return m_pTimerContext->cancel();
	}

	//
	//
	//
	virtual void reschedule(Time nMsDuration, bool fFirstNow = false) override
	{
		m_pTimerContext->reschedule(nMsDuration, fFirstNow);
	}

	//
	//
	//
	bool isActive() override
	{
		return m_pTimerContext->isActive();
	}

	//
	//
	//
	void setContext(ThreadPool::TimerContextPtr pTimerContext)
	{
		m_pTimerContext = pTimerContext;
	}
};

//
//
//
ObjPtr<ITaskContext> convertToTask(ThreadPool::TimerContextPtr pTimerContext)
{
	auto pCtx = createObject<TaskContext>();
	pCtx->setContext(pTimerContext);
	return pCtx;
}

} // namespace cmd 
/// @}
