// Module:  Log4CPLUS
// File:    ndc.cxx
// Created: 6/2001
// Author:  Tad E. Smith
//
//
// Copyright 2001-2017 Tad E. Smith
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

#include <log4cplus/ndc.h>
#include <log4cplus/internal/internal.h>
#include <utility>
#include <algorithm>

#if defined (LOG4CPLUS_WITH_UNIT_TESTS)
#include <catch.hpp>
#endif


namespace log4cplus
{



///////////////////////////////////////////////////////////////////////////////
// log4cplus::DiagnosticContext ctors
///////////////////////////////////////////////////////////////////////////////


namespace
{


static
void
init_full_message (log4cplus::tstring & fullMessage,
    log4cplus::tstring const & message, DiagnosticContext const * parent)
{
    if (parent)
    {
        fullMessage.reserve (parent->fullMessage.size () + 1
            + message.size ());
        fullMessage = parent->fullMessage;
        fullMessage += LOG4CPLUS_TEXT(" ");
        fullMessage += message;
    }
    else
        fullMessage = message;
}


} // namespace


DiagnosticContext::DiagnosticContext(const log4cplus::tstring& message_,
                                     DiagnosticContext const * parent)
    : message(message_)
    , fullMessage()
{
    init_full_message (fullMessage, message, parent);
}


DiagnosticContext::DiagnosticContext(tchar const * message_,
                                     DiagnosticContext const * parent)
    : message(message_)
    , fullMessage()
{
    init_full_message (fullMessage, message, parent);
}


DiagnosticContext::DiagnosticContext(const log4cplus::tstring& message_)
    : message(message_)
    , fullMessage(message)
{
}


DiagnosticContext::DiagnosticContext(tchar const * message_)
    : message(message_)
    , fullMessage(message)
{
}


DiagnosticContext::DiagnosticContext (DiagnosticContext const & other)
    : message (other.message)
    , fullMessage (other.fullMessage)
{ }


DiagnosticContext & DiagnosticContext::operator = (
    DiagnosticContext const & other)
{
    DiagnosticContext (other).swap (*this);
    return *this;
}


DiagnosticContext::DiagnosticContext (DiagnosticContext && other)
    : message (std::move (other.message))
    , fullMessage (std::move (other.fullMessage))
{ }


DiagnosticContext &
DiagnosticContext::operator = (DiagnosticContext && other)
{
    DiagnosticContext (std::move (other)).swap (*this);
    return *this;
}


void
DiagnosticContext::swap (DiagnosticContext & other)
{
    using std::swap;
    swap (message, other.message);
    swap (fullMessage, other.fullMessage);
}

///////////////////////////////////////////////////////////////////////////////
// log4cplus::NDC ctor and dtor
///////////////////////////////////////////////////////////////////////////////

NDC::NDC()
{ }


NDC::~NDC()
{ }



///////////////////////////////////////////////////////////////////////////////
// log4cplus::NDC public methods
///////////////////////////////////////////////////////////////////////////////

void
NDC::clear()
{
    DiagnosticContextStack* ptr = getPtr();
    DiagnosticContextStack ().swap (*ptr);
}


void
NDC::remove()
{
    clear();
}


DiagnosticContextStack
NDC::cloneStack() const
{
    DiagnosticContextStack* ptr = getPtr();
    return DiagnosticContextStack(*ptr);
}


void
NDC::inherit(const DiagnosticContextStack& stack)
{
    DiagnosticContextStack* ptr = getPtr();
    DiagnosticContextStack (stack).swap (*ptr);
}


log4cplus::tstring const &
NDC::get() const
{
    DiagnosticContextStack* ptr = getPtr();
    if(!ptr->empty())
        return ptr->back().fullMessage;
    else
        return internal::empty_str;
}


std::size_t
NDC::getDepth() const
{
    DiagnosticContextStack* ptr = getPtr();
    return ptr->size();
}


log4cplus::tstring
NDC::pop()
{
    DiagnosticContextStack* ptr = getPtr();
    if(!ptr->empty())
    {
        tstring message;
        message.swap (ptr->back ().message);
        ptr->pop_back();
        return message;
    }
    else
        return log4cplus::tstring ();
}


void
NDC::pop_void ()
{
    DiagnosticContextStack* ptr = getPtr ();
    if (! ptr->empty ())
        ptr->pop_back ();
}


log4cplus::tstring const &
NDC::peek() const
{
    DiagnosticContextStack* ptr = getPtr();
    if(!ptr->empty())
        return ptr->back().message;
    else
        return internal::empty_str;
}


void
NDC::push(const log4cplus::tstring& message)
{
    push_worker (message);
}


void
NDC::push(tchar const * message)
{
    push_worker (message);
}


template <typename StringType>
void
NDC::push_worker (StringType const & message)
{
    DiagnosticContextStack* ptr = getPtr();
    if (ptr->empty())
        ptr->push_back( DiagnosticContext(message, nullptr) );
    else
    {
        DiagnosticContext const & dc = ptr->back();
        ptr->push_back( DiagnosticContext(message, &dc) );
    }
}


void
NDC::setMaxDepth(std::size_t maxDepth)
{
    DiagnosticContextStack* ptr = getPtr();
    while(maxDepth < ptr->size())
        ptr->pop_back();
}


DiagnosticContextStack* NDC::getPtr()
{
    internal::per_thread_data * ptd = internal::get_ptd ();
    return &ptd->ndc_dcs;
}


//
//
//

NDCContextCreator::NDCContextCreator(const log4cplus::tstring& msg)
{
    getNDC().push(msg);
}


NDCContextCreator::NDCContextCreator(tchar const * msg)
{
    getNDC().push(msg);
}


NDCContextCreator::~NDCContextCreator()
{
    getNDC().pop_void();
}


#if defined (LOG4CPLUS_WITH_UNIT_TESTS)
CATCH_TEST_CASE ("NDC", "[NDC]")
{
    NDC & ndc = getNDC ();
    ndc.clear ();
    static tchar const CONTEXT1[] = LOG4CPLUS_TEXT ("c1");
    static tchar const CONTEXT2[] = LOG4CPLUS_TEXT ("c2");
    static tchar const CONTEXT3[] = LOG4CPLUS_TEXT ("c3");
    static tstring const C1C2 = tstring (CONTEXT1)
        + LOG4CPLUS_TEXT (' ')
        + CONTEXT2;
    static tstring const C1C2C3 = C1C2
        + LOG4CPLUS_TEXT (' ')
        + CONTEXT3;

    CATCH_SECTION ("basic")
    {
        CATCH_REQUIRE (ndc.get ().empty ());
        CATCH_REQUIRE (ndc.peek ().empty ());
        CATCH_REQUIRE (ndc.getDepth () == 0);
        NDCContextCreator c1 (CONTEXT1);
        CATCH_REQUIRE (ndc.peek () == CONTEXT1);
        CATCH_REQUIRE (ndc.get () == CONTEXT1);
        CATCH_REQUIRE (ndc.getDepth () == 1);
        {
            NDCContextCreator c2 (LOG4CPLUS_C_STR_TO_TSTRING (CONTEXT2));
            CATCH_REQUIRE (ndc.get () == C1C2);
            CATCH_REQUIRE (ndc.getDepth () == 2);
            CATCH_REQUIRE (ndc.peek () == CONTEXT2);

            ndc.push (CONTEXT3);
            CATCH_REQUIRE (ndc.get () == C1C2C3);
            CATCH_REQUIRE (ndc.peek () == CONTEXT3);
            CATCH_REQUIRE (ndc.pop () == CONTEXT3);
        }
        CATCH_REQUIRE (ndc.peek () == CONTEXT1);
        CATCH_REQUIRE (ndc.get () == CONTEXT1);
        CATCH_REQUIRE (ndc.getDepth () == 1);
    }

    CATCH_SECTION ("remove")
    {
        ndc.push (CONTEXT1);
        CATCH_REQUIRE (ndc.peek () == CONTEXT1);
        CATCH_REQUIRE (ndc.get () == CONTEXT1);
        CATCH_REQUIRE (ndc.getDepth () == 1);

        ndc.remove ();
        CATCH_REQUIRE (ndc.get ().empty ());
        CATCH_REQUIRE (ndc.peek ().empty ());
        CATCH_REQUIRE (ndc.getDepth () == 0);
    }
}

#endif

} // namespace log4cplus
