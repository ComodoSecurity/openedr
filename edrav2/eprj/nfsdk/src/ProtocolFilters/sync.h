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

namespace ProtocolFilters
{

#if defined(WIN32)
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
		if( m_h != INVALID_HANDLE_VALUE )
		{
			Close();
		}
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

	class AutoEventHandleM : public AutoHandle
	{
	public:
		AutoEventHandleM()
		{
			m_h = CreateEvent(NULL, TRUE, FALSE, NULL);
		}
		~AutoEventHandleM()
		{
			Close();
		}
	};
#else
	class AutoCriticalSection
	{
	public:
		AutoCriticalSection() throw()
		{
            pthread_mutexattr_t attr;
            
            int const init_attr_res=pthread_mutexattr_init(&attr);
            if(init_attr_res)
            {
                throw std::exception();
            }
            int const set_attr_res=pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
            if(set_attr_res)
            {
                throw std::exception();
            }

			int const res=pthread_mutex_init(&m, &attr);
            if(res)
            {
                throw std::exception();
            }

			pthread_mutexattr_destroy(&attr);
		}
		~AutoCriticalSection()
		{
			pthread_mutex_destroy(&m);
		}
		void Lock()
		{
            pthread_mutex_lock(&m);
		}
		void Unlock()
		{
            pthread_mutex_unlock(&m);
		}

		pthread_mutex_t m;
	};

#endif

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

#if defined(WIN32)

	class NFSyncEvent
	{
	public:
		NFSyncEvent()
		{
			m_flags = 0;
		}
		~NFSyncEvent()
		{
		}
		
		bool fire(int flags)
		{
			{
				AutoLock lock(m_cs);
				m_flags |= flags;
			}

			return (SetEvent(m_event) != 0);
		}

		bool wait(int * pFlags)
		{
			if (WaitForSingleObject(m_event, INFINITE) == WAIT_OBJECT_0)
			{
				AutoLock lock(m_cs);
				if (pFlags)
				{
					*pFlags = m_flags;
				}
				m_flags = 0;
				return true;
			}

			return false;
		}

		bool isFlagFired(int flag)
		{
			AutoLock lock(m_cs);
			return (m_flags & flag) != 0;
		}

	private:
		AutoEventHandle m_event;
		int m_flags;
		AutoCriticalSection	m_cs;
	};

#else

	class NFSyncEvent
	{
	public:
		NFSyncEvent() throw()
		{
			m_flags = 0;
			if (pthread_cond_init(&m_cond, NULL) != 0)
			{
				throw std::exception();
			}
		}
		~NFSyncEvent()
		{
			pthread_cond_destroy(&m_cond);
		}
		
		bool fire(int flags)
		{
			{
				AutoLock lock(m_cs);
				m_flags |= flags;
			}

			return (pthread_cond_signal(&m_cond) == 0);
		}

		bool wait(int * pFlags)
		{
			AutoLock lock(m_cs);
			
			if (pthread_cond_wait(&m_cond, &m_cs.m) == 0)
			{
				if (pFlags)
				{
					*pFlags = m_flags;
				}
				m_flags = 0;
				return true;
			}

			return false;
		}
		
		bool isFlagFired(int flag)
		{
			AutoLock lock(m_cs);
			return (m_flags & flag) != 0;
		}

	private:
		pthread_cond_t m_cond;
		int m_flags;
		AutoCriticalSection	m_cs;
	};

#endif

}

#endif