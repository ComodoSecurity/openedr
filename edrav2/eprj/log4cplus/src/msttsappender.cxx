// Module:  Log4cplus
// File:    msttsappender.cxx
// Created: 10/2012
// Author:  Vaclav Zeman
//
//
//  Copyright (C) 2012-2017, Vaclav Zeman. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modifica-
//  tion, are permitted provided that the following conditions are met:
//
//  1. Redistributions of  source code must  retain the above copyright  notice,
//     this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
//  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//  FITNESS  FOR A PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN NO  EVENT SHALL  THE
//  APACHE SOFTWARE  FOUNDATION  OR ITS CONTRIBUTORS  BE LIABLE FOR  ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLU-
//  DING, BUT NOT LIMITED TO, PROCUREMENT  OF SUBSTITUTE GOODS OR SERVICES; LOSS
//  OF USE, DATA, OR  PROFITS; OR BUSINESS  INTERRUPTION)  HOWEVER CAUSED AND ON
//  ANY  THEORY OF LIABILITY,  WHETHER  IN CONTRACT,  STRICT LIABILITY,  OR TORT
//  (INCLUDING  NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT OF THE  USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <log4cplus/config.hxx>
#include <log4cplus/msttsappender.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/spi/factory.h>
#include <log4cplus/thread/threads.h>
#include <log4cplus/thread/syncprims.h>

#include <iomanip>
#include <sstream>

#include <sapi.h>


namespace log4cplus
{


namespace
{

static
void
loglog_win32_error (tchar const * msg, bool dothrow = false)
{
    DWORD err = GetLastError ();
    tostringstream oss;
    oss << LOG4CPLUS_TEXT ("MSTTSAppender: ") << msg << LOG4CPLUS_TEXT(": ")
        << err << LOG4CPLUS_TEXT (" / 0x")
        << std::setw (8) << std::setfill (LOG4CPLUS_TEXT ('0')) << std::hex
        << err;
    helpers::getLogLog ().error (oss.str (), dothrow);
}


static
void
loglog_com_error (tchar const * msg, HRESULT hr)
{
    tostringstream oss;
    oss << LOG4CPLUS_TEXT ("MSTTSAppender: ") << msg << LOG4CPLUS_TEXT(": ")
        << hr << LOG4CPLUS_TEXT (" / 0x")
        << std::setw (8) << std::setfill (LOG4CPLUS_TEXT ('0')) << std::hex
        << hr;
    helpers::getLogLog ().error (oss.str ());
}


struct COMInitializer
{
    explicit COMInitializer (DWORD apartment_model = COINIT_APARTMENTTHREADED,
        HRESULT hr = S_OK)
        : uninit ((hr = CoInitializeEx (nullptr, apartment_model)
            , hr == S_OK || hr == S_FALSE))
    { }

    ~COMInitializer ()
    {
        if (uninit)
            CoUninitialize ();
    }

    bool const uninit;
};


class SpeechObjectThread
    : public virtual log4cplus::thread::AbstractThread
{
public:
    SpeechObjectThread (ISpVoice * & ispvoice_ref)
        : ispvoice (ispvoice_ref)
    {
        terminate_ev = CreateEvent (0, true, false, 0);
        if (! terminate_ev)
            loglog_win32_error (
                LOG4CPLUS_TEXT ("SpeechObjectThread: CreateEvent failed"),
                true);
    }


    ~SpeechObjectThread ()
    {
        if (! CloseHandle (terminate_ev))
            loglog_win32_error (
                LOG4CPLUS_TEXT ("SpeechObjectThread: CloseHandle failed"));
    }


    virtual
    void
    run ()
    {
        COMInitializer com_init (COINIT_MULTITHREADED);

        HRESULT hr = CoCreateInstance (CLSID_SpVoice, nullptr,
            CLSCTX_ALL, IID_ISpVoice, reinterpret_cast<void **>(&ispvoice));
        if (FAILED (hr))
            loglog_com_error (
                LOG4CPLUS_TEXT ("SpeechObjectThread: CoCreateInstance(IID_ISpVoice) failed"),
                hr);

        init_ev.signal ();

        for (;;)
        {
            // wait for the event and for messages
            DWORD dwReturn = 0;
            hr = ::CoWaitForMultipleHandles (0, INFINITE, 1, &terminate_ev,
                &dwReturn);

            // this thread has been reawakened. Determine why
            // and handle appropriately.
            if (hr == S_OK && dwReturn == WAIT_OBJECT_0)
                // our event happened.
                break;
            else if (FAILED (hr))
                loglog_com_error (
                    LOG4CPLUS_TEXT ("SpeechObjectThread: CoWaitForMultipleHandles() failed"),
                    hr);
        }
    };

    void
    wait_init_done () const
    {
        init_ev.wait ();
    }

    void
    terminate () const
    {
        if (! SetEvent (terminate_ev))
            loglog_win32_error (LOG4CPLUS_TEXT ("SpeechObjectThread: SetEvent failed"), true);
    }

private:
    ISpVoice * & ispvoice;
    thread::ManualResetEvent init_ev;
    HANDLE terminate_ev;
};


} // namespace


struct MSTTSAppender::Data
{
    Data ()
        : ispvoice (0)
        , async (false)
        , speak_punc (false)
    { }

    ~Data ()
    {
        if (ispvoice)
            ispvoice->Release ();
    }

    helpers::SharedObjectPtr<SpeechObjectThread> speech_thread;
    ISpVoice * ispvoice;
    bool async;
    bool speak_punc;
};


MSTTSAppender::MSTTSAppender ()
    : Appender ()
    , data (new Data)
{
    init ();
}


MSTTSAppender::MSTTSAppender (helpers::Properties const & props)
    : Appender (props)
    , data (new Data)
{
    unsigned long volume = 0;
    bool has_volume = props.getULong (volume, LOG4CPLUS_TEXT ("Volume"));
    if (has_volume)
        volume = (std::min) (volume, 100ul);

    long rate = 0;
    bool has_rate = props.getLong (rate, LOG4CPLUS_TEXT ("Rate"));
    if (has_rate)
        rate = (std::max) (-10l, (std::min) (rate, 10l));

    bool async = false;
    async = props.getBool (async, LOG4CPLUS_TEXT ("Async")) && async;

    bool speak_punc = false;
    speak_punc = props.getBool (speak_punc, LOG4CPLUS_TEXT ("SpeakPunc"))
        && speak_punc;

    init (has_rate ? &rate : 0, has_volume ? &volume : 0, speak_punc, async);
}


MSTTSAppender::~MSTTSAppender ()
{
    destructorImpl ();
    delete data;
}


void
MSTTSAppender::init (long const * rate, unsigned long const * volume,
    bool speak_punc, bool async)
{
    data->speech_thread =
        helpers::SharedObjectPtr<SpeechObjectThread> (
            new SpeechObjectThread (data->ispvoice));
    data->speech_thread->start ();
    data->speech_thread->wait_init_done ();

    COMInitializer com_init;

    HRESULT hr = S_OK;

    if (rate)
    {
        hr = data->ispvoice->SetRate (*rate);
        if (FAILED (hr))
            loglog_com_error (
                LOG4CPLUS_TEXT ("SpeechObjectThread: SetRate failed"), hr);
    }

    if (volume)
    {
        hr = data->ispvoice->SetVolume (static_cast<USHORT>(*volume));
        if (FAILED (hr))
            loglog_com_error (
                LOG4CPLUS_TEXT ("SpeechObjectThread: SetVolume failed"), hr);
    }

    data->async = async;
    data->speak_punc = speak_punc;
}


void
MSTTSAppender::close ()
{
    data->speech_thread->terminate ();
    data->speech_thread->join ();
}


void
MSTTSAppender::append (spi::InternalLoggingEvent const & ev)
{
    if (! data->ispvoice)
        return;

    // TODO: Expose log4cplus' internal TLS to use here.
    tostringstream oss;
    layout->formatAndAppend(oss, ev);

    DWORD flags = SPF_IS_NOT_XML;

    flags |= SPF_ASYNC * +data->async;
    flags |= SPF_NLP_SPEAK_PUNC * +data->speak_punc;

    COMInitializer com_init;
    HRESULT hr = data->ispvoice->Speak (
        helpers::towstring (oss.str ()).c_str (), flags, nullptr);
    if (FAILED (hr))
        loglog_com_error (LOG4CPLUS_TEXT ("Speak failed"), hr);
}


void
MSTTSAppender::registerAppender ()
{
    log4cplus::spi::AppenderFactoryRegistry & reg
        = log4cplus::spi::getAppenderFactoryRegistry ();
    LOG4CPLUS_REG_APPENDER (reg, MSTTSAppender);
}


} // namespace log4cplus


extern "C"
BOOL WINAPI DllMain(LOG4CPLUS_DLLMAIN_HINSTANCE,  // handle to DLL module
                    DWORD fdwReason,     // reason for calling function
                    LPVOID)  // reserved
{
    // Perform actions based on the reason for calling.
    switch( fdwReason )
    {
    case DLL_PROCESS_ATTACH:
    {
        // We cannot do this here because it causes the thread to deadlock 
        // when compiled with Visual Studio due to use of C++11 threading 
        // facilities.

        //log4cplus::MSTTSAppender::registerAppender ();
        break;
    }

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}
