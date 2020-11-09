#ifndef LOG4CPLUS_LOGGER_SWG
#define LOG4CPLUS_LOGGER_SWG

%module log4cplus

%{

#include "log4cplus/logger.h"

%}

%include "hierarchy.swg"


namespace log4cplus
{

%rename(assign) Logger::operator =;	
	
class Logger
{
public:
  Logger ();
  Logger (Logger const & other);
  Logger & operator = (Logger const & other);
  virtual ~Logger ();

  void swap (Logger & other);
  Logger getParent () const;

  void assertion (bool assertionVal, log4cplus::tstring const & message) const;
  bool isEnabledFor (LogLevel ll) const;
  void log (LogLevel ll, log4cplus::tstring const & message,
    char const * file = NULL, int line = -1, char const * function = NULL)
    const;
  void log (spi::InternalLoggingEvent const & event) const;
  void forcedLog (LogLevel ll, log4cplus::tstring const & message,
    char const * file = NULL, int line = -1, char const * function = NULL)
    const;
  void forcedLog (spi::InternalLoggingEvent const & event) const;
  LogLevel getChainedLogLevel () const;
  LogLevel getLogLevel () const;
  void setLogLevel (LogLevel ll);
  log4cplus::tstring const & getName () const;
  bool getAdditivity () const;
  void setAdditivity (bool additive);

  static Hierarchy & getDefaultHierarchy ();
  static bool exists (log4cplus::tstring const & loggerName);
  static Logger getInstance (log4cplus::tstring const & loggerName);
  static Logger getRoot ();  
};

} // namespace Logger

#endif // LOG4CPLUS_LOGGER_SWG

