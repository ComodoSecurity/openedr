//
// edrav2.libcore project
//
// Author: Yury Ermakov (11.12.2018)
// Reviewer: Denis Bogdanov (13.02.2019)
//
///
/// @file Exceptions and errors declaration
///
/// @addtogroup errors Errors processing
/// Basic error and exceptions processing subsystem.
/// @{
#pragma once
#include "basic.hpp"
#include "string.hpp"

namespace openEdr {

// Variant forward declaration
namespace variant {
class Variant;
} // namespace variant

using variant::Variant;

namespace error {

///
/// Error codes.
///
/// New error code procedure includes the following steps:
///   * Add new item into enum class ErrorCode.
///   * Add the code's description into getErrorString().
///   * Add new typedef using correspondent template class.
///   * Add new case into factory function ExceptionBase::getRealException().
///
enum class ErrorCode : int32_t
{
	Undefined				= INT32_MIN,
	UserBreak				= int32_t(0x00000002),
	NoAction				= int32_t(0x00000001),
	OK						= int32_t(0x00000000),

	// Logic errors
	LogicError				= int32_t(0xE0010000),
	InvalidArgument			= int32_t(0xE0010001),
	InvalidUsage			= int32_t(0xE0010002),
	OutOfRange				= int32_t(0xE0010003),
	TypeError				= int32_t(0xE0010004),

	// Runtime errors
	RuntimeError			= int32_t(0xE0020000),
	AccessDenied			= int32_t(0xE0020001),
	ArithmeticError			= int32_t(0xE0020002),
	FileLocked				= int32_t(0xE0020003),
	NotFound				= int32_t(0xE0020004),
	InvalidFormat			= int32_t(0xE0020005),
	LimitExceeded			= int32_t(0xE0020006),
	NoData					= int32_t(0xE0020007),
	StringEncodingError		= int32_t(0xE0020008),
	OperationNotSupported	= int32_t(0xE0020009),
	ConnectionError			= int32_t(0xE002000A),
	BadAlloc				= int32_t(0xE002000B),
	PoolThreadStopped		= int32_t(0xE002000C),
	OperationDeclined		= int32_t(0xE002000D),
	AlreadyExists			= int32_t(0xE002000E),
	ShutdownIsStarted		= int32_t(0xE002000F),
	OperationCancelled		= int32_t(0xE0020010),
	CompileError			= int32_t(0xE0020011),
	SlowLaneOperation		= int32_t(0xE0020012),

	// System errors
	SystemError				= int32_t(0xE0030000),
};

///
/// Checks if the passed error code means error.
///
/// @param errCode - error code.
/// @return The function returns `true` if the `errCode` means an error.
///
inline bool isError(ErrorCode errCode)
{
	return int32_t(errCode) < 0;
}

///
/// Get description for specified error code.
///
/// @param errCode - Error code.
/// @return The function returns text string which describes specified error code.
///
const char* getErrorString(ErrorCode errCode);

namespace detail {

//
//
//
struct StackTraceItem
{
	int nLine;
	std::string sFile;
	std::string sComponent;
	std::string sDescription;

	StackTraceItem(const std::string& sFile, int nLine, const std::string& sComponent,
		const std::string& sDescription) :
		sFile(sFile),
		nLine(nLine),
		sComponent(sComponent),
		sDescription(sDescription)
	{}
};

//
//
//
using StackTrace = std::list<StackTraceItem>;

///
/// Exception (error) base class.
///
class ExceptionBase : public std::exception
{
private:

	StackTrace m_stackTrace;
	ErrorCode m_errCode = ErrorCode::Undefined;
	Size m_nStackTraceInitialSize = 0;

protected:

	virtual std::exception_ptr getRealException();
	void addTrace(std::string_view sFile, int nLine, std::string_view sComponent, std::string_view sDescription);

public:

	///
	/// Constructor.
	///
	/// @param sMsg [opt] - the error description (default: "").
	///
	ExceptionBase(std::string_view sMsg = "");

	///
	/// Constructor.
	///
	/// @param errCode - the error code.
	/// @param sMsg [opt] - the error description (default: "").
	///
	ExceptionBase(ErrorCode errCode, std::string_view sMsg = "");

	///
	/// Constructor.
	///
	/// @param srcLoc - information about a source file and line.
	/// @param sMsg [opt] - the error description (default: "").
	///
	ExceptionBase(SourceLocation srcLoc, std::string_view sMsg = "");

	///
	/// Constructor.
	///
	/// @param srcLoc - information about a source file and line.
	/// @param errCode - the error code.
	/// @param sMsg [opt] - the error description (default: "").
	///
	ExceptionBase(SourceLocation srcLoc, ErrorCode errCode, std::string_view sMsg = "");

	///
	/// Constructor.
	///
	/// @param vSerialized - exception object serialized in Variant.
	///
	ExceptionBase(const Variant& vSerialized);

	///
	/// Destructor.
	///
	virtual ~ExceptionBase();

	///
	/// Prints the exception's details to `std::cerr`.
	///
	virtual void print() const;

	///
	/// Prints the exception's details to an arbitrary `std::ostream`.
	///
	/// @param os - the target `std::ostream` object.
	///
	virtual void print(std::ostream& os) const;

	///
	/// Prints the exception's stack to an arbitrary `std::ostream`.
	///
	/// @param os - the target `std::ostream` object.
	/// @param sIndent [opt] - the line indentation string (default: "").
	///
	virtual void printStack(std::ostream& os, std::string_view sIndent = "") const;

	///
	/// Gets the exception's error code.
	///
	/// @returns The function returns the value of an error code.
	///
	virtual ErrorCode getErrorCode() const;

	///
	/// Gets the exception's stacktrace.
	///
	/// @returns The function returns the exception's stacktrace object.
	///
	virtual const StackTrace& getStackTrace() const;

	///
	/// Appends a new trace.
	///
	/// @param srcLoc - the source location in source file.
	/// @param sDescription - the trace's description.
	/// @retuns The function returns a reference to itself.
	///
	virtual ExceptionBase& addTrace(SourceLocation srcLoc, std::string_view sDescription);

	///
	/// Throws itself as an exception.
	///
	[[noreturn]] void throwException();

	///
	/// Logs itself as an error (without a message).
	///
	virtual void log();

	///
	/// Logs itself as an error.
	///
	/// @param sMsg - the additional description.
	///
	virtual void log(std::string_view sMsg);

	///
	/// Logs itself as an error (without a message).
	///
	/// @param srcLoc - information about a source file and line.
	///
	virtual void log(SourceLocation srcLoc);

	///
	/// Logs itself as an error
	///
	/// @param srcLoc - information about a source file and line.
	/// @param sMsg - the additional description.
	///
	virtual void log(SourceLocation srcLoc, std::string_view sMsg);

	///
	/// Logs itself as an error (without a message).
	///
	virtual void logAsWarning();

	///
	/// Logs itself as a warning.
	///
	/// @param sMsg - the additional description.
	///
	virtual void logAsWarning(std::string_view sMsg);

	///
	/// Serializes the exception to Variant.
	///
	/// @return The function returns a Variant value. 
	///
	virtual Variant serialize() const;

	///
	/// Checks that exception class or its base classes has specified ErrorCode.
	///
	/// @param errCode - ErrorCode value to check.
	/// @return The function returns `true` if the exception is (or based on) the specified `errCode`. . 
	///
	virtual bool isBasedOnCode(ErrorCode errCode) const;
};

//
// Top level error
//
template<class Base, ErrorCode ERR_CODE>
class ExceptionBaseTmpl : public Base
{
	static const inline ErrorCode c_baseErrCode = ERR_CODE;

	virtual std::exception_ptr getRealException()
	{
		return std::make_exception_ptr(*this);
	}

public:
	ExceptionBaseTmpl(std::string_view sMsg = "") :
		Base(c_baseErrCode, sMsg) {}
	ExceptionBaseTmpl(ErrorCode errCode, std::string_view sMsg = "") :
		Base(errCode, sMsg) {}
	ExceptionBaseTmpl(SourceLocation srcLoc, std::string_view sMsg = "") :
		Base(srcLoc, c_baseErrCode, sMsg) {}
	ExceptionBaseTmpl(SourceLocation srcLoc, ErrorCode errCode, std::string_view sMsg = "") :
		Base(srcLoc, errCode, sMsg) {}
	ExceptionBaseTmpl(const Base& base) :
		Base(base) {}
	virtual ~ExceptionBaseTmpl() {}

	//
	//
	//
	bool isBasedOnCode(ErrorCode errCode) const override
	{
		if (errCode == c_baseErrCode)
			return true;
		return Base::isBasedOnCode(errCode);
	}
};

} // namespace detail

// Generic exception
using Exception = openEdr::error::detail::ExceptionBase;

// Logic exceptions
using LogicError = detail::ExceptionBaseTmpl<Exception, ErrorCode::LogicError>;
using InvalidArgument = detail::ExceptionBaseTmpl<LogicError, ErrorCode::InvalidArgument>;
using InvalidUsage = detail::ExceptionBaseTmpl<LogicError, ErrorCode::InvalidUsage>;
using OutOfRange = detail::ExceptionBaseTmpl<LogicError, ErrorCode::OutOfRange>;
using TypeError = detail::ExceptionBaseTmpl<LogicError, ErrorCode::TypeError>;

// Runtime exceptions
using RuntimeError = detail::ExceptionBaseTmpl<Exception, ErrorCode::RuntimeError>;
using AccessDenied = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::AccessDenied>;
using ArithmeticError = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::ArithmeticError>;
using FileLocked = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::FileLocked>;
using NotFound = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::NotFound>;
using InvalidFormat = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::InvalidFormat>;
using LimitExceeded = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::LimitExceeded>;
using NoData = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::NoData>;
using StringEncodingError = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::StringEncodingError>;
using OperationNotSupported = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::OperationNotSupported>;
using ConnectionError = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::ConnectionError>;
using BadAlloc = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::BadAlloc>;
using PoolThreadStopped = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::PoolThreadStopped>;
using OperationDeclined = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::OperationDeclined>;
using AlreadyExists = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::AlreadyExists>;
using ShutdownIsStarted = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::ShutdownIsStarted>;
using OperationCancelled = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::OperationCancelled>;

// System exceptions
using SystemError = detail::ExceptionBaseTmpl<RuntimeError, ErrorCode::SystemError>;

///
/// CRT errors wrapper class.
///
/// Some CRT `errno` values are translated to a specific error:
///  * `EINVAL` -> `error::InvalidArgument`
///  * `ENOENT` -> `error::NotFound`
///
class CrtError : public SystemError
{
private:
	errno_t m_errNo;

	std::exception_ptr getRealException() override;
	std::string createWhat(errno_t errNo, std::string_view sMsg);

public:
	///
	/// Constructor.
	///
	/// @param errNo - the `errno` value.
	/// @param sMsg [opt] - the error description (default: "").
	///
	CrtError(errno_t errNo, std::string_view sMsg = "");

	///
	/// Constructor.
	///
	/// @param srcLoc - information about a source file and line.
	/// @param sMsg [opt] - the error description (default: "").
	///
	CrtError(const SourceLocation srcLoc, std::string_view sMsg = "");

	///
	/// Constructor.
	///
	/// @param sMsg [opt] - the error description (default: "").
	///
	CrtError(std::string_view sMsg = "");

	///
	/// Destructor.
	///
	virtual ~CrtError();

	///
	/// Gets the `errno` value.
	///
	/// @returns The function returns the `errno` value.
	///
	errno_t getErrNo() const;
};

///
/// The `std` system errors wrapper class.
///
/// This class is intended for wrapping errors of `std::system_error`.
///
class StdSystemError : public SystemError
{
private:
	std::error_code m_stdErrorCode;
	std::string createWhat(std::error_code ec, std::string_view sMsg);

	std::exception_ptr getRealException() override;

public:

	///
	/// Constructor.
	///
	/// @param ec - a value of `std::error_code`.
	/// @param sMsg [opt] - an error description (default: "").
	///
	StdSystemError(std::error_code ec, std::string_view sMsg = "");

	///
	/// Constructor.
	///
	/// @param srcLoc - information about a source file and line.
	/// @param ec - a value of `std::error_code`.
	/// @param sMsg [opt] - an error description (default: "").
	///
	StdSystemError(SourceLocation srcLoc, std::error_code ec, std::string_view sMsg = "");

	///
	/// Destructor.
	///
	virtual ~StdSystemError();

	///
	/// Returns the `std` error code.
	///
	/// @returns The function returns the value of `std::error_code`.
	///
	std::error_code getStdErrorCode() const;
};

///
/// The `boost` system errors wrapper class.
///
/// This class is intended for wrapping errors of `boost::system::system_error`.
///
class BoostSystemError : public SystemError
{
private:
	boost::system::error_code m_boostErrorCode;
	std::string createWhat(boost::system::error_code ec, std::string_view sMsg);

	std::exception_ptr getRealException() override;

public:

	///
	/// Constructor.
	///
	/// @param ec - a value of `boost::system::error_code`.
	/// @param sMsg [opt] - an error description (default: "").
	///
	BoostSystemError(boost::system::error_code ec, std::string_view sMsg = "");

	///
	/// Constructor.
	///
	/// @param srcLoc - information about a source file and line.
	/// @param ec - a value of `std::error_code`.
	/// @param sMsg [opt] - an error description (default: "").
	///
	BoostSystemError(SourceLocation srcLoc, boost::system::error_code ec, std::string_view sMsg = "");

	///
	/// Destructor.
	///
	virtual ~BoostSystemError();

	///
	/// Returns the `boost` error code.
	///
	/// @returns The function returns the value of `boost::system::error_code`.
	///
	std::error_code getBoostErrorCode() const;
};

//
// Helps to output error code with description using operator<<.
//
template<typename T>
struct StackTraceFmt
{
	SourceLocation m_srcLoc;
	const T& m_value;
	const char* m_sIndent = "";

	StackTraceFmt(SourceLocation srcLoc, const T& value, std::string_view sIndent = "") :
		m_srcLoc(srcLoc), m_value(value), m_sIndent(sIndent.data()) {}
};

//
// Helps to output error code with description using operator<<.
//
struct ErrorCodeFmt
{
	ErrorCode m_errCode;

	ErrorCodeFmt(ErrorCode errCode) :
		m_errCode(errCode) {}
	ErrorCodeFmt(const Exception& exception) :
		m_errCode(exception.getErrorCode()) {}
};

//
//
//
struct SourceLocationFmt
{
	SourceLocation m_srcLoc;
	const char* m_sIndent = "";

	SourceLocationFmt(SourceLocation srcLoc, std::string_view sIndent = "") :
		m_srcLoc(srcLoc), m_sIndent(sIndent.data()) {}
};

} // namespace error

using error::ErrorCode;
using error::isError;

//
//
//
inline std::ostream& operator<<(std::ostream& os, const error::ErrorCode& errCode)
{
	return (os << hex(errCode));
}

//
// Output exception to std::ostream
//
inline std::ostream& operator<<(std::ostream& os, const error::Exception& ex)
{
	ex.print(os);
	return os;
}

//
//
//
template<typename T>
inline std::ostream& operator<<(std::ostream& os, const error::StackTraceFmt<T>& fmt)
{
	fmt.m_value.printStack(os, fmt.m_sIndent);
	return os;
}

//
//
//
inline std::ostream& operator<<(std::ostream& os, const error::ErrorCodeFmt& fmt)
{
	os << fmt.m_errCode << " - " << error::getErrorString(fmt.m_errCode);
	return os;
}

//
//
//
inline std::ostream& operator<<(std::ostream& os, const error::StackTraceFmt<error::ErrorCode>& fmt)
{
	os << fmt.m_sIndent << fmt.m_srcLoc.sFileName << ':' << fmt.m_srcLoc.nLine
		<< " (" << error::ErrorCodeFmt(fmt.m_value) << ")";
	return os;
}

//
//
//
inline std::ostream& operator<<(std::ostream& os, const error::SourceLocationFmt& fmt)
{
	os << fmt.m_sIndent << fmt.m_srcLoc.sFileName << ":" << fmt.m_srcLoc.nLine;
	return os;
}

} // namespace openEdr

///
/// Trace scope's begin macro
///
#define CMD_TRACE_BEGIN try { CHECK_IN_SOURCE_LOCATION();

//
//
//
struct __TraceMsgHlp
{
	const char* pWhat = "";
	__TraceMsgHlp(const std::exception& ex) : pWhat(ex.what()) {}
	std::string get() { return std::string(pWhat); }
	std::string get(const char* pMsg) { return std::string(pMsg) + ": " + pWhat; }
	std::string get(const std::string& sMsg) { return sMsg + ": " + pWhat; }
};

///
/// Trace scope's end macro.
///
/// In case of exception, an additional description can be supplied by parameter.
///
#define CMD_TRACE_END(...) } \
	catch (openEdr::error::Exception& e) \
	{ e.addTrace(SL, std::string(__VA_ARGS__)); throw; } \
	catch (const boost::system::system_error& e) \
	{ openEdr::error::BoostSystemError(SL, e.code(), std::string(__VA_ARGS__)).throwException(); } \
	catch (const std::system_error& e) \
	{ openEdr::error::StdSystemError(SL, e.code(), std::string(__VA_ARGS__)).throwException(); } \
	catch (const std::out_of_range& e) \
	{ openEdr::error::OutOfRange(SL, __TraceMsgHlp(e).get(__VA_ARGS__)).throwException(); } \
	catch (const std::invalid_argument& e) \
	{ openEdr::error::InvalidArgument(SL, __TraceMsgHlp(e).get(__VA_ARGS__)).throwException(); } \
	catch (const std::logic_error& e) \
	{ openEdr::error::LogicError(SL, __TraceMsgHlp(e).get(__VA_ARGS__)).throwException(); } \
	catch (const std::runtime_error& e) \
	{ openEdr::error::RuntimeError(SL, __TraceMsgHlp(e).get(__VA_ARGS__)).throwException(); } \
	catch (const std::bad_alloc& e) \
	{ openEdr::error::BadAlloc(SL, __TraceMsgHlp(e).get(__VA_ARGS__)).throwException(); } \
	catch (const std::exception& e) \
	{ openEdr::error::Exception(SL, __TraceMsgHlp(e).get(__VA_ARGS__)).throwException(); } \
	catch (...) \
	{ openEdr::error::Exception(SL, std::string(__VA_ARGS__)).throwException(); }

/// @copydoc CMD_TRACE_BEGIN
#define TRACE_BEGIN CMD_TRACE_BEGIN

/// @copydoc CMD_TRACE_END
#define TRACE_END(...) CMD_TRACE_END(__VA_ARGS__)

///
/// A special try-catch block begin macro.
///
/// Shall be used only with CMD_PREPARE_CATCH macro.
///
#define CMD_TRY try { CHECK_IN_SOURCE_LOCATION(); try

///
/// Macro for `std` exceptions to `cmd` exceptions translaction.
///
/// This macro shall be used only with `CMD_TRY` macro. Table of translation: 
///  * `std::logic_error` -> `error::LogicError`
///  * `std::invalid_argument` -> `error::InvalidArgument`
///  * `std::domain_error` -> `error::LogicError`
///  * `std::length_error` -> `error::LogicError`
///  * `std::out_of_range` -> `error::OutOfRange`
///  * `std::future_error` -> `error::LogicError`
///  * `std::bad_optional_access` -> `error::Exception`
///  * `std::runtime_error` -> `error::RuntimeError`
///  * `std::range_error` -> `error::RuntimeError`
///  * `std::overflow_error` -> `error::RuntimeError`
///  * `std::underflow_error` -> `error::RuntimeError`
///  * `std::regex_error` -> `error::RuntimeError`
///  * `std::system_error` -> `error::StdSystemError`
///  * `std::ios_base::failure` -> `error::StdSystemError`
///  * `std::filesystem::filesystem_error` -> `error::StdSystemError`
///  * `std::bad_typeid` -> `error::Exception`
///  * `std::bad_cast` -> `error::Exception`
///  * `std::bad_any_cast` -> `error::Exception`
///  * `std::bad_weak_ptr` -> `error::Exception`
///  * `std::bad_function_call` -> `error::Exception`
///  * `std::bad_alloc` -> `error::BadAlloc`
///  * `std::bad_array_new_length` -> `error::BadAlloc`
///  * `std::bad_exception` -> `error::Exception`
///  * `std::bad_variant_access` -> `error::Exception`
///  * `boost::system::system_error` -> `error::BoostSystemError`
///  * other `std` exception` -> `error::Exception`
///
#define CMD_PREPARE_CATCH \
	catch (const openEdr::error::Exception&) { throw; } \
	catch (const boost::system::system_error& ex) \
		{ openEdr::error::BoostSystemError(SL, ex.code()).throwException(); } \
	catch (const std::invalid_argument& ex) { openEdr::error::InvalidArgument(SL, ex.what()).throwException(); } \
	catch (const std::out_of_range& ex) { openEdr::error::OutOfRange(SL, ex.what()).throwException(); } \
	catch (const std::logic_error& ex) { openEdr::error::LogicError(SL, ex.what()).throwException(); } \
	catch (const std::system_error& ex) { openEdr::error::StdSystemError(SL, ex.code()).throwException(); } \
	catch (const std::runtime_error& ex) { openEdr::error::RuntimeError(SL, ex.what()).throwException(); } \
	catch (const std::bad_alloc& ex) { openEdr::error::BadAlloc(SL, ex.what()).throwException(); } \
	catch (const std::exception& ex) { openEdr::error::Exception(SL, ex.what()).throwException(); } \
	catch (...) { openEdr::error::Exception(SL, "Unknown exception").throwException(); } \
	}

#define CMD_CATCH(msg) \
	CMD_PREPARE_CATCH \
	catch (error::Exception& ex) \
	{ ex.log(SL, msg); }

/// @}
