//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (15.06.2019)
// Reviewer:
//
///
/// @file Atomic types
///
/// @addtogroup kernelCmnLib
/// @{
#pragma once

namespace openEdr {

//
// Atomic types
//

//
// Helper type traits for underlying.
// @tparam c_nTypeSize is sizeof(wrapped type)
//
template<size_t c_nTypeSize>
struct AtomicIntTypeTraits {};

template<>
struct AtomicIntTypeTraits<1>
{
	typedef INT8 UnderlyingType;
	static UnderlyingType load(volatile UnderlyingType* pData) { return *pData; }
	static void store(volatile UnderlyingType* pData, UnderlyingType val) { *pData = val; }
	static UnderlyingType increment(volatile UnderlyingType* pData) { return ++(*pData); }
	static UnderlyingType decrement(volatile UnderlyingType* pData) { return --(*pData); }
};

template<>
struct AtomicIntTypeTraits<2>
{
	typedef SHORT UnderlyingType;
	static UnderlyingType load(volatile UnderlyingType* pData) { return InterlockedCompareExchange16(pData, 0, 0); }
	static void store(volatile UnderlyingType* pData, UnderlyingType val) { InterlockedExchange16(pData, val); }
	static UnderlyingType increment(volatile UnderlyingType* pData) { return InterlockedIncrement16(pData); }
	static UnderlyingType decrement(volatile UnderlyingType* pData) { return InterlockedDecrement16(pData); }
};

template<>
struct AtomicIntTypeTraits<4>
{
	typedef LONG UnderlyingType;
	static UnderlyingType load(volatile UnderlyingType* pData) { return InterlockedCompareExchange(pData, 0, 0); }
	static void store(volatile UnderlyingType* pData, UnderlyingType val) { InterlockedExchange(pData, val); }
	static UnderlyingType increment(volatile UnderlyingType* pData) { return InterlockedIncrement(pData); }
	static UnderlyingType decrement(volatile UnderlyingType* pData) { return InterlockedDecrement(pData); }
};

template<>
struct AtomicIntTypeTraits<8>
{
	typedef LONGLONG UnderlyingType;
	static UnderlyingType load(volatile UnderlyingType* pData) { return InterlockedCompareExchange64(pData, 0, 0); }
	static void store(volatile UnderlyingType* pData, UnderlyingType val) { InterlockedExchange64(pData, val); }
	static UnderlyingType increment(volatile UnderlyingType* pData) { return InterlockedIncrement64(pData); }
	static UnderlyingType decrement(volatile UnderlyingType* pData) { return InterlockedDecrement64(pData); }
};

///
/// Atomic integer value for kernel mode.
///
template<typename T, typename _TypeTraits>
class AtomicBase
{
protected:

	static_assert(sizeof(T) <= 8, "AtomicBase support up to 64 bit only");
	using TypeTraits = _TypeTraits;
	typedef typename TypeTraits::UnderlyingType UnderlyingType;

	alignas(volatile UnderlyingType) volatile UnderlyingType m_val;

public:

	//
	//
	//
	__forceinline T load()
	{
		return static_cast<T>(TypeTraits::load(&m_val));
	}

	//
	//
	//
	__forceinline void store(T val)
	{
		TypeTraits::store(&m_val, static_cast<UnderlyingType>(val));
	}

	//
	//
	//
	__forceinline AtomicBase& operator=(T val)
	{
		store(val);
		return *this;
	}

	//
	//
	//
	__forceinline operator T()
	{
		return load();
	}
};



///
/// Atomic boolean value for kernel mode.
///
class AtomicBool : public AtomicBase<bool, AtomicIntTypeTraits<sizeof(ULONG)> >
{
	typedef typename AtomicBase<bool, AtomicIntTypeTraits<sizeof(ULONG)> > Base;
public:
	AtomicBool(bool val = false) { m_val = val; }
};

///
/// Atomic integer value for kernel mode.
///
template<typename T>
class AtomicInt : public AtomicBase<T, AtomicIntTypeTraits<sizeof(T)> >
{
private:
	typedef typename AtomicBase<T, AtomicIntTypeTraits<sizeof(T)> > Base;

public:
	AtomicInt(T val = T{})
	{
		this->m_val = (typename Base::UnderlyingType)val;
	}

	T operator++()
	{
		return (T)Base::TypeTraits::increment(&this->m_val);
	}

	T operator--()
	{
		return (T)Base::TypeTraits::decrement(&this->m_val);
	}
};

} // namespace openEdr

/// @}
