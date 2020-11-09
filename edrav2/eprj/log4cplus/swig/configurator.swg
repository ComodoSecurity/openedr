#ifndef LOG4CPLUS_CONFIGURATOR_SWG
#define LOG4CPLUS_CONFIGURATOR_SWG

%{

#include "log4cplus/configurator.h"

%}

%include "hierarchy.swg"

namespace log4cplus {

/* class PropertyConfigurator
{
public:
  enum PCFLags
  {
    fRecursiveExpansion
    , fShadowEnvironment
    , fAllowEmptyVars
    , fUnspecEncoding
#if defined (LOG4CPLUS_HAVE_CODECVT_UTF8_FACET) && defined (UNICODE)
    , fUTF8
#endif
#if (defined (LOG4CPLUS_HAVE_CODECVT_UTF16_FACET) || defined (_WIN32)) \
    && defined (UNICODE)
    , fUTF16
#endif
#if defined (LOG4CPLUS_HAVE_CODECVT_UTF32_FACET) && defined (UNICODE)
    , fUTF32
#endif   
  };
  
  PropertyConfigurator (log4cplus::tstring const & propertyFile,
    Hierarchy& h = Logger::getDefaultHierarchy(), unsigned flags = 0
}; */

class BasicConfigurator
{
public:
  BasicConfigurator (Hierarchy & h
      = log4cplus::Logger::getDefaultHierarchy (), bool logToStdErr = false);
  virtual ~BasicConfigurator ();
  
  static void doConfigure(log4cplus::Hierarchy & h
      = log4cplus::Logger::getDefaultHierarchy (),
    bool logToStdErr = false);
};
	
} // namespace log4cplus

#endif // LOG4CPLUS_CONFIGURATOR_SWG

