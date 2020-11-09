
#include "customloglevel.h"

static log4cplus::tstring const CRITICAL_STRING (LOG4CPLUS_TEXT("CRITICAL"));
static log4cplus::tstring const empty_str;


static
tstring const &
criticalToStringMethod(LogLevel ll)
{
    if(ll == CRITICAL_LOG_LEVEL) {
        return CRITICAL_STRING;
    }
    else {
        return empty_str;
    }
}


static
LogLevel
criticalFromStringMethod(const tstring& s) 
{
    if(s == CRITICAL_STRING) return CRITICAL_LOG_LEVEL;

    return NOT_SET_LOG_LEVEL;
}



class CriticalLogLevelInitializer {
public:
    CriticalLogLevelInitializer() {
        getLogLevelManager().pushToStringMethod(criticalToStringMethod);
        getLogLevelManager().pushFromStringMethod(criticalFromStringMethod);
    }
};

CriticalLogLevelInitializer criticalLogLevelInitializer_;

