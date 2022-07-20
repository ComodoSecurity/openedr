#include "pch.h"
#include "FltPort.h"

namespace cmd {
namespace win {

		////
		////
		////
		//void HandleTraits::cleanup(ResourceType& rsrc) noexcept
		//{
		//	if (!::CloseHandle(rsrc))
		//		error::win::WinApiError(SL, "Can't close handle").log();
		//}

		//
		//FltPort::FltPort
		//


		FltPort::FltPort(const std::wstring& _portName)
			: portName(_portName)
		{
		}

		//
		//FltPort::Send
		//

		HRESULT FltPort::Send(const Buffer& item, Buffer& answer)
		{
			HRESULT hr = S_FALSE;
			if (!m_hFltPort)
			{
				hr = ::FilterConnectCommunicationPort(portName.data(), 0, nullptr, 0, nullptr, &m_hFltPort);
				if (FAILED(hr))
					return hr;
			}
			
			if (!m_hFltPort)
				return HRESULT_FROM_WIN32(ERROR_PORT_UNREACHABLE);

			DWORD resultLength = 0, 
			requestLength = static_cast<DWORD>(item.size()),
			replyLength = static_cast<DWORD>(answer.size());
			
			LPVOID requestBuffer = static_cast<LPVOID>(const_cast<uint8_t*>(item.data())),
			replyBuffer = static_cast<LPVOID>(answer.data());

			hr = ::FilterSendMessage(m_hFltPort, requestBuffer, requestLength, replyBuffer, replyLength, &resultLength);
			return hr;
		}

		//
		//FltPort::~FltPort
		//

		FltPort::~FltPort()
		{
			m_hFltPort.reset();
		}
}
}