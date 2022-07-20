//
// EDRAv2.libcore
// 
// Author: Denis Bogdanov (20.01.2019)
// Reviewer: ???
//
///
/// @file Console class implementation
///
/// @weakgroup io IO subsystem
/// @{
#include "pch.h"
#include "console.h"

namespace cmd {
namespace io {

//
//
//
void Console::write(const void* pBuffer, Size nSize)
{
	const Size c_nBufferSize = 0x1000;
	std::string sData;
	while (nSize > 0)
	{
		Size currSize = std::min(c_nBufferSize, nSize);
		sData.resize(currSize, 0);
		memcpy(&sData[0], pBuffer, currSize);
		std::cout << sData;
		pBuffer = static_cast<const Byte*>(pBuffer) + currSize;
		nSize -= currSize;
	}
}

//
//
//
void Console::flush()
{
	std::flush(std::cout);
}

//
//
//
Variant Console::execute(Variant vCommand, Variant vParams) noexcept(false)
{
	if (vCommand == "write")
	{
		std::string sData = vParams["data"];
		write(sData.data(), sData.size());
		if (vParams.get("eol", false))
			write("\n", 1);
		return {};
	}
	else if (vCommand == "put")
	{
		Variant vData = vParams["data"];
		put(vData);
		return {};
	}
	error::InvalidArgument(SL, FMT("Console doesn't support command <" << vCommand << ">")).throwException();
}

//
//
//
void Console::put(const Variant& vData)
{
	// @Bogdanov FIXME: Possible better use Variant::print()
	variant::serializeToJson(queryInterface<IRawWritableStream>(getPtrFromThis()), vData);
	write("\n", 1);
}

} // namespace io
} // namespace cmd

/// @}
