//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _SYNC_H
#define _SYNC_H

namespace nfapi
{
	class CriticalSection
	{
	public:
		CriticalSection() throw()
		{
			memset(&m_sec, 0, sizeof(CRITICAL_SECTION));
		}
		~CriticalSection()
		{
		}
		HRESULT Lock() throw()
		{
			EnterCriticalSection(&m_sec);
			return S_OK;
		}
		HRESULT Unlock() throw()
		{
			LeaveCriticalSection(&m_sec);
			return S_OK;
		}
		HRESULT Init() throw()
		{
			HRESULT hRes = E_FAIL;
			__try
			{
				InitializeCriticalSection(&m_sec);
				hRes = S_OK;
			}
			// structured exception may be raised in low memory situations
			__except(STATUS_NO_MEMORY == GetExceptionCode())
			{			
				hRes = E_OUTOFMEMORY;		
			}
			return hRes;
		}

		HRESULT Term() throw()
		{
			DeleteCriticalSection(&m_sec);
			return S_OK;
		}	
		CRITICAL_SECTION m_sec;
	};

	class AutoCriticalSection : public CriticalSection
	{
	public:
		AutoCriticalSection()
		{
			CriticalSection::Init();
		}
		~AutoCriticalSection() throw()
		{
			CriticalSection::Term();
		}
	private :
		HRESULT Init();
		HRESULT Term();
	};

	class AutoLock
	{
	public:
		AutoLock(AutoCriticalSection& cs) : m_cs(cs)
		{
			m_cs.Lock();
		}

		virtual ~AutoLock()
		{
			m_cs.Unlock(); 
		}

	private:
		AutoCriticalSection& m_cs;
	};

	class AutoUnlock
	{
	public:
		AutoUnlock(AutoCriticalSection& cs) : m_cs(cs)
		{
			m_cs.Unlock();
		}

		virtual ~AutoUnlock()
		{
			m_cs.Lock(); 
		}

	private:
		AutoCriticalSection& m_cs;
	};

	class AutoHandle
	{
	public:
		AutoHandle() throw();
		AutoHandle( AutoHandle& h ) throw();
		explicit AutoHandle( HANDLE h ) throw();
		~AutoHandle() throw();

		AutoHandle& operator=( AutoHandle& h ) throw();

		operator HANDLE() const throw();

		// Attach to an existing handle (takes ownership).
		void Attach( HANDLE h ) throw();
		// Detach the handle from the object (releases ownership).
		HANDLE Detach() throw();

		// Close the handle.
		void Close() throw();

	public:
		HANDLE m_h;
	};

	inline AutoHandle::AutoHandle() throw() :
		m_h( INVALID_HANDLE_VALUE )
	{
	}

	inline AutoHandle::AutoHandle( AutoHandle& h ) throw() :
		m_h( INVALID_HANDLE_VALUE )
	{
		Attach( h.Detach() );
	}

	inline AutoHandle::AutoHandle( HANDLE h ) throw() :
		m_h( h )
	{
	}

	inline AutoHandle::~AutoHandle() throw()
	{
		if( m_h != INVALID_HANDLE_VALUE )
		{
			Close();
		}
	}

	inline AutoHandle& AutoHandle::operator=( AutoHandle& h ) throw()
	{
		if( this != &h )
		{
			if( m_h != INVALID_HANDLE_VALUE )
			{
				Close();
			}
			Attach( h.Detach() );
		}

		return( *this );
	}

	inline AutoHandle::operator HANDLE() const throw()
	{
		return( m_h );
	}

	inline void AutoHandle::Attach( HANDLE h ) throw()
	{
		m_h = h;  // Take ownership
	}

	inline HANDLE AutoHandle::Detach() throw()
	{
		HANDLE h;

		h = m_h;  // Release ownership
		m_h = INVALID_HANDLE_VALUE;

		return( h );
	}

	inline void AutoHandle::Close() throw()
	{
		if( m_h != INVALID_HANDLE_VALUE )
		{
			::CloseHandle( m_h );
			m_h = INVALID_HANDLE_VALUE;
		}
	}

	class AutoEventHandle : public AutoHandle
	{
	public:
		AutoEventHandle()
		{
			m_h = CreateEvent(NULL, FALSE, FALSE, NULL);
		}
		~AutoEventHandle()
		{
			Close();
		}
	};
}

#endif