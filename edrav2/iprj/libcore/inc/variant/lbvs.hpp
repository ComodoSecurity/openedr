//
// EDRAv2.libcore project
//
// Author: Denis Kroshin (31.01.2019)
// Reviewer: Denis Bogdanov (04.02.2019)
//
// FIXME: define group to include (@ladygin)
// @addtogroup commonFunctions Common functions
///
/// @file Implementation of LBVS serialization
///
/// @{
#pragma once


#ifndef CMD_KERNEL_MODE
#include <stdint.h>
#endif // CMD_KERNEL_MODE

namespace cmd {
namespace variant {

namespace lbvs {

///
/// User defined enum must inherit from FieldId
///
typedef uint16_t FieldId;

/// Invalid FieldId value
static const FieldId g_nInvalidFieldId = 0xffffui16; // UINT16_MAX

///
/// Redefine size for stream to reduce packet size
///
typedef uint16_t StreamSize;

///
/// Internal type of data of LBVS
///
enum class FieldType : uint8_t
{
	Invalid = (uint8_t)-1,
	Null = 0,
	String = 1,
	WString = 2,
	Stream = 3,
	Bool = 4,
	Uint32 = 5,
	Uint64 = 6,
	SeqDict = 7,
	SeqSeq = 8
};

#ifndef CMD_KERNEL_MODE

// FIXME: Why template function is used here?
template<typename T>
inline T convertFromString(std::string_view sType)
{
	static_assert(false, "Type does not support convertation to string");
}

//
//
//
template<>
inline FieldType convertFromString<FieldType>(std::string_view sType)
{
	if (sType == "null")
		return FieldType::Null;
	if (sType == "str")
		return FieldType::String;
	if (sType == "wstr")
		return FieldType::WString;
	if (sType == "stream")
		return FieldType::Stream;
	if (sType == "bool")
		return FieldType::Bool;
	if (sType == "uint32")
		return FieldType::Uint32;
	if (sType == "uint64")
		return FieldType::Uint64;
	if (sType == "seqdict")
		return FieldType::SeqDict;
	if (sType == "seqseq")
		return FieldType::SeqSeq;
	if (sType == "uint32")
		return FieldType::Uint32;
	return FieldType::Invalid;
}
#endif


#pragma pack(push, 1)

// FIXME: Please add documentation
///
/// LBVS packet header
///
struct LbvsHeader
{
	uint32_t nMagic;
	uint8_t nVersion;
	uint32_t nSize;
	uint8_t nCount;
};

///
/// LBVS Field header
///
struct FieldHeader
{
	FieldId eId;
	FieldType eType;
};
#pragma pack(pop)

typedef void* (*FnAlloc) (size_t nSize);

typedef void(*FnFree) (void* pPtr);

///
/// @brief "Null" allocator for LbvsSerializer. It forces use only static buffer.
///
/// Every allocator must provide 2 methods: 
/// * allocMem 
/// * freeMem 
///
struct NullAllocator
{
	///
	/// @brief User defined allocator function
	/// @param nSize Size to allocate
	/// @return Pointer to allocated data
	/// @throw Allocator must NOT throw. Return nullptr on fail.
	///
	void* allocMem(size_t /*nSize*/)
	{
		return nullptr;
	}

	///
	/// @brief User defined free function
	/// @param pPtr Pointer to previously allocated data
	/// @throw Deallocator must NOT throw.
	///
	void freeMem(void* /*pPtr*/)
	{
	}
};

} // namespace lbvs

///
/// @brief Object serialize data to LBVS format
///
template<typename IdType, typename MemAllocator, size_t c_nBufferSize, size_t c_nLimitSize>
class BasicLbvsSerializer
{
private:

	// FIXME: It is common function!
	template<typename T>
	inline static T growToLog2(T Val, T Size)
	{
		return (Val & (Size - 1)) ? (Val & ~(Size - 1)) + Size : Val;
	}

	static const size_t c_nBufferGrow = 0x100;

	uint8_t m_pBuffer[c_nBufferSize] = {};
	size_t m_nBufferSize = c_nBufferSize;
	uint8_t* m_pData = m_pBuffer;
	size_t m_nDataSize = 0;

	lbvs::LbvsHeader m_Hdr = { 'SVBL', 1, sizeof(lbvs::LbvsHeader), 0 };

	MemAllocator m_allocator;

	// FIXME: In this function the field is allocated so we can add initialization of headers here
	// and rename it to addField()
	///
	/// @brief Reserve memory for next item
	/// @param nSize Size of item to serialize
	/// @return Pointer where new item will be serialized
	///
	uint8_t* reserveMem(size_t nSize)
	{
		uint8_t* pNewPtr = nullptr;
		if (m_nBufferSize - m_Hdr.nSize < nSize)
		{
			size_t nNewSize = growToLog2(m_Hdr.nSize + nSize + c_nBufferGrow, c_nBufferGrow);
			// Check limit and overflow
			if (nNewSize > c_nLimitSize || nNewSize < m_Hdr.nSize)
				return nullptr;
			pNewPtr = (uint8_t*)m_allocator.allocMem(nNewSize);
			if (pNewPtr == nullptr)
				return nullptr;
			m_nBufferSize = nNewSize;
			memcpy(pNewPtr, m_pData, m_Hdr.nSize);
			if (m_pData != m_pBuffer)
				m_allocator.freeMem(m_pData);
			m_pData = pNewPtr;
		}
		pNewPtr = m_pData + m_Hdr.nSize;
		m_Hdr.nSize += static_cast<uint32_t>(nSize);
		m_Hdr.nCount++;
		return pNewPtr;
	}

public:
	BasicLbvsSerializer() = default;

	// Disable copy and move
	BasicLbvsSerializer(const BasicLbvsSerializer&) = delete;
	BasicLbvsSerializer(BasicLbvsSerializer&&) = delete;
	BasicLbvsSerializer& operator=(const BasicLbvsSerializer&) = delete;
	BasicLbvsSerializer& operator=(BasicLbvsSerializer&&) = delete;

	//
	//
	//
	~BasicLbvsSerializer()
	{
		if (m_pData != m_pBuffer)
			m_allocator.freeMem(m_pData);
	}

	///
	/// Reset object to initial state
	///
	/// @return Result of operation
	///
	bool reset()
	{
		m_Hdr.nSize = sizeof(lbvs::LbvsHeader);
		m_Hdr.nCount = 0;
		return true;
	}

	///
	/// Return pointer to serialized data
	///
	/// @param ppData Pointer to returned pointer
	/// @param Pointer to returned size
	/// @return Result of operation
	///
	bool getData(void** ppData, size_t* pDataSize)
	{
		if (ppData == nullptr || pDataSize == nullptr)
			return false;
		*(lbvs::LbvsHeader*)m_pData = m_Hdr;
		*ppData = m_pData;
		*pDataSize = m_Hdr.nSize;
		return true;
	}

#ifndef CMD_KERNEL_MODE
	bool getData(std::vector<uint8_t>& vData)
	{
		*(lbvs::LbvsHeader*)m_pData = m_Hdr;
		vData.resize(m_Hdr.nSize);
		std::copy(m_pData, m_pData + m_Hdr.nSize, vData.begin());
		return true;
	}
#endif

	///
	/// Write a new empty dictionary to sequence (Sequence of sequences doesn't supported)
	/// 
	/// @param User defined field id
	/// @return Result of operation
	///
	// TODO: Add support sequence of sequences
	bool write(IdType eFieldId)
	{
		lbvs::FieldHeader* pPtr = (lbvs::FieldHeader*)reserveMem(sizeof(lbvs::FieldHeader));
		if (pPtr == nullptr)
			return false;
		pPtr->eId = lbvs::FieldId(eFieldId);
		pPtr->eType = lbvs::FieldType::SeqDict;
		return true;
	}

	///
	/// @brief Write boolean value
	///
	/// @param User defined field id
	/// @param Value
	/// @return Result of operation
	///
	bool write(IdType eFieldId, bool nValue)
	{
		uint8_t* pPtr = reserveMem(sizeof(lbvs::FieldHeader) + sizeof(uint8_t));
		if (pPtr == nullptr)
			return false;

		((lbvs::FieldHeader*)pPtr)->eId = lbvs::FieldId(eFieldId);
		((lbvs::FieldHeader*)pPtr)->eType = lbvs::FieldType::Bool;
		pPtr += sizeof(lbvs::FieldHeader);
		*(uint8_t*)pPtr = static_cast<uint8_t>(nValue);
		return true;
	}

	///
	/// @brief Write unsigned int 32bits value
	/// @param User defined field id
	/// @param Value
	/// @return Result of operation
	///
	bool write(IdType eFieldId, uint32_t nValue)
	{
		uint8_t* pPtr = reserveMem(sizeof(lbvs::FieldHeader) + sizeof(uint32_t));
		if (pPtr == nullptr)
			return false;

		((lbvs::FieldHeader*)pPtr)->eId = lbvs::FieldId(eFieldId);
		((lbvs::FieldHeader*)pPtr)->eType = lbvs::FieldType::Uint32;
		pPtr += sizeof(lbvs::FieldHeader);
		*(uint32_t*)pPtr = nValue;
		return true;
	}

	///
	/// @brief Write signed int 32bits value
	/// @param User defined field id
	/// @param Value
	/// @return Result of operation
	///
	inline bool write(IdType eFieldId, int nValue)
	{
		// FIXME: Strange conversion between 32-64 bit values
		return write(eFieldId, uint32_t(nValue));
	}

	///
	/// @brief Write unsigned int 64bits value
	/// @param User defined field id
	/// @param Value
	/// @return Result of operation
	///
	bool write(IdType eFieldId, uint64_t nValue)
	{
		uint8_t* pPtr = reserveMem(sizeof(lbvs::FieldHeader) + sizeof(uint64_t));
		if (pPtr == nullptr)
			return false;

		((lbvs::FieldHeader*)pPtr)->eId = lbvs::FieldId(eFieldId);
		((lbvs::FieldHeader*)pPtr)->eType = lbvs::FieldType::Uint64;
		pPtr += sizeof(lbvs::FieldHeader);
		*(uint64_t*)pPtr = nValue;
		return true;
	}

	///
	/// @brief Write signed int 64bits value
	/// @param User defined field id
	/// @param Value
	/// @return Result of operation
	///
	inline bool write(IdType eFieldId, int64_t nValue)
	{
		return write(eFieldId, uint64_t(nValue));
	}

	///
	/// @brief Write multibyte string
	/// @param User defined field id
	/// @param String
	/// @param Length of string in chars
	/// @return Result of operation
	///
	bool write(IdType eFieldId, const char* sStr, size_t nStrLen = SIZE_MAX)
	{
		static const char c_sZero = 0;
		// FIXME: Do we use SIZE_MAX constant???
		const size_t nCopySize = (nStrLen == SIZE_MAX ? strlen(sStr) : nStrLen);
		uint8_t* pPtr = reserveMem(sizeof(lbvs::FieldHeader) + nCopySize + sizeof(c_sZero));
		if (pPtr == nullptr)
			return false;

		((lbvs::FieldHeader*)pPtr)->eId = lbvs::FieldId(eFieldId);
		((lbvs::FieldHeader*)pPtr)->eType = lbvs::FieldType::String;
		pPtr += sizeof(lbvs::FieldHeader);
		memcpy(pPtr, sStr, nCopySize);
		memcpy(pPtr + nCopySize, &c_sZero, sizeof(c_sZero));
		return true;
	}

	///
	/// @brief Write multibyte string
	/// @param User defined field id
	/// @param String
	/// @param Length of string in chars
	/// @return Result of operation
	///
	inline bool write(IdType eFieldId, char* sStr, size_t nStrLen = SIZE_MAX)
	{
		// FIXME: Do we need it? There is automatic conversion 
		return write(eFieldId, (const char*)sStr, nStrLen);
	}

#ifndef CMD_KERNEL_MODE
	inline bool write(IdType eFieldId, const std::string& sStr)
	{
		return write(eFieldId, sStr.c_str(), sStr.size());
	}
#endif

	///
	/// @brief Write unicode string
	/// @param User defined field id
	/// @param String
	/// @param Length of string in chars
	/// @return Result of operation
	///
	bool write(IdType eFieldId, const wchar_t* sStr, size_t nStrLen = SIZE_MAX)
	{
		static const wchar_t c_sZero = 0;
		const size_t nCopySize = (nStrLen == SIZE_MAX ? wcslen(sStr) : nStrLen) * sizeof(wchar_t);
		uint8_t* pPtr = reserveMem(sizeof(lbvs::FieldHeader) + nCopySize + sizeof(c_sZero));
		if (pPtr == nullptr)
			return false;

		((lbvs::FieldHeader*)pPtr)->eId = lbvs::FieldId(eFieldId);
		((lbvs::FieldHeader*)pPtr)->eType = lbvs::FieldType::WString;
		pPtr += sizeof(lbvs::FieldHeader);
		memcpy(pPtr, sStr, nCopySize);
		memcpy(pPtr + nCopySize, &c_sZero, sizeof(c_sZero));
		return true;
	}
	
	///
	/// @brief Write unicode string
	/// @param User defined field id
	/// @param String
	/// @param Length of string in chars
	/// @return Result of operation
	///
	inline bool write(IdType eFieldId, wchar_t* sStr, size_t nStrLen = SIZE_MAX)
	{
		return write(eFieldId, (const wchar_t*)sStr, nStrLen);
	}

#ifndef CMD_KERNEL_MODE
	inline bool write(IdType eFieldId, const std::wstring& sStr)
	{
		return write(eFieldId, sStr.c_str(), sStr.size());
	}
#endif

	///
	/// @brief Write buffer
	/// @param User defined field id
	/// @param Pinter to buffer
	/// @param Size of buffer
	/// @return Result of operation
	///
	bool write(IdType eFieldId, const void* pBuffer, size_t nBufferSize)
	{
		if(pBuffer == nullptr && nBufferSize != 0)
			return false;
		if (nBufferSize > lbvs::StreamSize(-1))
			return false;

		uint8_t* pPtr = reserveMem(sizeof(lbvs::FieldHeader) + sizeof(lbvs::StreamSize) + nBufferSize);
		if (pPtr == nullptr)
			return false;
		((lbvs::FieldHeader*)pPtr)->eId = lbvs::FieldId(eFieldId);
		((lbvs::FieldHeader*)pPtr)->eType = lbvs::FieldType::Stream;
		pPtr += sizeof(lbvs::FieldHeader);
		*(lbvs::StreamSize*)pPtr = static_cast<lbvs::StreamSize>(nBufferSize);
		if(nBufferSize != 0)
			memcpy(pPtr + sizeof(lbvs::StreamSize), pBuffer, nBufferSize);
		return true;
	}

	///
	/// @brief Write buffer
	/// @param User defined field id
	/// @param Pinter to buffer
	/// @param Size of buffer
	/// @return Result of operation
	///
	inline bool write(IdType eFieldId, void* sStr, size_t nStrLen = SIZE_MAX)
	{
		return write(eFieldId, (const void*)sStr, nStrLen);
	}

};

///
/// @brief LbvsSerializer without dynamic allocator
///
template<typename IdType, size_t c_nBufferSize = 0x1000, size_t c_nLimitSize = SIZE_MAX>
using StaticLbvsSerializer = BasicLbvsSerializer<IdType, lbvs::NullAllocator, c_nBufferSize, c_nLimitSize>;

template<typename IdType>
struct BasicLbvsDeserializerField
{
	using FieldType = lbvs::FieldType;

	IdType id = {};
	FieldType type = {};
	void* data = nullptr;
	size_t size = 0;

	//
	//
	//
	bool getValue(uint64_t& value) const
	{
		if (type == FieldType::Uint64)
			value = *(const uint64_t*)data;
		else if (type == FieldType::Uint32)
			value = *(const uint32_t*)data;
		else
			return false;
		return true;
	}

	//
	//
	//
	bool getValue(uint32_t& value) const
	{
		if (type == FieldType::Uint64)
			value = (uint32_t)*(const uint64_t*)data;
		else if (type == FieldType::Uint32)
			value = *(const uint32_t*)data;
		else
			return false;
		return true;
	}

	//
	//
	//
	bool getValue(bool& value) const
	{
		if (type == FieldType::Uint64)
			value = *(const uint64_t*)data != 0;
		else if (type == FieldType::Uint32)
			value = *(const uint32_t*)data != 0;
		else if (type == FieldType::Bool)
			value = *(const uint8_t*)data != 0;
		else
			return false;
		return true;
	}

	//
	// FIXME: Are you sure that we don't need to use copy semantic?
	//
	bool getValue(const char*& value) const
	{
		if (type == FieldType::String)
			value = (const char*)data;
		else
			return false;
		return true;
	}

	//
	//
	//
	bool getValue(const wchar_t*& value) const
	{
		if (type == FieldType::WString)
			value = (const wchar_t*)data;
		else
			return false;
		return true;
	}
		
#ifndef CMD_KERNEL_MODE
	bool getValue(std::string& value) const
	{
		if (type == FieldType::String)
			value = (const char*)data;
		else
			return false;
		return true;
	}

	bool getValue(std::wstring& value) const
	{
		if (type == FieldType::WString)
			value = (const wchar_t*)data;
		else
			return false;
		return true;
	}
#endif // CMD_KERNEL_MODE
};



///
/// @brief Object serialize data to LBVS format
///
template<typename IdType>
class BasicLbvsDeserializer
{
private:
	const uint8_t* m_pData = nullptr;
	size_t m_nDataSize = 0;
	size_t m_nCount = 0;

	// NOTE: Iterator (except end()) can be created from Deserializer only
	struct IteratorTag {};

public:

	BasicLbvsDeserializer() {}
	~BasicLbvsDeserializer() {}

	///
	/// Restart parsing with new data
	///
	/// @param data Pointer to LBVS buffer
	/// @param size Size of LBVS buffer
	/// @return Status of operation
	///
	bool reset(const void* data, const size_t size)
	{
		if (size < sizeof(lbvs::LbvsHeader))
			return false;
		lbvs::LbvsHeader* pHdr = (lbvs::LbvsHeader*)data;
		if (pHdr->nMagic != 'SVBL' || pHdr->nSize > size)
			return false;

		m_pData = (uint8_t*)data;
		m_nDataSize = size;
		m_nCount = pHdr->nCount;
		return true;
	}

	///
	/// Return number of items in LBVS message
	///
	/// @return Number of items
	///
	size_t getCount() const
	{
		return m_nCount;
	}

	typedef BasicLbvsDeserializerField<IdType> Field;

	//
	// Iterator
	//
	class ConstIterator
	{
	private:

		// FIXME: SIZE_MAX???
		size_t m_nOffset = SIZE_MAX;
		const uint8_t* m_pData = nullptr;
		size_t m_nDataSize = 0;
		Field m_Field;

		//
		//
		//
		bool getNextItem()
		{
			if (m_nOffset == SIZE_MAX)
				return false;
			if (m_nOffset + sizeof(lbvs::FieldHeader) > m_nDataSize)
				return false;

			lbvs::FieldHeader* pFldHdr = (lbvs::FieldHeader*)(m_pData + m_nOffset);
			m_nOffset += sizeof(lbvs::FieldHeader);
			m_Field.id = (IdType)pFldHdr->eId;
			m_Field.type = pFldHdr->eType;
			m_Field.data = (void*)(m_pData + m_nOffset);

			// Detect item size
			switch (pFldHdr->eType)
			{
				case lbvs::FieldType::String:
				{
					if (m_nOffset >= m_nDataSize)
						return false;

					const size_t nMaxLen = (m_nDataSize - m_nOffset) / sizeof(char);
					size_t nStrLen = strnlen_s((char*)(m_pData + m_nOffset), nMaxLen);
					if (nStrLen == nMaxLen)
						return false;

					m_Field.size = (nStrLen + 1) * sizeof(char);
					break;
				}
				case lbvs::FieldType::WString:
				{
					if (m_nOffset >= m_nDataSize)
						return false;

					const size_t nMaxLen = (m_nDataSize - m_nOffset) / sizeof(wchar_t);
					size_t nStrLen = wcsnlen_s((wchar_t*)(m_pData + m_nOffset), nMaxLen);
					if (nStrLen == nMaxLen)
						return false;

					m_Field.size = (nStrLen + 1) * sizeof(wchar_t);
					break;
				}
				case lbvs::FieldType::Stream:
				{
					if (m_nOffset + sizeof(lbvs::StreamSize) > m_nDataSize)
						return false;

					size_t nStreamSize = *(lbvs::StreamSize*)(m_pData + m_nOffset);
					m_nOffset += sizeof(lbvs::StreamSize);
					m_Field.data = (void*)(m_pData + m_nOffset);

					m_Field.size = nStreamSize;
					break;
				}
				case lbvs::FieldType::Bool:
				{
					m_Field.size = sizeof(uint8_t);
					break;
				}
				case lbvs::FieldType::Uint32:
				{
					m_Field.size = sizeof(uint32_t);
					break;
				}
				case lbvs::FieldType::Uint64:
				{
					m_Field.size = sizeof(uint64_t);
					break;
				}
				case lbvs::FieldType::SeqDict:
				{
					m_Field.size = 0;
					break;
				}
				default:
					return false;
			}

			if (m_Field.size > m_nDataSize - m_nOffset)
				return false;

			m_nOffset += m_Field.size;
			return true;
		}

	public:

		//
		// Default constructor and constructor for end()
		//
		ConstIterator() = default;

		//
		// Constructor for begin()
		//
		ConstIterator(IteratorTag, const uint8_t* pData, size_t nDataSize) : m_pData(pData), m_nDataSize(nDataSize)
		{
			m_nOffset = sizeof(lbvs::LbvsHeader);
			if (!getNextItem())
				m_nOffset = SIZE_MAX;
		}

		// default copy constructors
		ConstIterator(const ConstIterator&) = default;
		ConstIterator& operator=(const ConstIterator&) = default;

		//
		// Access to value
		//
		const Field& operator*()
		{
			return m_Field;
		}

		//
		// Access to value
		//
		const Field* operator->()
		{
			return &m_Field;
		}

		//
		// Check the end
		//
		bool operator!=(const ConstIterator& other) const
		{
			return m_nOffset != other.m_nOffset;
		}

		//
		// Get next item
		//
		ConstIterator& operator++()
		{
			if (!getNextItem())
				m_nOffset = SIZE_MAX;
			return *this;
		}
	};

	using const_iterator = ConstIterator;

	//
	//
	//
	const_iterator begin() const
	{
		if (m_pData == nullptr)
			return const_iterator{};
		return const_iterator(IteratorTag{}, m_pData, m_nDataSize);
	}

	//
	//
	//
	const_iterator end() const
	{
		return const_iterator{};
	}

};

#ifndef CMD_KERNEL_MODE

namespace lbvs {

//
// User mode allocator for LbvsSerializer
//
struct MallocAllocator
{
	void* allocMem(size_t nSize)
	{
		return malloc(nSize);
	}

	void freeMem(void* pPtr)
	{
		free(pPtr);
	}
};

} // namespace lbvs

///
/// @brief User mode LbvsSerializer with dynamic allocator
///
template<typename IdType, size_t c_nBufferSize = 0x1000, size_t c_nLimitSize = SIZE_MAX>
using LbvsSerializer = BasicLbvsSerializer<IdType, lbvs::MallocAllocator, c_nBufferSize, c_nLimitSize>;

///
/// @brief User mode LbvsDeserializer
///
template<typename IdType>
using LbvsDeserializer = BasicLbvsDeserializer<IdType>;

#endif // CMD_KERNEL_MODE


} // namespace variant
} // namespace cmd
/// @}
