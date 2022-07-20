//
// edrav2.libcore project
//
// Author: Yury Ermakov (15.01.2019)
// Reviewer: Denis Bogdanov (13.02.2019)
//
///
/// @file Exceptions and errors implementation
///
#include "pch.h"

namespace cmd {
namespace error {

//
//
//
const char* getErrorString(ErrorCode errCode)
{
	switch (errCode)
	{
		case ErrorCode::Undefined: return "Undefined";
		case ErrorCode::OK: return "OK";

		// Logic errors
		case ErrorCode::LogicError:	return "Logic error";
		case ErrorCode::InvalidArgument: return "Invalid argument";
		case ErrorCode::InvalidUsage: return "Invalid usage scenario";
		case ErrorCode::OutOfRange: return "Out of range";
		case ErrorCode::TypeError: return "Type error";
		
		// Runtime errors
		case ErrorCode::RuntimeError: return "Runtime error";
		case ErrorCode::AccessDenied: return "Access is denied";
		case ErrorCode::ArithmeticError: return "Arithmetic error";
		case ErrorCode::FileLocked: return "File is locked";
		case ErrorCode::NotFound: return "Resource is not found";
		case ErrorCode::InvalidFormat: return "Invalid format";
		case ErrorCode::LimitExceeded: return "Limit is exceeded";
		case ErrorCode::NoData: return "No data";
		case ErrorCode::StringEncodingError: return "String encoding error";
		case ErrorCode::OperationNotSupported: return "Operation is not supported";
		case ErrorCode::ConnectionError: return "Connection error";
		case ErrorCode::BadAlloc: return "Memory allocation error";
		case ErrorCode::PoolThreadStopped: return "Thread from pool is stopped";
		case ErrorCode::OperationDeclined: return "Operation is declined";
		case ErrorCode::AlreadyExists: return "Object already exists";
		case ErrorCode::ShutdownIsStarted: return "Application is shutting down";
		case ErrorCode::OperationCancelled: return "Operation is cancelled";			
		case ErrorCode::CompileError: return "Compile error";			

		// System errors
		case ErrorCode::SystemError: return "System error";
	}

	return "Unknown error";
}

namespace detail {

//
//
//
ExceptionBase::ExceptionBase(std::string_view sMsg) :
	std::exception(!sMsg.empty() ? std::string(sMsg).c_str() : getErrorString(ErrorCode::Undefined))
{
}

//
//
//
ExceptionBase::ExceptionBase(ErrorCode errCode, std::string_view sMsg) :
	m_errCode(errCode),
	std::exception(!sMsg.empty() ? std::string(sMsg).c_str() : getErrorString(errCode))
{
}

//
//
//
ExceptionBase::ExceptionBase(SourceLocation srcLoc, ErrorCode errCode, std::string_view sMsg) :
	m_errCode(errCode),
	std::exception(!sMsg.empty() ? std::string(sMsg).c_str() : getErrorString(errCode))
{
	addTrace(srcLoc, what());
}

//
//
//
ExceptionBase::ExceptionBase(SourceLocation srcLoc, std::string_view sMsg) :
	std::exception(!sMsg.empty() ? std::string(sMsg).c_str() : getErrorString(ErrorCode::Undefined))
{
	addTrace(srcLoc, what());
}

//
//
//
ExceptionBase::ExceptionBase(const Variant& vSerialized) :
	m_errCode(vSerialized.get("errCode", ErrorCode::Undefined)),
	std::exception(std::string(vSerialized.get("sMsg",
		getErrorString(vSerialized.get("errCode", ErrorCode::Undefined)))).c_str())
{
	if (vSerialized.has("stackTrace"))
		for (auto it : vSerialized["stackTrace"])
			addTrace(it.get("sFile", "<undefined>"), it["nLine"], it.get("sComponent", ""), it.get("sDescription", ""));
	
	m_nStackTraceInitialSize = m_stackTrace.size();
}

//
//
//
ExceptionBase::~ExceptionBase()
{
}

//
// TODO: rename to makeException, fAddTrace?
//
std::exception_ptr ExceptionBase::getRealException()
{
	switch (m_errCode)
	{
		// Logic errors
		case ErrorCode::LogicError:	return std::make_exception_ptr(LogicError(*this));
		case ErrorCode::InvalidArgument: return std::make_exception_ptr(InvalidArgument(*this));
		case ErrorCode::InvalidUsage: return std::make_exception_ptr(InvalidUsage(*this));
		case ErrorCode::OutOfRange: return std::make_exception_ptr(OutOfRange(*this));
		case ErrorCode::TypeError: return std::make_exception_ptr(TypeError(*this));

		// Runtime errors
		case ErrorCode::RuntimeError: return std::make_exception_ptr(RuntimeError(*this));
		case ErrorCode::AccessDenied: return std::make_exception_ptr(AccessDenied(*this));
		case ErrorCode::ArithmeticError: return std::make_exception_ptr(ArithmeticError(*this));
		case ErrorCode::FileLocked: return std::make_exception_ptr(FileLocked(*this));
		case ErrorCode::NotFound: return std::make_exception_ptr(NotFound(*this));
		case ErrorCode::InvalidFormat: return std::make_exception_ptr(InvalidFormat(*this));
		case ErrorCode::LimitExceeded: return std::make_exception_ptr(LimitExceeded(*this));
		case ErrorCode::NoData: return std::make_exception_ptr(NoData(*this));
		case ErrorCode::StringEncodingError: return std::make_exception_ptr(StringEncodingError(*this));
		case ErrorCode::OperationNotSupported: return std::make_exception_ptr(OperationNotSupported(*this));
		case ErrorCode::ConnectionError: return std::make_exception_ptr(ConnectionError(*this));
		case ErrorCode::BadAlloc: return std::make_exception_ptr(BadAlloc(*this));
		case ErrorCode::PoolThreadStopped: return std::make_exception_ptr(PoolThreadStopped(*this));
		case ErrorCode::OperationDeclined: return std::make_exception_ptr(OperationDeclined(*this));
		case ErrorCode::AlreadyExists: return std::make_exception_ptr(AlreadyExists(*this));
		case ErrorCode::ShutdownIsStarted: return std::make_exception_ptr(ShutdownIsStarted(*this));
		case ErrorCode::OperationCancelled: return std::make_exception_ptr(OperationCancelled(*this));

		// System errors
		case ErrorCode::SystemError: return std::make_exception_ptr(SystemError(*this));
	}

	return std::make_exception_ptr(*this);
}

//
//
//
void ExceptionBase::print() const
{
	print(std::cerr);
}

//
//
//
void ExceptionBase::print(std::ostream& os) const
{
	os << "Exception: " << ErrorCodeFmt(*this) << std::endl;
	printStack(os);
}

//
//
//
void ExceptionBase::printStack(std::ostream& os, std::string_view sIdent) const
{
	auto i = m_stackTrace.size();
	for (auto it = m_stackTrace.crbegin(); it != m_stackTrace.crend(); ++it, --i)
		os << sIdent << ((i <= m_nStackTraceInitialSize) ? " | " : "")
			<< it->sFile << ':' << it->nLine << " <" << it->sComponent << "> ("
			<< it->sDescription << ")" << std::endl;
}

//
//
//
ErrorCode ExceptionBase::getErrorCode() const
{
	return m_errCode;
}

//
//
//
const StackTrace& ExceptionBase::getStackTrace() const
{
	return m_stackTrace;
}

//
//
//
ExceptionBase& ExceptionBase::addTrace(SourceLocation srcLoc, std::string_view sDescription)
{
	addTrace(srcLoc.sFileName, srcLoc.nLine, srcLoc.sComponent, sDescription);
	return *this;
}

//
//
//
void ExceptionBase::addTrace(std::string_view sFile, int nLine, std::string_view sComponent,
	std::string_view sDescription)
{
	m_stackTrace.emplace_back(std::string(sFile), nLine, std::string(sComponent), std::string(sDescription));
}

//
//
//
void ExceptionBase::throwException()
{
	std::rethrow_exception(getRealException());
}

//
// TODO: Can we do it more effectively (without throwing)
//
void ExceptionBase::log()
{ 
	try
	{
		throwException();
	}
	catch (const Exception& ex)
	{
		if (!ex.getStackTrace().empty())
			LOGERRNOSL(ex.getStackTrace().back().sComponent, ex);
		else
			LOGERRNOSL(ex);
	}
}

//
// TODO: Can we do it more effectively (without throwing)
//
void ExceptionBase::logAsWarning()
{
	try
	{
		throwException();
	}
	catch (const Exception& ex)
	{
		if (!ex.getStackTrace().empty())
			LOGWRN(ex.getStackTrace().back().sComponent, ex.what());
		else
			LOGWRN(ex.what());
	}
}

//
//
//
void ExceptionBase::log(std::string_view sMsg)
{
	try
	{
		std::rethrow_exception(getRealException());
	}
	catch (const Exception& ex)
	{
		if (!ex.getStackTrace().empty())
			LOGERRNOSL(ex.getStackTrace().back().sComponent, ex, sMsg);
		else
			LOGERRNOSL("", ex, sMsg);
	}
}

//
//
//
void ExceptionBase::logAsWarning(std::string_view sMsg)
{
	try
	{
		std::rethrow_exception(getRealException());
	}
	catch (const Exception& ex)
	{
		if (!ex.getStackTrace().empty())
			LOGWRN(ex.getStackTrace().back().sComponent, sMsg << " (" << ex.what() << ")");
		else
			LOGWRN(sMsg << " (" << ex.what() << ")");
	}
}

//
//
//
void ExceptionBase::log(SourceLocation srcLoc)
{
	addTrace(srcLoc, what());
	log();
}

//
//
//
void ExceptionBase::log(SourceLocation srcLoc, std::string_view sMsg)
{
	addTrace(srcLoc, sMsg);
	log();
}

//
//
//
Variant ExceptionBase::serialize() const
{
	Variant vStackTrace = Sequence();
	for (auto it = m_stackTrace.cbegin(); it != m_stackTrace.cend(); ++it)
		vStackTrace.push_back(Dictionary({
			{"sFile", it->sFile},
			{"nLine", it->nLine},
			{"sComponent", it->sComponent},
			{"sDescription", it->sDescription}
		}));

	Variant vResult = Dictionary({
		{ "sMsg", what() },
		{ "errCode", m_errCode },
		{ "stackTrace", vStackTrace }
	});
	
	return vResult;
}

//
//
//
bool ExceptionBase::isBasedOnCode(ErrorCode errCode) const
{
	// ExceptionBase does not have own exception code
	return m_errCode == errCode;
}


} // namespace detail

//////////////////////////////////////////////////////////////////////////////
//
// CrtError methods
//
//////////////////////////////////////////////////////////////////////////////

//
//
//
std::string CrtError::createWhat(errno_t errNo, std::string_view sMsg)
{
	const size_t nErrLength = 0x400;
	char errBuf[nErrLength]; *errBuf = 0;
	strerror_s(errBuf, nErrLength, errNo);

	std::string s(sMsg);
	if (s.empty()) 
		s += "CRT error " + string::convertToHex(errNo) + " - " + std::string(errBuf);
	else
		s += " [CRT error " + string::convertToHex(errNo) + " - " + std::string(errBuf) + "]";

	return s;
}

//
//
//
CrtError::CrtError(errno_t errNo, std::string_view sMsg) :
	SystemError(createWhat(errNo, sMsg)),
	m_errNo(errNo)
{
}

//
//
//
CrtError::CrtError(std::string_view sMsg) :
	SystemError(createWhat(m_errNo = errno, sMsg))
{
}

//
//
//
CrtError::CrtError(SourceLocation srcLoc, std::string_view sMsg) :
	SystemError(srcLoc, createWhat(m_errNo = errno, sMsg))
{
}

//
//
//
CrtError::~CrtError()
{
}

//
// 
//
errno_t CrtError::getErrNo() const
{
	return m_errNo;
}

//
//
//
std::exception_ptr CrtError::getRealException()
{
	if (m_errNo == EINVAL)
		return std::make_exception_ptr(InvalidArgument(*this));
	if (m_errNo == ENOENT)
		return std::make_exception_ptr(NotFound(*this));
	return std::make_exception_ptr(*this);
}

//////////////////////////////////////////////////////////////////////////////
//
// StdSystemError methods
//
//////////////////////////////////////////////////////////////////////////////

//
//
//
std::string StdSystemError::createWhat(std::error_code ec, std::string_view sMsg)
{
	std::string s(sMsg);
	if (s.empty())
		s += "System error " + string::convertToHex(ec.value()) +
			" - " + string::convertToUtf8(ec.message());
	else
		s += " [System error " + string::convertToHex(ec.value()) +
			" - " + string::convertToUtf8(ec.message()) + "]";

	return s;
}

//
//
//
StdSystemError::StdSystemError(std::error_code ec, std::string_view sMsg) :
	SystemError(createWhat(ec, sMsg)),
	m_stdErrorCode(ec)
{
}

//
//
//
StdSystemError::StdSystemError(SourceLocation srcLoc, std::error_code ec, std::string_view sMsg) :
	SystemError(srcLoc, createWhat(ec, sMsg)),
	m_stdErrorCode(ec)
{
}

//
//
//
StdSystemError::~StdSystemError()
{
}

//
// 
//
std::error_code StdSystemError::getStdErrorCode() const
{
	return m_stdErrorCode;
}

//
//
//
std::exception_ptr StdSystemError::getRealException()
{
	return std::make_exception_ptr(*this);
}

//////////////////////////////////////////////////////////////////////////////
//
// BoostSystemError methods
//
//////////////////////////////////////////////////////////////////////////////

//
//
//
std::string BoostSystemError::createWhat(boost::system::error_code ec, std::string_view sMsg)
{
	std::string s(sMsg);
	if (s.empty())
		s += "System error " + string::convertToHex(ec.value()) +
			" - " + string::convertToUtf8(ec.message());
	else
		s += " [System error " + string::convertToHex(ec.value()) +
			" - " + string::convertToUtf8(ec.message()) + "]";

	return s;
}

//
//
//
BoostSystemError::BoostSystemError(boost::system::error_code ec, std::string_view sMsg) :
	SystemError(createWhat(ec, sMsg)),
	m_boostErrorCode(ec)
{
}

//
//
//
BoostSystemError::BoostSystemError(SourceLocation srcLoc, boost::system::error_code ec,
	std::string_view sMsg) :
	SystemError(srcLoc, createWhat(ec, sMsg)),
	m_boostErrorCode(ec)
{
}

//
//
//
BoostSystemError::~BoostSystemError()
{
}

//
// 
//
std::error_code BoostSystemError::getBoostErrorCode() const
{
	return m_boostErrorCode;
}

//
//
//
std::exception_ptr BoostSystemError::getRealException()
{
	if ((m_boostErrorCode == boost::beast::http::error::end_of_stream) ||
		(m_boostErrorCode == boost::beast::http::error::partial_message) ||
		(m_boostErrorCode == boost::asio::error::connection_aborted) ||
		(m_boostErrorCode == boost::asio::error::connection_refused) ||
		(m_boostErrorCode == boost::asio::error::connection_reset) ||
		(m_boostErrorCode == boost::asio::error::not_connected) ||
		(m_boostErrorCode == boost::asio::error::not_found) ||
		(m_boostErrorCode == boost::asio::error::timed_out) ||
		(m_boostErrorCode == boost::asio::error::host_unreachable) ||
		(m_boostErrorCode == boost::asio::error::host_not_found) ||
		(m_boostErrorCode == boost::asio::error::host_not_found_try_again) ||
		(m_boostErrorCode == boost::asio::error::network_down) ||
		(m_boostErrorCode == boost::asio::error::network_reset) ||
		(m_boostErrorCode == boost::asio::error::network_unreachable))
		return std::make_exception_ptr(ConnectionError(*this));

	if (m_boostErrorCode.value() == 10022 /* WSAEINVAL */ || 
		m_boostErrorCode.value() == 11001 /* WSAHOST_NOT_FOUND */ ||
		m_boostErrorCode.value() == 10050 /* WSAENETDOWN */ ||
		m_boostErrorCode.value() == 10051 /* WSAENETUNREACH */ || 
		m_boostErrorCode.value() == 10060 /* WSAETIMEDOUT */)
		return std::make_exception_ptr(ConnectionError(*this));
	return std::make_exception_ptr(*this);
}

} // namespace error
} // namespace cmd
