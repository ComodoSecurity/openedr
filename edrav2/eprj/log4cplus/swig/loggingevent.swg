#ifndef LOG4CPLUS_LOGGINGEVENT_SWG
#define LOG4CPLUS_LOGGINGEVENT_SWG

%{

#include "log4cplus/spi/loggingevent.h"

%}

namespace log4cplus { namespace spi {

%rename(assign) InternalLoggingEvent::operator =;
  
class InternalLoggingEvent
{
public:
  InternalLoggingEvent ();
  InternalLoggingEvent (log4cplus::tstring const & logger,
    log4cplus::LogLevel ll, log4cplus::tstring const & message,
    char const * filename, int line, char const * function);
  InternalLoggingEvent (InternalLoggingEvent const & other);
  virtual ~InternalLoggingEvent ();
  
  InternalLoggingEvent & operator = (InternalLoggingEvent const & other);
  
  void swap (InternalLoggingEvent & other);
  
  void setLoggingEvent (log4cplus::tstring const & logger,
    log4cplus::LogLevel ll, log4cplus::tstring const & message,
    char const * filename, int line, char const * function);
  
  void setFunction (char const * func);
  void setFunction (log4cplus::tstring const & func);
  
  virtual log4cplus::tstring const & getMessage () const;
  log4cplus::tstring const & getLoggerName () const;
  log4cplus::LogLevel getLogLevel () const;
  log4cplus::tstring const & getFile () const;
  int getLine () const;
  
};
  
} } // namespace log4cplus { namespace spi {

#endif // LOG4CPLUS_LOGGINGEVENT_SWG

