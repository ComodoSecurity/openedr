#pragma once
#include "pch.h"
#include <functional>
#include <fltuser.h>
//#incldue <string>
#include "common.h"
//#include "procmon.hpp"

namespace cmd {
namespace win {
	using Buffer = std::vector<uint8_t>;
	
	//// Specialization for windows HANDLE
	//struct HandleTraits
	//{
	//	using ResourceType = HANDLE;
	//	static inline const ResourceType c_defVal = nullptr;
	//	static void cleanup(ResourceType& rsrc) noexcept;
	//};
	//using ScopedHandle = UniqueResource<HandleTraits>;

	struct HandlerContext 
	{
		const Byte* pInData;
		Size nInDataSize;
		Byte* pOutData = nullptr;
		Size nOutDataSize = 0;
	};
	using Handler = std::function<bool(HandlerContext& hctx)>;

	class FltPort
	{
		cmd::edrpm::ScopedHandle m_hFltPort;
		std::wstring portName;
	public:
		FltPort(const std::wstring& portName);
		HRESULT Send(const Buffer& item, Buffer& answer);
		~FltPort();
	};
}
}