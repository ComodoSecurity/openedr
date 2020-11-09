//
// edrav2.libcore
// Type to keep BasicDictionary key
// 
// Autor: Yury Podpruzhnikov (05.07.2019)
// Reviewer: Denis Bogdanov (12.07.2019)
//
#pragma once

namespace openEdr {
namespace variant {
namespace detail {

//
// Optimized dictionary string key
// Main aim: minimize memory usage
//
class DictKey
{
public:
	typedef uint64_t Hash;

private:
	// Data
	union 
	{
		Hash m_nHash; // DictKey stores hash
		char m_sStr[8]; // DictKey stores value (zero-end string)
	};

	//
	//
	//
	void createFromStr(std::string_view str, bool fPutToStorage);

	//
	// Tests that DictKey contains hash
	//
	bool checkIsHash() const
	{
		return m_sStr[sizeof(m_sStr) - 1] != 0;
	}

	//
	// Sets that DictKey contains hash
	//
	void setIsHash(bool fIsHash)
	{
		if (fIsHash)
			m_sStr[sizeof(m_sStr) - 1] |= 0x80;
		else
			m_sStr[sizeof(m_sStr) - 1] &= 0x7F;
	}

public:

	//
	// Default constructor
	//
	DictKey() : m_nHash(0) 
	{
	}

	//
	// Copy & move
	//
	DictKey(const DictKey& other) : m_nHash(other.m_nHash) {}
	
	DictKey& operator=(const DictKey& other)
	{
		m_nHash = other.m_nHash;
		return *this;
	}

	DictKey(DictKey&& other) noexcept : m_nHash(other.m_nHash) {}
	
	DictKey& operator=(DictKey&& other) noexcept
	{
		m_nHash = other.m_nHash;
		return *this;
	}

	//
	// Constructors from string
	//
	explicit DictKey(const std::string& str, bool fPutToStorage = false)
	{
		createFromStr(str, fPutToStorage);
	}
	explicit DictKey(std::string_view str, bool fPutToStorage = false)
	{
		createFromStr(str, fPutToStorage);
	}
	explicit DictKey(const char* str, bool fPutToStorage = false)
	{
		createFromStr(str, fPutToStorage);
	}

	//
	// Compare for map and unordered map
	//
	bool operator==(const DictKey other) const
	{
		return other.m_nHash == m_nHash;
	}
	bool operator!=(const DictKey other) const
	{
		return other.m_nHash != m_nHash;
	}
	bool operator<(const DictKey other) const
	{
		return other.m_nHash < m_nHash;
	}

	//
	// Hash for unordered map
	//
	std::size_t getStdHash() const
	{
		static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8, "Unsupported size of size_t");

		if constexpr (sizeof(size_t) == 8)
		{
			return m_nHash;
		}
		else // sizeof(size_t) == 4
		{
			return size_t(m_nHash) ^ size_t(m_nHash >> 32);
		}
	}

	//
	// Converts to std::string
	//
	std::string convertToStr() const;
};

//
// Auto test DictKey size
//
static_assert(sizeof(DictKey) == 8, "Invalid size of DictKey");

///
/// Global storage of dictionary keys
///
class DictKeyStorage
{
private:
	mutable std::shared_mutex m_mtxStorage;
	std::unordered_map<DictKey::Hash, std::string> m_mapStorage;

public:
	DictKeyStorage() = default;
	DictKeyStorage(const DictKeyStorage&) = delete;
	DictKeyStorage& operator=(const DictKeyStorage&) = delete;
	DictKeyStorage(DictKeyStorage&&) = delete;
	DictKeyStorage& operator=(DictKeyStorage&&) = delete;

	//
	// Add string into storage
	//
	void addString(DictKey::Hash nHash, std::string_view str)
	{
		// Check existence
		{
			std::shared_lock lock(m_mtxStorage);
			// Ignore possibility string with same hash
			if (m_mapStorage.find(nHash) != m_mapStorage.end())
				return;
		}

		// Add new
		{
			std::unique_lock lock(m_mtxStorage);
			m_mapStorage[nHash] = str;
		}
	}

	//
	// get string from storage
	//
	std::string getString(DictKey::Hash nHash) const
	{
		std::shared_lock lock(m_mtxStorage);
		auto iter = m_mapStorage.find(nHash);
		if (iter != m_mapStorage.end())
			return iter->second;

		// If hash is not found throw exception
		error::InvalidArgument(SL, FMT("Hash <" << openEdr::hex(nHash) << "> is not found")).throwException();
	}
};

//
// DictKeyStorage declared in variant.cpp
// We can't use inline variable because MSVC bug, which fixed in MSVS 2019 only.
//
extern DictKeyStorage g_DictKeyStorage;

//
//
//
inline void DictKey::createFromStr(std::string_view str, bool fPutToStorage)
{
	// create from raw str
	if (str.size() <= sizeof(m_sStr) - 1)
	{
		m_nHash = 0;
		memcpy(m_sStr, str.data(), str.size());
		setIsHash(false);
		return;
	}

	// create from raw hash
	m_nHash = crypt::xxh::xxhash<64>(str.data(), str.size());
	setIsHash(true);

	// Put to storage
	if (fPutToStorage)
		g_DictKeyStorage.addString(m_nHash, str);
}

//
//
//
inline std::string DictKey::convertToStr() const
{
	if (!checkIsHash())
		return m_sStr;
	return g_DictKeyStorage.getString(m_nHash);
}

} // namespace detail
} // namespace variant
} // namespace openEdr

// custom specialization of std::hash for DictKey

namespace std {

//
// Custom specialization of std::hash for DictKey
// For usage inside std::unordered_map
//
template<> struct hash<openEdr::variant::detail::DictKey>
{
    typedef openEdr::variant::detail::DictKey argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const noexcept
    {
		return s.getStdHash();
    }
};

} // namespace std 
