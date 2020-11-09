#ifndef LOG4CPLUS_LOG4CPLUS_SWG
#define LOG4CPLUS_LOG4CPLUS_SWG

%module log4cplus

%include "std_basic_string.i"
#ifdef SWIGPYTHON
%include "std_string.i"
#ifdef UNICODE
%include "std_wstring.i"
#endif
#endif

%{
#include "log4cplus/tchar.h"
#include "log4cplus/tstring.h"
%}

namespace log4cplus
{

#ifdef UNICODE
typedef wchar_t tchar;
#else
typedef char tchar;
#endif

typedef std::basic_string<tchar> tstring;

typedef int LogLevel;

} // namespace log4cplus

%include "hierarchy.swg"
%include "loggingevent.swg"
%include "logger.swg"
%include "configurator.swg"

#endif // LOG4CPLUS_LOG4CPLUS_SWG

