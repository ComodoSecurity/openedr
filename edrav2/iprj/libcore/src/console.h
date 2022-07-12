//
// EDRAv2.libcore
// 
// Author: Denis Bogdanov (20.01.2019)
// Reviewer: 
//
///
/// @file Console class declaration
///
/// @weakgroup io IO subsystem
/// @{
#pragma once
#include <common.hpp>
#include <command.hpp>
#include <io.hpp>
#include <objects.h>

namespace cmd {
namespace io {

///
/// Console input/output provider.
///
/// Console provides allows to do I/O operations using current attached console.
///
class Console : public ObjectBase<CLSID_Console>,
	public IRawWritableStream,
	public ICommandProcessor,
	public IDataReceiver
{
private:

public:

	// IRawWritableStream

	/// @copydoc IRawWritableStream::write()
	void write(const void* pBuffer, Size nSize) override;
	/// @copydoc IRawWritableStream::flush()
	void flush() override;

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	Variant execute(Variant vCommand, Variant vParams) override;

	// IDataReceiver

	/// @copydoc IDataReceiver::put()
	void put(const Variant& vData) override;
};

} // namespace io
} // namespace cmd

/// @}
