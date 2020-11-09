// Module:  Log4CPLUS
// File:    global-init.cxx
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

#include <log4cplus/config.hxx>
#include <log4cplus/initializer.h>
#include <log4cplus/config/windowsh-inc.h>
#include <log4cplus/logger.h>
#include <log4cplus/ndc.h>
#include <log4cplus/mdc.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/internal/customloglevelmanager.h>
#include <log4cplus/internal/internal.h>
#include <log4cplus/thread/impl/tls.h>
#include <log4cplus/thread/syncprims-pub-impl.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/spi/factory.h>
#include <log4cplus/hierarchy.h>
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
#include "ThreadPool.h"
#endif
#if defined (LOG4CPLUS_WITH_UNIT_TESTS)
#  define CATCH_CONFIG_RUNNER
#  include <catch.hpp>
#endif
#include <cstdio>
#include <iostream>
#include <stdexcept>


// Forward Declarations
namespace log4cplus
{

#ifdef UNICODE
LOG4CPLUS_EXPORT tostream & tcout = std::wcout;
LOG4CPLUS_EXPORT tostream & tcerr = std::wcerr;

#else
LOG4CPLUS_EXPORT tostream & tcout = std::cout;
LOG4CPLUS_EXPORT tostream & tcerr = std::cerr;

#endif // UNICODE


struct InitializerImpl
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    std::mutex mtx;

    static std::once_flag flag;
#endif

    unsigned count = 0;

    static InitializerImpl * instance;
};

#if ! defined (LOG4CPLUS_SINGLE_THREADED)
std::once_flag InitializerImpl::flag;
#endif
InitializerImpl * InitializerImpl::instance;


Initializer::Initializer ()
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
        std::call_once (InitializerImpl::flag,
            [&] {
                InitializerImpl::instance = new InitializerImpl;
            });
#else
        InitializerImpl::instance = new InitializerImpl;
#endif

    LOG4CPLUS_THREADED (
        std::unique_lock<std::mutex> guard (
            InitializerImpl::instance->mtx));
    if (InitializerImpl::instance->count == 0)
        initialize ();

    ++InitializerImpl::instance->count;
}


// Forward declaration. Defined in this file.
void shutdownThreadPool();

Initializer::~Initializer ()
{
    bool destroy = false;
    {
        LOG4CPLUS_THREADED (
            std::unique_lock<std::mutex> guard (
                InitializerImpl::instance->mtx));
        --InitializerImpl::instance->count;
        if (InitializerImpl::instance->count == 0)
        {
            destroy = true;
            deinitialize ();
        }
    }
    if (destroy)
    {
        delete InitializerImpl::instance;
        InitializerImpl::instance = nullptr;
    }
}


namespace
{

//! Default context.
struct DefaultContext
{
    log4cplus::thread::Mutex console_mutex;
    helpers::LogLog loglog;
    LogLevelManager log_level_manager;
    internal::CustomLogLevelManager custom_log_level_manager;
    helpers::Time TTCCLayout_time_base;
    NDC ndc;
    MDC mdc;
    spi::AppenderFactoryRegistry appender_factory_registry;
    spi::LayoutFactoryRegistry layout_factory_registry;
    spi::FilterFactoryRegistry filter_factory_registry;
    spi::LocaleFactoryRegistry locale_factory_registry;
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    std::unique_ptr<progschj::ThreadPool> thread_pool {new progschj::ThreadPool};
#endif
    Hierarchy hierarchy;
};


enum DCState
{
    DC_UNINITIALIZED,
    DC_INITIALIZED,
    DC_DESTROYED
};


static DCState default_context_state;
static DefaultContext * default_context;


struct destroy_default_context
{
    ~destroy_default_context ()
    {
        delete default_context;
        default_context = nullptr;
        default_context_state = DC_DESTROYED;
    }
} static destroy_default_context_
LOG4CPLUS_INIT_PRIORITY (LOG4CPLUS_INIT_PRIORITY_BASE + 1);


static
void
alloc_dc ()
{
    assert (! default_context);
    assert (default_context_state == DC_UNINITIALIZED);

    if (default_context)
        throw std::logic_error (
            "alloc_dc() called with non-NULL default_context.");

    if (default_context_state == DC_INITIALIZED)
        throw std::logic_error ("alloc_dc() called in DC_INITIALIZED state.");

    default_context = new DefaultContext;

    if (default_context_state == DC_DESTROYED)
        default_context->loglog.error (
            LOG4CPLUS_TEXT ("Re-initializing default context after it has")
            LOG4CPLUS_TEXT (" already been destroyed.\n")
            LOG4CPLUS_TEXT ("The memory will be leaked."));

    default_context_state = DC_INITIALIZED;
}


static
DefaultContext *
get_dc (bool alloc = true)
{
    if (LOG4CPLUS_UNLIKELY (! default_context && alloc))
        alloc_dc ();
    return default_context;
}


} // namespace


namespace internal {

CustomLogLevelManager & getCustomLogLevelManager ()
{
    return get_dc ()->custom_log_level_manager;
}

} // namespace internal


namespace helpers
{


log4cplus::thread::Mutex const &
getConsoleOutputMutex ()
{
    return get_dc ()->console_mutex;
}


LogLog &
getLogLog ()
{
    return get_dc ()->loglog;
}


} // namespace helpers


helpers::Time const &
getTTCCLayoutTimeBase ()
{
    return get_dc ()->TTCCLayout_time_base;
}


LogLevelManager &
getLogLevelManager ()
{
    return get_dc ()->log_level_manager;
}


Hierarchy &
getDefaultHierarchy ()
{
    return get_dc ()->hierarchy;
}


NDC &
getNDC ()
{
    return get_dc ()->ndc;
}


MDC &
getMDC ()
{
    return get_dc ()->mdc;
}


#if ! defined (LOG4CPLUS_SINGLE_THREADED)
void
enqueueAsyncDoAppend (SharedAppenderPtr const & appender,
    spi::InternalLoggingEvent const & event)
{
    get_dc ()->thread_pool->enqueue (
        [=] ()
        {
            appender->asyncDoAppend (event);
        });
}

#endif

void
shutdownThreadPool ()
{
    LOG4CPLUS_THREADED (get_dc ()->thread_pool.reset ());
}


void
waitUntilEmptyThreadPoolQueue ()
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    DefaultContext * const dc = get_dc ();
    if (dc->thread_pool)
    {
        dc->thread_pool->wait_until_empty ();
        dc->thread_pool->wait_until_nothing_in_flight ();
    }
#endif
}


namespace spi
{


AppenderFactoryRegistry &
getAppenderFactoryRegistry ()
{
    return get_dc ()->appender_factory_registry;
}


LayoutFactoryRegistry &
getLayoutFactoryRegistry ()
{
    return get_dc ()->layout_factory_registry;
}


FilterFactoryRegistry &
getFilterFactoryRegistry ()
{
    return get_dc ()->filter_factory_registry;
}


LocaleFactoryRegistry &
getLocaleFactoryRegistry()
{
    return get_dc ()->locale_factory_registry;
}


} // namespace spi


namespace internal
{


gft_scratch_pad::gft_scratch_pad ()
    : uc_q_str_valid (false)
    , q_str_valid (false)
    , s_str_valid (false)
{ }


gft_scratch_pad::~gft_scratch_pad ()
{ }


appender_sratch_pad::appender_sratch_pad ()
{ }


appender_sratch_pad::~appender_sratch_pad ()
{ }


per_thread_data::per_thread_data ()
    : fnull (nullptr), macros_oss_busy(false)
{ }


per_thread_data::~per_thread_data ()
{
    if (fnull)
        std::fclose (fnull);
}


log4cplus::thread::impl::tls_key_type tls_storage_key;


#if ! defined (LOG4CPLUS_SINGLE_THREADED) \
    && defined (LOG4CPLUS_THREAD_LOCAL_VAR)

LOG4CPLUS_THREAD_LOCAL_VAR per_thread_data * ptd = nullptr;


per_thread_data *
alloc_ptd ()
{
    auto * tmp = new per_thread_data;
    set_ptd (tmp);
    // This is a special hack. We set the keys' value to non-NULL to
    // get the ptd_cleanup_func to execute when this thread ends. The
    // cast is safe; the associated value will never be used if read
    // again using the key.
    thread::impl::tls_set_value (tls_storage_key,
        reinterpret_cast<void *>(1));

    return tmp;
}

#  else

per_thread_data *
alloc_ptd ()
{
    auto * tmp = new per_thread_data;
    set_ptd (tmp);
    return tmp;
}

#  endif

} // namespace internal


void initializeFactoryRegistry ();


//! Thread local storage clean up function for POSIX threads.
#if defined (LOG4CPLUS_USE_WIN32_THREADS)
static
void NTAPI
#else
static
void
#endif
ptd_cleanup_func (void * arg)
{
    internal::per_thread_data * const arg_ptd
        = static_cast<internal::per_thread_data *>(arg);
    internal::per_thread_data * const ptd = internal::get_ptd (false);
    (void) ptd;

    // Either it is a dummy value or it should be the per thread data
    // pointer we get from internal::get_ptd().
    assert (arg == reinterpret_cast<void *>(1)
        || arg_ptd == ptd
        || (! ptd && arg_ptd));

    if (arg == reinterpret_cast<void *>(1))
        // Setting the value through the key here is necessary in case
        // we are using TLS using __thread or __declspec(thread) or
        // similar constructs with POSIX threads.  Otherwise POSIX
        // calls this cleanup routine more than once if the value
        // stays non-NULL after it returns.
        thread::impl::tls_set_value (internal::tls_storage_key, nullptr);
    else if (arg)
    {
        // Instead of using internal::get_ptd(false) here we are using
        // the value passed to this function directly.  This is
        // necessary because of the following (from SUSv4):
        //
        // A call to pthread_getspecific() for the thread-specific
        // data key being destroyed shall return the value NULL,
        // unless the value is changed (after the destructor starts)
        // by a call to pthread_setspecific().
        delete arg_ptd;
        thread::impl::tls_set_value (internal::tls_storage_key, nullptr);
    }
    else
    {
        // In this case we fall through to threadCleanup() and it does
        // all the necessary work itself.
        ;
    }

    threadCleanup ();
}


static
void
threadSetup ()
{
    internal::get_ptd (true);
}


void
initializeLog4cplus()
{
    static bool initialized = false;
    if (initialized)
        return;

    internal::tls_storage_key = thread::impl::tls_init (ptd_cleanup_func);
    threadSetup ();

    DefaultContext * dc = get_dc (true);
    dc->TTCCLayout_time_base = helpers::now ();
    Logger::getRoot();
    initializeFactoryRegistry();

    initialized = true;
}


void
initialize ()
{
    initializeLog4cplus ();
}


void
deinitialize ()
{
    shutdownThreadPool();
    Logger::shutdown ();
}


void
threadCleanup ()
{
    // Here we check that we can get CRT's heap handle because if we do not
    // then the following `delete` will fail with access violation in
    // `RtlFreeHeap()`.
    //
    // How is it possible that the CRT heap handle is NULL?
    //
    // This function can be called from TLS initializer/terminator by loader
    // when log4cplus is compiled and linked to as a static library. In case of
    // other threads temination, it should do its job and free per-thread
    // data. However, when the whole process is being terminated, it is called
    // after the CRT has been uninitialized and the CRT heap is not available
    // any more. In such case, instead of crashing, we just give up and leak
    // the memory for the short while before the process terminates anyway.
    //
    // It is possible to work around this situation in user application by
    // calling `threadCleanup()` manually before `main()` exits.
#if defined (_WIN32)
    if (_get_heap_handle() != 0)
    {
#endif
        // Do thread-specific cleanup.
        internal::per_thread_data * ptd = internal::get_ptd (false);
        delete ptd;
#if defined (_WIN32)
    }
    else
    {
        OutputDebugString (
            LOG4CPLUS_TEXT ("log4cplus: ")
            LOG4CPLUS_TEXT ("CRT heap is already gone in threadCleanup()\n"));
    }
#endif
    internal::set_ptd (nullptr);
}


void
setThreadPoolSize (std::size_t pool_size)
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    get_dc ()->thread_pool->set_pool_size (pool_size);
#endif
}


static
void
freeTLSSlot ()
{
    if (internal::tls_storage_key != thread::impl::tls_key_type ())
    {
        thread::impl::tls_cleanup(internal::tls_storage_key);
        internal::tls_storage_key = thread::impl::tls_key_type();
    }
}


#if defined (_WIN32)
static
VOID CALLBACK
initializeLog4cplusApcProc (ULONG_PTR /*dwParam*/)
{
    initializeLog4cplus ();
    threadSetup ();
}


static
void
queueLog4cplusInitializationThroughAPC ()
{
#if defined (LOG4CPLUS_BUILD_DLL)
    if (! QueueUserAPC (initializeLog4cplusApcProc, GetCurrentThread (),
        0))
        throw std::runtime_error ("QueueUserAPC() has failed");
#endif
}


static
void NTAPI
thread_callback (LPVOID /*hinstDLL*/, DWORD fdwReason, LPVOID /*lpReserved*/)
{
    // Perform actions based on the reason for calling.
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        // We cannot initialize log4cplus directly here. This is because
        // DllMain() is called under loader lock. When we are using C++11
        // threads and synchronization primitives then there is a deadlock
        // somewhere in internals of std::mutex::lock().
        queueLog4cplusInitializationThroughAPC ();
        break;
    }

    case DLL_THREAD_ATTACH:
    {
        // We could call threadSetup() here but that imposes overhead
        // on threads that do not use log4cplus. Thread local data will
        // be initialized lazily instead.
        break;
    }

    case DLL_THREAD_DETACH:
    {
        // Do thread-specific cleanup.
        log4cplus::threadCleanup ();

        break;
    }

    case DLL_PROCESS_DETACH:
    {
        // Perform any necessary cleanup.

        // Do thread-specific cleanup.
        log4cplus::threadCleanup ();
        log4cplus::freeTLSSlot();

        break;
    }

    } // switch
}


static
void NTAPI
thread_callback_initializer(LPVOID hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH
        || fdwReason == DLL_THREAD_ATTACH)
        thread_callback(hinstDLL, fdwReason, lpReserved);
}

static
void NTAPI
thread_callback_terminator(LPVOID hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_THREAD_DETACH
        || fdwReason == DLL_PROCESS_DETACH)
        thread_callback(hinstDLL, fdwReason, lpReserved);
}

#endif


#if defined (LOG4CPLUS_WITH_UNIT_TESTS)
LOG4CPLUS_EXPORT int unit_tests_main (int argc, char* argv[]);
int
unit_tests_main (int argc, char * argv[])
{
    return Catch::Session ().run (argc, argv);
}

#endif // defined (LOG4CPLUS_WITH_UNIT_TESTS)

} // namespace log4cplus


#if defined (_WIN32)
#if defined (LOG4CPLUS_BUILD_DLL) && defined (_DLL)
extern "C"
BOOL
WINAPI
DllMain (LOG4CPLUS_DLLMAIN_HINSTANCE hinstDLL, DWORD fdwReason,
    LPVOID lpReserved)
{
    log4cplus::thread_callback (hinstDLL, fdwReason, lpReserved);

    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

#elif defined (_MSC_VER) && _MSC_VER >= 1400 && defined (_DLL)
extern "C"
{

// This magic has been pieced together from several sources:
// - <http://www.nynaeve.net/?p=183>
// - <http://lists.llvm.org/pipermail/cfe-dev/2011-November/018818.html>
// - `internal_shared.h` in CRT source in Visual Studio 2015

#pragma data_seg (push, old_seg)
#ifdef _WIN64
#pragma const_seg (".CRT$XLY")
extern const
#else
#pragma data_seg (".CRT$XLY")
#endif
PIMAGE_TLS_CALLBACK log4cplus_p_thread_callback_initializer = log4cplus::thread_callback_initializer;
#pragma data_seg (pop, old_seg)

#pragma data_seg (push, old_seg)
#ifdef _WIN64
#pragma const_seg (".CRT$XLAA")
extern const
#else
#pragma data_seg (".CRT$XLAA")
#endif
PIMAGE_TLS_CALLBACK log4cplus_p_thread_callback_terminator = log4cplus::thread_callback_terminator;
#pragma data_seg (pop, old_seg)

#ifdef _WIN64
#pragma comment (linker, "/INCLUDE:_tls_used")
#pragma comment (linker, "/INCLUDE:log4cplus_p_thread_callback_initializer")
#pragma comment (linker, "/INCLUDE:log4cplus_p_thread_callback_terminator")
#else
#pragma comment (linker, "/INCLUDE:__tls_used")
#pragma comment (linker, "/INCLUDE:_log4cplus_p_thread_callback_initializer")
#pragma comment (linker, "/INCLUDE:_log4cplus_p_thread_callback_terminator")
#endif

} // extern "C"

#endif

namespace {

struct _static_log4cplus_initializer
{
    _static_log4cplus_initializer ()
    {
        // It is not possible to reliably call initializeLog4cplus() here
        // when we are using Visual Studio and C++11 threads
        // and synchronization primitives. It would result into a deadlock
        // on loader lock.
#if ! defined (_MSC_VER)
        log4cplus::initializeLog4cplus ();
#endif
    }

    ~_static_log4cplus_initializer ()
    {
        // Last thread cleanup.
        log4cplus::threadCleanup ();
        log4cplus::freeTLSSlot();
    }
} static initializer;

} // namespace


#else // defined (WIN32)
namespace {

static void
_log4cplus_initializer_func ()
    LOG4CPLUS_CONSTRUCTOR_FUNC (LOG4CPLUS_INIT_PRIORITY_BASE);
static void
_log4cplus_initializer_func ()
{
    log4cplus::initializeLog4cplus();
}

struct _static_log4cplus_initializer
{
    _static_log4cplus_initializer ()
    {
        log4cplus::initializeLog4cplus();
    }

    ~_static_log4cplus_initializer ()
    {
        // Last thread cleanup.
        log4cplus::threadCleanup ();
        log4cplus::freeTLSSlot();
    }
} static initializer
LOG4CPLUS_INIT_PRIORITY (LOG4CPLUS_INIT_PRIORITY_BASE);

} // namespace

#endif
