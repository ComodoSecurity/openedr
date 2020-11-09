// -*- C++ -*-
// Module:  Log4CPLUS
// File:    filter.h
// Created: 5/2003
// Author:  Tad E. Smith
//
//
// Copyright 1999-2017 Tad E. Smith
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

/** @file
 * This header defines Filter and all of it's subclasses. */

#ifndef LOG4CPLUS_SPI_FILTER_HEADER_
#define LOG4CPLUS_SPI_FILTER_HEADER_

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#include <functional>

#include <log4cplus/helpers/pointer.h>
#include <log4cplus/loglevel.h>


namespace log4cplus {

    namespace helpers
    {

        class Properties;

    }

    namespace spi {


        enum FilterResult { DENY, /**< The log event must be dropped immediately
                                   *  without consulting with the remaining
                                   *  filters, if any, in the chain. */
                            NEUTRAL, /**< This filter is neutral with respect to
                                      *  the log event; the remaining filters, if
                                      *  if any, should be consulted for a final
                                      *  decision. */
                            ACCEPT /**< The log event must be logged immediately
                                    *  without consulting with the remaining
                                    *  filters, if any, in the chain. */
                          };

        // Forward Declarations
        class Filter;
        class InternalLoggingEvent;


        /**
         * This method is used to filter an InternalLoggingEvent.
         *
         * Note: <code>filter</code> can be NULL.
         */
        LOG4CPLUS_EXPORT FilterResult checkFilter(const Filter* filter,
                                                  const InternalLoggingEvent& event);

        typedef helpers::SharedObjectPtr<Filter> FilterPtr;


        /**
         * Users should extend this class to implement customized logging
         * event filtering. Note that the {@link Logger} and {@link
         * Appender} classes have built-in filtering rules. It is suggested
         * that you first use and understand the built-in rules before rushing
         * to write your own custom filters.
         *
         * This abstract class assumes and also imposes that filters be
         * organized in a linear chain. The {@link #decide
         * decide(LoggingEvent)} method of each filter is called sequentially,
         * in the order of their addition to the chain.
         *
         * If the value {@link #DENY} is returned, then the log event is
         * dropped immediately without consulting with the remaining
         * filters.
         *
         * If the value {@link #NEUTRAL} is returned, then the next filter
         * in the chain is consulted. If there are no more filters in the
         * chain, then the log event is logged. Thus, in the presence of no
         * filters, the default behaviour is to log all logging events.
         *
         * If the value {@link #ACCEPT} is returned, then the log
         * event is logged without consulting the remaining filters.
         *
         * The philosophy of log4cplus filters is largely inspired from the
         * Linux ipchains.
         */
        class LOG4CPLUS_EXPORT Filter
            : public virtual log4cplus::helpers::SharedObject
        {
        public:
          // ctor and dtor
            Filter();
            virtual ~Filter();

          // Methods
            /**
             * Appends <code>filter</code> to the end of this filter chain.
             */
            void appendFilter(FilterPtr filter);

            /**
             * If the decision is <code>DENY</code>, then the event will be
             * dropped. If the decision is <code>NEUTRAL</code>, then the next
             * filter, if any, will be invoked. If the decision is ACCEPT then
             * the event will be logged without consulting with other filters in
             * the chain.
             *
             * @param event The LoggingEvent to decide upon.
             * @return The decision of the filter.
             */
            virtual FilterResult decide(const InternalLoggingEvent& event) const = 0;

          // Data
            /**
             * Points to the next filter in the filter chain.
             */
            FilterPtr next;
        };



        /**
         * This filter drops all logging events.
         *
         * You can add this filter to the end of a filter chain to
         * switch from the default "accept all unless instructed otherwise"
         * filtering behaviour to a "deny all unless instructed otherwise"
         * behaviour.
         */
        class LOG4CPLUS_EXPORT DenyAllFilter : public Filter {
        public:
            DenyAllFilter ();
            DenyAllFilter (const log4cplus::helpers::Properties&);

            /**
             * Always returns the {@link #DENY} regardless of the
             * {@link InternalLoggingEvent} parameter.
             */
            virtual FilterResult decide(const InternalLoggingEvent& event) const;
        };


        /**
         * This is a very simple filter based on LogLevel matching.
         *
         * The filter admits two options <b>LogLevelToMatch</b> and
         * <b>AcceptOnMatch</b>. If there is an exact match between the value
         * of the LogLevelToMatch option and the LogLevel of the {@link
         * spi::InternalLoggingEvent}, then the {@link #decide} method returns
         * {@link #ACCEPT} in case the <b>AcceptOnMatch</b> option value is set
         * to <code>true</code>, if it is <code>false</code> then {@link #DENY}
         * is returned. If there is no match, {@link #NEUTRAL} is returned.
         */
        class LOG4CPLUS_EXPORT LogLevelMatchFilter : public Filter {
        public:
            LogLevelMatchFilter();
            LogLevelMatchFilter(const log4cplus::helpers::Properties& p);

            /**
             * Return the decision of this filter.
             *
             * Returns {@link #NEUTRAL} if the <b>LogLevelToMatch</b>
             * option is not set or if there is no match.  Otherwise, if
             * there is a match, then the returned decision is {@link #ACCEPT}
             * if the <b>AcceptOnMatch</b> property is set to <code>true</code>.
             * The returned decision is {@link #DENY} if the <b>AcceptOnMatch</b>
             * property is set to <code>false</code>.
             */
            virtual FilterResult decide(const InternalLoggingEvent& event) const;

        private:
          // Methods
            LOG4CPLUS_PRIVATE void init();

          // Data
            /** Do we return ACCEPT when a match occurs. Default is <code>true</code>. */
            bool acceptOnMatch;
            LogLevel logLevelToMatch;
        };



        /**
         * This is a very simple filter based on LogLevel matching, which can be
         * used to reject messages with LogLevels outside a certain range.
         *
         * The filter admits three options <b>LogLevelMin</b>, <b>LogLevelMax</b>
         * and <b>AcceptOnMatch</b>.
         *
         * If the LogLevel of the Logging event is not between Min and Max
         * (inclusive), then {@link #DENY} is returned.
         *
         * If the Logging event LogLevel is within the specified range, then if
         * <b>AcceptOnMatch</b> is true, {@link #ACCEPT} is returned, and if
         * <b>AcceptOnMatch</b> is false, {@link #NEUTRAL} is returned.
         *
         * If <code>LogLevelMin</code> is not defined, then there is no
         * minimum acceptable LogLevel (ie a LogLevel is never rejected for
         * being too "low"/unimportant).  If <code>LogLevelMax</code> is not
         * defined, then there is no maximum acceptable LogLevel (ie a
         * LogLevel is never rejected for beeing too "high"/important).
         *
         * Refer to the {@link
         * Appender#setThreshold setThreshold} method
         * available to <code>all</code> appenders for a more convenient way to
         * filter out events by LogLevel.
         */
        class LOG4CPLUS_EXPORT LogLevelRangeFilter : public Filter {
        public:
          // ctors
            LogLevelRangeFilter();
            LogLevelRangeFilter(const log4cplus::helpers::Properties& p);

            /**
             * Return the decision of this filter.
             */
            virtual FilterResult decide(const InternalLoggingEvent& event) const;

        private:
          // Methods
            LOG4CPLUS_PRIVATE void init();

          // Data
            /** Do we return ACCEPT when a match occurs. Default is <code>true</code>. */
            bool acceptOnMatch;
            LogLevel logLevelMin;
            LogLevel logLevelMax;
        };



        /**
         * This is a very simple filter based on string matching.
         *
         * The filter admits two options <b>StringToMatch</b> and
         * <b>AcceptOnMatch</b>. If there is a match between the value of the
         * StringToMatch option and the message of the Logging event,
         * then the {@link #decide} method returns {@link #ACCEPT} if
         * the <b>AcceptOnMatch</b> option value is true, if it is false then
         * {@link #DENY} is returned. If there is no match, {@link #NEUTRAL}
         * is returned.
         */
        class LOG4CPLUS_EXPORT StringMatchFilter : public Filter {
        public:
          // ctors
            StringMatchFilter();
            StringMatchFilter(const log4cplus::helpers::Properties& p);

            /**
             * Returns {@link #NEUTRAL} is there is no string match.
             */
            virtual FilterResult decide(const InternalLoggingEvent& event) const;

        private:
          // Methods
            LOG4CPLUS_PRIVATE void init();

          // Data
            /** Do we return ACCEPT when a match occurs. Default is <code>true</code>. */
            bool acceptOnMatch;
            log4cplus::tstring stringToMatch;
        };

        /**
         * This filter allows using `std::function<FilterResult(const
         * InternalLoggingEvent &)>`.
         */
        class LOG4CPLUS_EXPORT FunctionFilter
            : public Filter
        {
        public:
            typedef std::function<FilterResult (const InternalLoggingEvent &)>
                Function;

            FunctionFilter (Function);

            /**
             * Returns result returned by `function`.
             */
            virtual FilterResult decide(const InternalLoggingEvent&) const;

        private:
            Function function;
        };

        /**
         * This is a simple filter based on the string returned by event.getNDC().
         *
         * The filter admits three options <b>NeutralOnEmpty</b>, <b>NDCToMatch</b>
         * and <b>AcceptOnMatch</b>.
         *
         * If <code>NeutralOnEmpty</code> is true and <code>NDCToMatch</code> is empty
         * then {@link #NEUTRAL} is returned.
         *
         * If <code>NeutralOnEmpty</code> is true and the value returned by event.getNDC() is empty
         * then {@link #NEUTRAL} is returned.
         *
         * If the string returned by event.getNDC() matches <code>NDCToMatch</code>, then if
         * <b>AcceptOnMatch</b> is true, {@link #ACCEPT} is returned, and if
         * <b>AcceptOnMatch</b> is false, {@link #DENY} is returned.
         *
         * If the string returned by event.getNDC() does not match <code>NDCToMatch</code>, then if
         * <b>AcceptOnMatch</b> is true, {@link #DENY} is returned, and if
         * <b>AcceptOnMatch</b> is false, {@link #ACCEPT} is returned.
         *
         */

        class LOG4CPLUS_EXPORT NDCMatchFilter : public Filter
        {
            public:
              // ctors
                NDCMatchFilter();
                NDCMatchFilter(const log4cplus::helpers::Properties& p);

                /**
                 * Returns {@link #NEUTRAL} is there is no string match.
                 */
                virtual FilterResult decide(const InternalLoggingEvent& event) const;

            private:
              // Methods
                LOG4CPLUS_PRIVATE void init();

              // Data
                /** Do we return ACCEPT when a match occurs. Default is <code>true</code>. */
                bool acceptOnMatch;
                /** return NEUTRAL if event.getNDC() is empty or ndcToMatch is empty. Default is <code>true</code>. */
                bool neutralOnEmpty;
                log4cplus::tstring ndcToMatch;
        };

        /**
         * This is a simple filter based on the key/value pair stored in MDC.
         *
         * The filter admits four options <b>NeutralOnEmpty</b>, <b>MDCKeyToMatch</b>
         * <b>MDCValueToMatch</b> and <b>AcceptOnMatch</b>.
         *
         * If <code>NeutralOnEmpty</code> is true and <code>MDCKeyToMatch</code> or <code>MDCValueToMatch</code>
         * is empty then {@link #NEUTRAL} is returned.
         *
         * If <code>NeutralOnEmpty</code> is true and the string returned by event.getMDC(MDCKeyToMatch) is empty
         * then {@link #NEUTRAL} is returned.
         *
         * If the string returned by event.getMDC(MDCKeyToMatch) matches <code>MDCValueToMatch</code>, then if
         * <b>AcceptOnMatch</b> is true, {@link #ACCEPT} is returned, and if
         * <b>AcceptOnMatch</b> is false, {@link #DENY} is returned.
         *
         * If the string returned by event.getMDC(MDCKeyToMatch) does not match <code>MDCValueToMatch</code>, then if
         * <b>AcceptOnMatch</b> is true, {@link #DENY} is returned, and if
         * <b>AcceptOnMatch</b> is false, {@link #ACCEPT} is returned.
         *
         */

        class LOG4CPLUS_EXPORT MDCMatchFilter : public Filter
        {
            public:
              // ctors
                MDCMatchFilter();
                MDCMatchFilter(const log4cplus::helpers::Properties& p);

                /**
                 * Returns {@link #NEUTRAL} is there is no string match.
                 */
                virtual FilterResult decide(const InternalLoggingEvent& event) const;

            private:
              // Methods
                LOG4CPLUS_PRIVATE void init();

              // Data
                /** Do we return ACCEPT when a match occurs. Default is <code>true</code>. */
                bool acceptOnMatch;
                /** return NEUTRAL if mdcKeyToMatch is empty or event::getMDC(mdcKeyValue) is empty or mdcValueToMatch is empty. Default is <code>true</code>. */
                bool neutralOnEmpty;
                /** The MDC key to retrieve **/
                log4cplus::tstring mdcKeyToMatch;
                /** the MDC value to match **/
                log4cplus::tstring mdcValueToMatch;
        };

    } // end namespace spi
} // end namespace log4cplus

#endif /* LOG4CPLUS_SPI_FILTER_HEADER_ */
