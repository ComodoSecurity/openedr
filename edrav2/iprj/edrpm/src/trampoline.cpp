//
// edrav2.edrpm project
//
// Author: Yury Podpruzhnikov (26.04.2019)
// Reviewer: Denis Bogdanov (05.05.2019)
//
// Basis of trampolines
//
#include "pch.h"
#include "trampoline.h"

namespace cmd {
namespace edrpm {


template<typename T>
class ThreadLocalData
{
	static_assert(sizeof(T) <= sizeof(LPVOID), "Size is too large.");

	// TLS index
	DWORD m_nTlsIndex = TLS_OUT_OF_INDEXES;
public:

	//
	//
	//
	ThreadLocalData(T value = T{})
	{
		m_nTlsIndex = ::TlsAlloc();
		// TODO: process error
		put(value);
	}

	//
	//
	//
	~ThreadLocalData()
	{
		if(m_nTlsIndex != TLS_OUT_OF_INDEXES)
			::TlsFree(m_nTlsIndex);
	}

	//
	// Checks correct initialization
	//
	bool isValid()
	{
		return m_nTlsIndex != TLS_OUT_OF_INDEXES;
	}

	//
	// get value
	//
	T get()
	{
		if (m_nTlsIndex == TLS_OUT_OF_INDEXES)
			return T{};

		ULONG_PTR data = (ULONG_PTR) TlsGetValue(m_nTlsIndex);
		return (T)data;
	}

	//
	// get value
	//
	void put(T value)
	{
		if (m_nTlsIndex == TLS_OUT_OF_INDEXES)
			return;

		ULONG_PTR data = (ULONG_PTR)value;
		TlsSetValue(m_nTlsIndex, (LPVOID)data);
	}
};

ThreadLocalData<bool> g_fIsIntrampoline = false; 

//
//
//
bool isTrampolineAlreadyCalled()
{
	return g_fIsIntrampoline.get();
}

//
//
//
void markTrampolineAsCalled(bool val)
{
	g_fIsIntrampoline.put(val);
}


//
//
//
TrampManager& getTrampManager()
{
	static TrampManager g_TrampManager;
	return g_TrampManager;
}

//
//
//
void ITramp::addIntoTrampManager(TrampId nTrampId)
{
	getTrampManager().addTramp(nTrampId, this);
}

//
//
//
ICallbackRegistrator::ICallbackRegistrator()
{
	getTrampManager().addCallbackRegistrator(this);
}

} // namespace edrpm
} // namespace cmd

