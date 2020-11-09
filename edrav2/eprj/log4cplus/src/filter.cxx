// Module:  Log4CPLUS
// File:    filter.cxx
// Created: 5/2003
// Author:  Tad E. Smith
//
//
// Copyright 2003-2017 Tad E. Smith
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <log4cplus/spi/filter.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/spi/loggingevent.h>
#include <log4cplus/thread/syncprims-pub-impl.h>

#if defined (LOG4CPLUS_WITH_UNIT_TESTS)
#include <log4cplus/logger.h>
#include <log4cplus/ndc.h>
#include <log4cplus/mdc.h>
#include <catch.hpp>
#endif


namespace log4cplus { namespace spi {

///////////////////////////////////////////////////////////////////////////////
// global methods
///////////////////////////////////////////////////////////////////////////////

FilterResult
checkFilter(const Filter* filter, const InternalLoggingEvent& event)
{
    const Filter* currentFilter = filter;
    while(currentFilter) {
        FilterResult result = currentFilter->decide(event);
        if(result != NEUTRAL) {
            return result;
        }

        currentFilter = currentFilter->next.get();
    }

    return ACCEPT;
}



///////////////////////////////////////////////////////////////////////////////
// Filter implementation
///////////////////////////////////////////////////////////////////////////////

Filter::Filter()
{
}


Filter::~Filter()
{
}


void
Filter::appendFilter(FilterPtr filter)
{
    if (! next)
        next = filter;
    else
        next->appendFilter(filter);
}



///////////////////////////////////////////////////////////////////////////////
// DenyAllFilter implementation
///////////////////////////////////////////////////////////////////////////////

DenyAllFilter::DenyAllFilter ()
{ }


DenyAllFilter::DenyAllFilter (const helpers::Properties&)
{ }


FilterResult
DenyAllFilter::decide(const InternalLoggingEvent&) const
{
    return DENY;
}



///////////////////////////////////////////////////////////////////////////////
// LogLevelMatchFilter implementation
///////////////////////////////////////////////////////////////////////////////

LogLevelMatchFilter::LogLevelMatchFilter()
{
    init();
}



LogLevelMatchFilter::LogLevelMatchFilter(const helpers::Properties& properties)
{
    init();

    properties.getBool (acceptOnMatch, LOG4CPLUS_TEXT("AcceptOnMatch"));

    tstring const & log_level_to_match
        = properties.getProperty( LOG4CPLUS_TEXT("LogLevelToMatch") );
    logLevelToMatch = getLogLevelManager().fromString(log_level_to_match);
}


void
LogLevelMatchFilter::init()
{
    acceptOnMatch = true;
    logLevelToMatch = NOT_SET_LOG_LEVEL;
}


FilterResult
LogLevelMatchFilter::decide(const InternalLoggingEvent& event) const
{
    if(logLevelToMatch == NOT_SET_LOG_LEVEL) {
        return NEUTRAL;
    }

    bool matchOccured = (logLevelToMatch == event.getLogLevel());

    if(matchOccured) {
        return (acceptOnMatch ? ACCEPT : DENY);
    }
    else {
        return NEUTRAL;
    }
}



///////////////////////////////////////////////////////////////////////////////
// LogLevelRangeFilter implementation
///////////////////////////////////////////////////////////////////////////////

LogLevelRangeFilter::LogLevelRangeFilter()
{
    init();
}



LogLevelRangeFilter::LogLevelRangeFilter(const helpers::Properties& properties)
{
    init();

    properties.getBool (acceptOnMatch, LOG4CPLUS_TEXT("AcceptOnMatch"));

    tstring const & log_level_min
        = properties.getProperty( LOG4CPLUS_TEXT("LogLevelMin") );
    logLevelMin = getLogLevelManager().fromString(log_level_min);

    tstring const & log_level_max
        = properties.getProperty( LOG4CPLUS_TEXT("LogLevelMax") );
    logLevelMax = getLogLevelManager().fromString(log_level_max);
}


void
LogLevelRangeFilter::init()
{
    acceptOnMatch = true;
    logLevelMin = NOT_SET_LOG_LEVEL;
    logLevelMax = NOT_SET_LOG_LEVEL;
}


FilterResult
LogLevelRangeFilter::decide(const InternalLoggingEvent& event) const
{
    LogLevel const eventLogLevel = event.getLogLevel ();
    if((logLevelMin != NOT_SET_LOG_LEVEL) && (eventLogLevel < logLevelMin)) {
        // priority of event is less than minimum
        return DENY;
    }

    if((logLevelMax != NOT_SET_LOG_LEVEL) && (eventLogLevel > logLevelMax)) {
        // priority of event is greater than maximum
        return DENY;
    }

    if(acceptOnMatch) {
        // this filter set up to bypass later filters and always return
        // accept if priority in range
        return ACCEPT;
    }
    else {
        // event is ok for this filter; allow later filters to have a look...
        return NEUTRAL;
    }
}



///////////////////////////////////////////////////////////////////////////////
// StringMatchFilter implementation
///////////////////////////////////////////////////////////////////////////////

StringMatchFilter::StringMatchFilter()
{
    init();
}



StringMatchFilter::StringMatchFilter(const helpers::Properties& properties)
{
    init();

    properties.getBool (acceptOnMatch, LOG4CPLUS_TEXT("AcceptOnMatch"));
    stringToMatch = properties.getProperty( LOG4CPLUS_TEXT("StringToMatch") );
}


void
StringMatchFilter::init()
{
    acceptOnMatch = true;
}


FilterResult
StringMatchFilter::decide(const InternalLoggingEvent& event) const
{
    const tstring& message = event.getMessage();

    if(stringToMatch.empty () || message.empty ()) {
        return NEUTRAL;
    }

    if(message.find(stringToMatch) == tstring::npos) {
        return NEUTRAL;
    }
    else {  // we've got a match
        return (acceptOnMatch ? ACCEPT : DENY);
    }
}


//
//
//

FunctionFilter::FunctionFilter (FunctionFilter::Function f)
    : function (std::move (f))
{ }


FilterResult
FunctionFilter::decide(const InternalLoggingEvent& event) const
{
    return function (event);
}

//
// NDC Match filter
//
NDCMatchFilter::NDCMatchFilter()
{
    init();
}

NDCMatchFilter::NDCMatchFilter(const helpers::Properties& properties)
{
    init();

    properties.getBool (acceptOnMatch,LOG4CPLUS_TEXT("AcceptOnMatch"));
    properties.getBool (neutralOnEmpty,LOG4CPLUS_TEXT("NeutralOnEmpty"));
    ndcToMatch = properties.getProperty(LOG4CPLUS_TEXT("NDCToMatch"));
}


void NDCMatchFilter::init()
{
    acceptOnMatch = true;
    neutralOnEmpty = true;
}


FilterResult NDCMatchFilter::decide(const InternalLoggingEvent& event) const
{
    const tstring& ndcStr = event.getNDC();

    if(neutralOnEmpty && (ndcToMatch.empty () || ndcStr.empty()))
    {
        return NEUTRAL;
    }

    if(ndcStr.compare(ndcToMatch) == 0)
        return (acceptOnMatch ? ACCEPT : DENY);

    return (acceptOnMatch ? DENY : ACCEPT);
}

//
// MDC Match filter
//
MDCMatchFilter::MDCMatchFilter()
{
    init();
}

MDCMatchFilter::MDCMatchFilter(const helpers::Properties& properties)
{
    init();

    properties.getBool (acceptOnMatch,LOG4CPLUS_TEXT("AcceptOnMatch"));
    properties.getBool (neutralOnEmpty,LOG4CPLUS_TEXT("NeutralOnEmpty"));
    mdcValueToMatch = properties.getProperty(LOG4CPLUS_TEXT("MDCValueToMatch"));
    mdcKeyToMatch = properties.getProperty(LOG4CPLUS_TEXT("MDCKeyToMatch"));
}


void MDCMatchFilter::init()
{
    acceptOnMatch = true;
    neutralOnEmpty = true;
}


FilterResult MDCMatchFilter::decide(const InternalLoggingEvent& event) const
{
    if(neutralOnEmpty && (mdcKeyToMatch.empty() || mdcValueToMatch.empty()))
        return NEUTRAL;

    const tstring mdcStr = event.getMDC(mdcKeyToMatch);

    if(neutralOnEmpty && mdcStr.empty())
        return NEUTRAL;

    if(mdcStr.compare(mdcValueToMatch) == 0)
        return (acceptOnMatch ? ACCEPT : DENY);

    return (acceptOnMatch ? DENY : ACCEPT);
}


#if defined (LOG4CPLUS_WITH_UNIT_TESTS)
CATCH_TEST_CASE ("Filter", "[filter]")
{
    FilterPtr filter;
    Logger log (Logger::getInstance (LOG4CPLUS_TEXT ("test")));
    static InternalLoggingEvent const debug_ev (log.getName (), DEBUG_LOG_LEVEL,
        LOG4CPLUS_C_STR_TO_TSTRING (LOG4CPLUS_TEXT ("debug log message")),
        __FILE__, __LINE__);
    static InternalLoggingEvent const info_ev (log.getName (), INFO_LOG_LEVEL,
        LOG4CPLUS_C_STR_TO_TSTRING (LOG4CPLUS_TEXT ("info log message")),
        __FILE__, __LINE__);
    static InternalLoggingEvent const empty_ev (log.getName (), INFO_LOG_LEVEL,
        LOG4CPLUS_C_STR_TO_TSTRING (LOG4CPLUS_TEXT ("")),
        __FILE__, __LINE__);
    static InternalLoggingEvent const warn_ev (log.getName (), WARN_LOG_LEVEL,
        LOG4CPLUS_C_STR_TO_TSTRING (LOG4CPLUS_TEXT ("warn log message")),
        __FILE__, __LINE__);
    static InternalLoggingEvent const error_ev (log.getName (), ERROR_LOG_LEVEL,
        LOG4CPLUS_C_STR_TO_TSTRING (LOG4CPLUS_TEXT ("error log message")),
        __FILE__, __LINE__);
    static InternalLoggingEvent const fatal_ev (log.getName (), FATAL_LOG_LEVEL,
        LOG4CPLUS_C_STR_TO_TSTRING (LOG4CPLUS_TEXT ("fatal log message")),
        __FILE__, __LINE__);

    CATCH_SECTION ("deny all filter")
    {
        filter = new DenyAllFilter;
        CATCH_REQUIRE (filter->decide (info_ev) == DENY);
        CATCH_REQUIRE (checkFilter (filter.get (), info_ev) == DENY);
    }

    CATCH_SECTION ("log level match filter")
    {
        CATCH_SECTION ("accept level")
        {
            helpers::Properties props;
            props.setProperty (LOG4CPLUS_TEXT ("LogLevelToMatch"),
                LOG4CPLUS_TEXT ("INFO"));
            filter = new LogLevelMatchFilter (props);
            CATCH_REQUIRE (filter->decide (info_ev) == ACCEPT);
            CATCH_REQUIRE (filter->decide (error_ev) == NEUTRAL);
        }

        CATCH_SECTION ("deny level")
        {
            helpers::Properties props;
            props.setProperty (LOG4CPLUS_TEXT ("LogLevelToMatch"),
                LOG4CPLUS_TEXT ("INFO"));
            props.setProperty (LOG4CPLUS_TEXT ("AcceptOnMatch"),
                LOG4CPLUS_TEXT ("false"));
            filter = new LogLevelMatchFilter (props);
            CATCH_REQUIRE (filter->decide (info_ev) == DENY);
            CATCH_REQUIRE (filter->decide (error_ev) == NEUTRAL);
        }
    }

    CATCH_SECTION ("log level range filter")
    {
        CATCH_SECTION ("accept in range")
        {
            helpers::Properties props;
            props.setProperty (LOG4CPLUS_TEXT ("LogLevelMin"),
                LOG4CPLUS_TEXT ("WARN"));
            props.setProperty (LOG4CPLUS_TEXT ("LogLevelMax"),
                LOG4CPLUS_TEXT ("ERROR"));
            filter = new LogLevelRangeFilter (props);
            CATCH_REQUIRE (filter->decide (info_ev) == DENY);
            CATCH_REQUIRE (filter->decide (warn_ev) == ACCEPT);
            CATCH_REQUIRE (filter->decide (error_ev) == ACCEPT);
            CATCH_REQUIRE (filter->decide (fatal_ev) == DENY);
        }

        CATCH_SECTION ("deny out of range")
        {
            helpers::Properties props;
            props.setProperty (LOG4CPLUS_TEXT ("LogLevelMin"),
                LOG4CPLUS_TEXT ("WARN"));
            props.setProperty (LOG4CPLUS_TEXT ("LogLevelMax"),
                LOG4CPLUS_TEXT ("ERROR"));
            props.setProperty (LOG4CPLUS_TEXT ("AcceptOnMatch"),
                LOG4CPLUS_TEXT ("false"));
            filter = new LogLevelRangeFilter (props);
            CATCH_REQUIRE (filter->decide (info_ev) == DENY);
            CATCH_REQUIRE (filter->decide (warn_ev) == NEUTRAL);
            CATCH_REQUIRE (filter->decide (error_ev) == NEUTRAL);
            CATCH_REQUIRE (filter->decide (fatal_ev) == DENY);
        }
    }

    CATCH_SECTION ("string match filter")
    {
        CATCH_SECTION ("empty string to match is neutral")
        {
            filter = new StringMatchFilter;
            CATCH_REQUIRE (filter->decide (info_ev) == NEUTRAL);
            CATCH_REQUIRE (filter->decide (error_ev) == NEUTRAL);
        }

        CATCH_SECTION ("not found is neutral")
        {
            helpers::Properties props;
            props.setProperty (LOG4CPLUS_TEXT ("StringToMatch"),
                LOG4CPLUS_TEXT ("nonexistent"));
            filter = new StringMatchFilter (props);
            CATCH_REQUIRE (filter->decide (info_ev) == NEUTRAL);
            CATCH_REQUIRE (filter->decide (error_ev) == NEUTRAL);
        }

        CATCH_SECTION ("empty event is neutral")
        {
            helpers::Properties props;
            props.setProperty (LOG4CPLUS_TEXT ("StringToMatch"),
                LOG4CPLUS_TEXT ("message"));
            filter = new StringMatchFilter (props);
            CATCH_REQUIRE (filter->decide (empty_ev) == NEUTRAL);
        }

        CATCH_SECTION ("deny on match")
        {
            helpers::Properties props;
            props.setProperty (LOG4CPLUS_TEXT ("StringToMatch"),
                LOG4CPLUS_TEXT ("message"));
            props.setProperty (LOG4CPLUS_TEXT ("AcceptOnMatch"),
                LOG4CPLUS_TEXT ("false"));
            filter = new StringMatchFilter (props);
            CATCH_REQUIRE (filter->decide (empty_ev) == NEUTRAL);
            CATCH_REQUIRE (filter->decide (info_ev) == DENY);
            CATCH_REQUIRE (filter->decide (warn_ev) == DENY);
        }
    }

    CATCH_SECTION ("function filter")
    {
        filter = new FunctionFilter (
            [](InternalLoggingEvent const & ev) -> FilterResult {
                return ev.getLogLevel () >= INFO_LOG_LEVEL ? ACCEPT : DENY;
            });
        CATCH_REQUIRE (filter->decide (info_ev) == ACCEPT);
        CATCH_REQUIRE (filter->decide (debug_ev) == DENY);
    }


    CATCH_SECTION ("ndc match filter")
    {
        InternalLoggingEvent  ndc_error_ev (log.getName (), ERROR_LOG_LEVEL,
                LOG4CPLUS_C_STR_TO_TSTRING (LOG4CPLUS_TEXT ("NDC error log message")), __FILE__, __LINE__);

        CATCH_SECTION ("NeutralOnEmpty is true")
        {
            CATCH_SECTION ("string to match is empty, is neutral")
            {
                filter = new NDCMatchFilter;
                CATCH_REQUIRE (filter->decide (ndc_error_ev) == NEUTRAL);
            }

            CATCH_SECTION ("ndc string empty, is neutral")
            {
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("NDCToMatch"),
                    LOG4CPLUS_TEXT ("ndc-match"));
                filter = new NDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (ndc_error_ev) == NEUTRAL);
            }

            CATCH_SECTION ("ndc string match, is accept")
            {
                log4cplus::NDC().push(LOG4CPLUS_TEXT ("ndc-match"));
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("NDCToMatch"),
                    LOG4CPLUS_TEXT ("ndc-match"));
                filter = new NDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (ndc_error_ev) == ACCEPT);
                log4cplus::NDC().pop_void();
            }

            CATCH_SECTION ("ndc string mismatch, is deny")
            {
                log4cplus::NDC().push(LOG4CPLUS_TEXT ("ndc-match"));
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("NDCToMatch"),
                    LOG4CPLUS_TEXT ("no-match"));
                filter = new NDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (ndc_error_ev) == DENY);
                log4cplus::NDC().pop_void();
            }


            CATCH_SECTION ("ndc string match, AcceptOnMatch false, is deny")
            {
                log4cplus::NDC().push(LOG4CPLUS_TEXT ("ndc-match"));
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("NDCToMatch"),
                    LOG4CPLUS_TEXT ("ndc-match"));
                props.setProperty (LOG4CPLUS_TEXT ("AcceptOnMatch"),
                    LOG4CPLUS_TEXT ("False"));
                filter = new NDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (ndc_error_ev) == DENY);
                log4cplus::NDC().pop_void();
            }

            CATCH_SECTION ("ndc string mismatch, AcceptOnMatch false, is accept")
            {
                log4cplus::NDC().push(LOG4CPLUS_TEXT ("ndc-match"));
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("NDCToMatch"),
                    LOG4CPLUS_TEXT ("no-match"));
                props.setProperty (LOG4CPLUS_TEXT ("AcceptOnMatch"),
                    LOG4CPLUS_TEXT ("False"));
                filter = new NDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (ndc_error_ev) == ACCEPT);
                log4cplus::NDC().pop_void();
            }
        }

        CATCH_SECTION ("NeutralOnEmpty is false")
        {
            CATCH_SECTION ("ndc string empty, ndc to match empty is accept")
            {
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("NeutralOnEmpty"),
                    LOG4CPLUS_TEXT ("False"));
                filter = new NDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (ndc_error_ev) == ACCEPT);
            }

            CATCH_SECTION ("ndc string empty, match not empty is deny")
            {
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("NeutralOnEmpty"),
                    LOG4CPLUS_TEXT ("False"));
                props.setProperty (LOG4CPLUS_TEXT ("NDCToMatch"),
                    LOG4CPLUS_TEXT ("ndc-match"));
                filter = new NDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (ndc_error_ev) == DENY);
            }

            CATCH_SECTION ("ndc string no empty, match empty is deny")
            {
                log4cplus::NDC().push(LOG4CPLUS_TEXT ("ndc-match"));
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("NeutralOnEmpty"),
                    LOG4CPLUS_TEXT ("False"));
                filter = new NDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (ndc_error_ev) == DENY);
                log4cplus::NDC().pop_void();
            }
        }
    }

    CATCH_SECTION ("mdc match filter")
    {
        InternalLoggingEvent  mdc_error_ev (log.getName (), ERROR_LOG_LEVEL,
                LOG4CPLUS_C_STR_TO_TSTRING (LOG4CPLUS_TEXT ("MDC error log message")), __FILE__, __LINE__);

        CATCH_SECTION ("NeutralOnEmpty is true")
        {
            CATCH_SECTION ("MDCKeyToMatch is empty, is neutral")
            {
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("MDCValueToMatch"),
                    LOG4CPLUS_TEXT ("mdc-match"));
                filter = new MDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (mdc_error_ev) == NEUTRAL);
            }

            CATCH_SECTION ("MDCValueToMatch empty, is neutral")
            {
                log4cplus::MDC().put(LOG4CPLUS_TEXT ("KeyToMatch"), LOG4CPLUS_TEXT ("mdc-match"));
                filter = new MDCMatchFilter;
                CATCH_REQUIRE (filter->decide (mdc_error_ev) == NEUTRAL);
                log4cplus::MDC().clear();
            }

            CATCH_SECTION ("MDC Key/Values match, is accept")
            {
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("MDCValueToMatch"),
                    LOG4CPLUS_TEXT ("mdc-match"));
                props.setProperty (LOG4CPLUS_TEXT ("MDCKeyToMatch"),
                    LOG4CPLUS_TEXT ("KeyToMatch"));
                log4cplus::MDC().put(LOG4CPLUS_TEXT ("KeyToMatch"), LOG4CPLUS_TEXT ("mdc-match"));
                filter = new MDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (mdc_error_ev) == ACCEPT);
                log4cplus::MDC().clear();
            }

            CATCH_SECTION ("MDC Values mismatch, is deny")
            {
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("MDCValueToMatch"),
                    LOG4CPLUS_TEXT ("mdc-match"));
                props.setProperty (LOG4CPLUS_TEXT ("MDCKeyToMatch"),
                    LOG4CPLUS_TEXT ("KeyToMatch"));
                log4cplus::MDC().put(LOG4CPLUS_TEXT ("KeyToMatch"), LOG4CPLUS_TEXT ("mdc-no-match"));
                filter = new MDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (mdc_error_ev) == DENY);
                log4cplus::MDC().clear();
            }

            CATCH_SECTION ("AcceptOnMatch is false, MDC Key/Values match, is deny")
            {
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("AcceptOnMatch"),
                    LOG4CPLUS_TEXT ("False"));
                props.setProperty (LOG4CPLUS_TEXT ("MDCValueToMatch"),
                    LOG4CPLUS_TEXT ("mdc-match"));
                props.setProperty (LOG4CPLUS_TEXT ("MDCKeyToMatch"),
                    LOG4CPLUS_TEXT ("KeyToMatch"));
                log4cplus::MDC().put(LOG4CPLUS_TEXT ("KeyToMatch"), LOG4CPLUS_TEXT ("mdc-match"));
                filter = new MDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (mdc_error_ev) == DENY);
                log4cplus::MDC().clear();
            }

            CATCH_SECTION ("AcceptOnmatch is false MDC Values mismatch, is accept")
            {
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("AcceptOnMatch"),
                    LOG4CPLUS_TEXT ("False"));
                props.setProperty (LOG4CPLUS_TEXT ("MDCValueToMatch"),
                    LOG4CPLUS_TEXT ("mdc-match"));
                props.setProperty (LOG4CPLUS_TEXT ("MDCKeyToMatch"),
                    LOG4CPLUS_TEXT ("KeyToMatch"));
                log4cplus::MDC().put(LOG4CPLUS_TEXT ("KeyToMatch"), LOG4CPLUS_TEXT ("mdc-no-match"));
                filter = new MDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (mdc_error_ev) == ACCEPT);
                log4cplus::MDC().clear();
            }
        }

        CATCH_SECTION ("NeutralOnEmpty is false")
        {
            CATCH_SECTION ("mdc key/value empty, MDC value to match empty is accept")
            {
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("NeutralOnEmpty"),
                    LOG4CPLUS_TEXT ("False"));
                filter = new MDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (mdc_error_ev) == ACCEPT);
            }

            CATCH_SECTION ("mdc key/value empty, MDC value to match not empty is deny")
            {
                helpers::Properties props;
                props.setProperty (LOG4CPLUS_TEXT ("NeutralOnEmpty"),
                    LOG4CPLUS_TEXT ("False"));
                props.setProperty (LOG4CPLUS_TEXT ("MDCValueToMatch"),
                    LOG4CPLUS_TEXT ("mdc-match"));
                filter = new MDCMatchFilter(props);
                CATCH_REQUIRE (filter->decide (mdc_error_ev) == DENY);
            }
        }
    }
}

#endif

} } // namespace log4cplus { namespace spi {
