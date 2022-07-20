#ifndef _NF_THREAD_H
#define _NF_THREAD_H

namespace ProtocolFilters
{

#ifdef WIN32

#define NFTHREAD_FUNC unsigned WINAPI
typedef unsigned (WINAPI * NFTHREAD_START)(void*);

class NFThread
{
public:
	NFThread()
	{
		m_hThread = NULL;
	}
	~NFThread()
	{
	}
	bool create(NFTHREAD_START start_routine, void *arg)
	{
		if (m_hThread != 0)
			return false;

		m_hThread = (HANDLE)_beginthreadex(0, 0, start_routine, arg, 0, NULL);
		if (m_hThread == (HANDLE)(-1L))
		{
			m_hThread = NULL;
			return false;
		}
		return true;
	}
	bool wait()
	{
		if (m_hThread == 0)
			return false;

        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
        m_hThread = NULL;
		return true;
	}
	bool isCreated()
	{
		return m_hThread != NULL;
	}
private:
	HANDLE m_hThread;
};


#else

#define NFTHREAD_FUNC void *
typedef void *(* NFTHREAD_START)(void*);

class NFThread
{
public:
	NFThread()
	{
		m_hThread = 0;
	}
	~NFThread()
	{
	}
	bool create(NFTHREAD_START start_routine, void *arg)
	{
		if (m_hThread != 0)
			return false;

		return pthread_create(&m_hThread, NULL, start_routine, arg) == 0;
	}
	bool wait()
	{
		bool res = (pthread_join(m_hThread, NULL) == 0);
		m_hThread = 0;
		return res;
	}
	bool isCreated()
	{
		return m_hThread != 0;
	}
private:
	pthread_t m_hThread;
};

#endif

}

#endif