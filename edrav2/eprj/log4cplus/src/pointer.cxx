// Module:  Log4CPLUS
// File:    pointer.cxx
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

#include <log4cplus/streams.h>
#include <log4cplus/helpers/pointer.h>
#include <log4cplus/thread/threads.h>
#include <log4cplus/thread/impl/syncprims-impl.h>
#include <cassert>


namespace log4cplus { namespace helpers {


///////////////////////////////////////////////////////////////////////////////
// log4cplus::helpers::SharedObject dtor
///////////////////////////////////////////////////////////////////////////////

SharedObject::~SharedObject()
{
    assert(count__ == 0);
}



///////////////////////////////////////////////////////////////////////////////
// log4cplus::helpers::SharedObject public methods
///////////////////////////////////////////////////////////////////////////////

void
SharedObject::addReference() const LOG4CPLUS_NOEXCEPT
{
#if defined (LOG4CPLUS_SINGLE_THREADED)
    ++count__;

#else
    std::atomic_fetch_add_explicit (&count__, 1u,
        std::memory_order_relaxed);

#endif
}


void
SharedObject::removeReference() const
{
    assert (count__ > 0);
    bool destroy;

#if defined (LOG4CPLUS_SINGLE_THREADED)
    destroy = --count__ == 0;

#else
    destroy = std::atomic_fetch_sub_explicit (&count__, 1u,
        std::memory_order_release) == 1;
    if (LOG4CPLUS_UNLIKELY (destroy))
        std::atomic_thread_fence (std::memory_order_acquire);

#endif
    if (LOG4CPLUS_UNLIKELY (destroy))
        delete this;
}


} } // namespace log4cplus { namespace helpers
