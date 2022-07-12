//
// EDRAv2.edrdrv
//
// Author: Yury Podpruzhnikov (15.06.2019)
// Reviewer:
//
///
/// @file miscellaneous
///
/// @addtogroup kernelCmnLib
/// @{
#pragma once

namespace cmd {

//////////////////////////////////////////////////////////////////////////
//
// Enum and flags support
//
//////////////////////////////////////////////////////////////////////////

#define CMD_DEFINE_ENUM_FLAG_OPERATORS(EnumType, UnderlyingType) \
inline constexpr EnumType operator | (EnumType a, EnumType b) noexcept { return static_cast<EnumType>((UnderlyingType)a | (UnderlyingType)b); } \
inline constexpr EnumType operator & (EnumType a, EnumType b) noexcept { return static_cast<EnumType>((UnderlyingType)a & (UnderlyingType)b); } \
inline constexpr EnumType operator ^ (EnumType a, EnumType b) noexcept { return static_cast<EnumType>((UnderlyingType)a ^ (UnderlyingType)b); } \
inline EnumType& operator |= (EnumType &a, EnumType b) noexcept { (*(UnderlyingType*)&a |= (UnderlyingType)b); return a; } \
inline EnumType& operator &= (EnumType &a, EnumType b) noexcept { (*(UnderlyingType*)&a &= (UnderlyingType)b); return a; } \
inline EnumType& operator ^= (EnumType &a, EnumType b) noexcept { (*(UnderlyingType*)&a ^= (UnderlyingType)b); return a; } \
inline constexpr EnumType operator ~ (EnumType a) noexcept { return static_cast<EnumType>(~(UnderlyingType)a); }

///
/// Bit operations
///
template <typename T>
inline bool testFlag(T V, T F)
{
	return (V & F) != T(0);
}

template <typename T>
inline T resetFlag(T& V, T F)
{
	V &= (~(F));
	return V;
}

template <typename T>
inline T resetFlag(const T& V, T F)
{
	return V & (~(F));
}

template <typename T>
inline T setFlag(T& V, T F)
{
	V |= F;
	return V;
}

template <typename T>
inline bool testMask(T V, T M)
{
	return (V & M) == M;
}

template <typename T>
inline bool testMaskAny(T V, T M)
{
	return (V & M) != T(0);
}


///
/// Definition of enum which contains flags.
///
/// @param EnumType - a name of new enum.
/// @param UnderlyingType - a name of underlying type.
///
#define CMD_DEFINE_ENUM_FLAG(EnumType, UnderlyingType) \
enum class EnumType : UnderlyingType; \
CMD_DEFINE_ENUM_FLAG_OPERATORS(EnumType, UnderlyingType) \
enum class EnumType : UnderlyingType


//////////////////////////////////////////////////////////////////////////
//
// Buffer with size
//
//////////////////////////////////////////////////////////////////////////

///
/// The class for buffer with size.
///
template<PoolType c_ePoolType = PoolType::NonPaged>
class Blob
{
private:
	void* m_pData = nullptr;  //< data
	size_t m_nSize = 0;

	void release()
	{
		m_pData = nullptr;
		m_nSize = 0;
	}

	void reset()
	{
		if (m_pData != nullptr)
		{
			ExFreePoolWithTag(m_pData, c_nAllocTag);
			m_pData = nullptr;
			m_nSize = 0;
		}
	}

	void reset(void* pData, size_t nSize)
	{
		reset();
		m_pData = pData;
		m_nSize = nSize;
	}


public:

	Blob() = default;
	~Blob()
	{
		reset();
	}

	// Deny copy
	Blob(const Blob&) = delete;
	Blob& operator=(const Blob&) = delete;

	// Allow move
	Blob(Blob&& other)
	{
		reset(other.m_pData, other.m_nSize);
		other.release();
	}

	Blob& operator=(Blob&& other)
	{
		reset(other.m_pData, other.m_nSize);
		other.release();
		return *this;
	}

	//
	// Alloc new buffer and assign data
	//

	NTSTATUS alloc(size_t nNewSize)
	{
		void* pBuffer = nullptr;
		if (nNewSize != 0)
		{
			pBuffer = ExAllocatePoolWithTag(convetrToSystemPoolType(c_ePoolType), nNewSize, c_nAllocTag);
			if (pBuffer == nullptr)
				return STATUS_NO_MEMORY;
		}
		reset(pBuffer, nNewSize);
		return STATUS_SUCCESS;
	}

	NTSTATUS alloc(size_t nNewSize, uint8_t nFiller)
	{
		NTSTATUS ns = alloc(nNewSize);
		if (!NT_SUCCESS(ns)) 
			return ns;
		if(m_nSize != 0)
			memset(m_pData, nFiller, m_nSize);
		return STATUS_SUCCESS;
	}

	NTSTATUS alloc(const void* pData, size_t nDataSize)
	{
		NTSTATUS ns = alloc(nDataSize);
		if (!NT_SUCCESS(ns))
			return ns;
		if (m_nSize != 0)
			memcpy(m_pData, pData, m_nSize);
		return STATUS_SUCCESS;
	}

	NTSTATUS alloc(const void* pData, size_t nDataSize, size_t nNewSize)
	{
		NTSTATUS ns = alloc(nNewSize);
		if (!NT_SUCCESS(ns))
			return ns;
		if (m_nSize != 0)
			memcpy(m_pData, pData, min(nNewSize, nDataSize));
		return STATUS_SUCCESS;
	}

	NTSTATUS alloc(const Blob& other)
	{
		return alloc(other.m_pData, other.m_nSize);
	}

	NTSTATUS alloc(const Blob& other, size_t nNewSize)
	{
		return alloc(other.m_pData, other.m_nSize, nNewSize);
	}

	///
	/// Clear the data.
	///
	void clear()
	{
		reset();
	}

	///
	/// Get the data.
	///
	/// @return The function returns a raw pointer to the data.
	///
	uint8_t* getData()
	{
		return (uint8_t*)m_pData;
	}

	///
	/// Get the data.
	///
	/// @return The function returns a raw pointer to the data.
	///
	const uint8_t* getData() const
	{
		return (const uint8_t*)m_pData;
	}

	///
	/// Get the data size.
	///
	/// @return The function returns the size to the data.
	///
	size_t getSize() const
	{
		return m_nSize;
	}

	///
	/// Checks if container has no data.
	///
	/// @return The function return `true` if no data available.
	///
	bool isEmpty() const
	{
		return m_nSize == 0;
	}

	///
	/// Returns that container has data.
	///
	operator bool() const
	{
		return !isEmpty();
	}

};


//////////////////////////////////////////////////////////////////////////
//
// UniquePtr
// Smart pointer for kernel mode. Like unique_ptr 
//
//////////////////////////////////////////////////////////////////////////

//
// Default deleter for UniquePtr
// It uses operator delete
//
template<typename T>
struct DefaultDeleter
{
	void operator () (T* pData)
	{
		delete pData;
	}
};

template<typename T>
struct DefaultDeleter<T[]>
{
	void operator () (T* pData)
	{
		delete [] pData;
	}
};

//
// ExFreePool deleter for UniquePtr
//
struct FreeDeleter
{
	void operator () (void* pData)
	{
		ExFreePool(pData);
	}
};

template<typename T>
struct GetPointer
{
	using type = T * ;
};

template<typename T>
struct GetPointer<T[]>
{
	using type = T * ;
};


///
/// Smart pointer for kernel mode.
///
/// Like unique_ptr.
///
/// @tparam T - data type.
/// @tparam fUseDelete - way to free data Data type.
///
template<typename T, typename Deleter=DefaultDeleter<T> >
class UniquePtr
{
public:
	using Pointer = typename GetPointer<T>::type;
private:

	Pointer m_pData; //< data


	//
	// call Deleter
	//
	void cleanup()
	{
		if (m_pData == nullptr)
			return;
		Deleter()(m_pData);
		m_pData = nullptr;
	}

public:

	///
	/// Constructor.
	///
	UniquePtr(Pointer pData = nullptr) : m_pData(pData)
	{
	}

	///
	/// Destructor.
	///
	~UniquePtr()
	{
		reset();
	}

	//
	// deny coping
	//
	UniquePtr(const UniquePtr&) = delete;
	UniquePtr<T>& operator= (const UniquePtr&) = delete;

	//
	// move constructor
	//
	UniquePtr(UniquePtr && other) : m_pData(other.release())
	{
	}

	//
	// move assignment
	//
	UniquePtr& operator= (UniquePtr && other)
	{
		if (this != &other)
			reset(other.release());
		return *this;
	}

	//
	// operator bool
	//
	operator bool() const
	{
		return m_pData != nullptr;
	}

	//
	// operator->
	//
	Pointer operator->() const
	{
		return m_pData;
	}

	///
	/// Get the data.
	///
	/// @return The function returns a smart pointer.
	///
	Pointer get() const
	{
		return m_pData;
	}

	///
	/// Release.
	///
	/// @return The function returns a smart pointer.
	///
	Pointer release()
	{
		T* pData = m_pData;
		m_pData = nullptr;
		return pData;
	}

	///
	/// Reset.
	///
	/// @param pData [opt] - a pointer to the data.
	///
	void reset(Pointer pData = nullptr)
	{
		cleanup();
		m_pData = pData;
	}

};

} // namespace cmd

/// @}
