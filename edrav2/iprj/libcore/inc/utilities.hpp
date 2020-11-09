//
// edrav2.libcore project
//
// Author: Denis Bogdanov (07.02.2019)
// Reviewer: Denis Bogdanov (22.02.2019)
//
///
/// @file Utilities
///
/// @addtogroup commonFunctions Common functions
/// @{

#pragma once

namespace openEdr {

///
/// A template-class for RAII-like resource management.
///
template<typename ResourceTraits>
class UniqueResource 
{
private:
	typedef typename ResourceTraits::ResourceType RT;
	static inline const auto c_defVal = ResourceTraits::c_defVal;

	RT m_rsrc{ c_defVal };

	void cleanup()
	{
		if (m_rsrc == c_defVal) return;
		ResourceTraits::cleanup(m_rsrc);
		m_rsrc = c_defVal;
	}

public:
	UniqueResource() noexcept = default;

	// Denies copy
	UniqueResource(const UniqueResource&) = delete;
	UniqueResource& operator=(const UniqueResource&) = delete;

	///
	/// Explicit initialization.
	///
	explicit UniqueResource(RT rsrc) noexcept :
		m_rsrc{ rsrc } 
	{}

	///
	/// Moves resource and its ownership.
	///
	UniqueResource(UniqueResource&& other) noexcept :
		m_rsrc{ other.release() }
	{
	}

	///
	/// Moves resource and its ownership.
	///
	UniqueResource& operator=(UniqueResource&& other) noexcept
	{
		assert(this != std::addressof(other));
		cleanup();
		m_rsrc = other.release();
		return *this;
	}

	~UniqueResource() noexcept
	{
		cleanup();
	}

	///
	/// Returns true if the owned resource != default value.
	///
	operator bool() noexcept
	{
		return m_rsrc != c_defVal;
	}

	///
	/// Returns the owned resource.
	///
	operator const RT&() const noexcept
	{
		return m_rsrc;
	}

	///
	/// Cleans the resource and returns the address of internal "empty" resource.
	///
	RT* operator&() noexcept
	{
		cleanup();
		return &m_rsrc;
	}

	///
	/// Returns the owned resource. 
	///
	RT get() noexcept
	{
		return m_rsrc;
	}

	///
	/// Releases the ownership of the managed resource and return it. 
	///
	RT release() noexcept
	{
		RT t = m_rsrc;
		m_rsrc = c_defVal;
		return t;
	}

	///
	/// Resets the resource.
	///
	void reset(RT rsrc = c_defVal) noexcept
	{
		cleanup();
		m_rsrc = rsrc;
	}
};

#ifndef CMD_SKIP_FRAMEWORK

///
/// Parse version string to variant with major, minor, fix, build fields.
///
inline Variant parseVersion(const std::string& sVersion)
{
	Dictionary vData;
	if (sVersion.empty()) return vData;

	const char* pBegin = sVersion.c_str();
	const char* pEnd = pBegin + sVersion.size();

	// Major
	char* pCurrEnd = const_cast<char*>(pBegin);
	Size nMajor = std::strtol(pCurrEnd, &pCurrEnd, 10);
	vData.put("major", nMajor);
	if (*pCurrEnd != '.') return vData;

	// Minor
	Size nMinor = std::strtol(pCurrEnd + 1, &pCurrEnd, 10);
	vData.put("minor", nMinor);
	if (*pCurrEnd != '.') return vData;

	// Revision
	Size nRevision = std::strtol(pCurrEnd + 1, &pCurrEnd, 10);
	vData.put("revision", nRevision);
	pCurrEnd = std::find(pCurrEnd, const_cast<char*>(pEnd), '.');
	if (pCurrEnd == pEnd) return vData;

	// Build
	Size nBuild = std::strtol(pCurrEnd + 1, &pCurrEnd, 10);
	vData.put("build", nBuild);
	return vData;
}

///
/// Compare two version descriptors (see parseVersion()).
///
/// @param v1 - first version descriptor
/// @param v2 - second version descriptor
///
/// @return 0 - if versions are equal, -1 if v1 lesser v2, 1 if v1 greater v2
///
inline int compareVersion(Variant v1, Variant v2)
{
	if ((Size)v1.get("major", 0) == (Size)v2.get("major", 0) &&
		(Size)v1.get("minor", 0) == (Size)v2.get("minor", 0) &&
		(Size)v1.get("revision", 0) == (Size)v2.get("revision", 0) &&
		(Size)v1.get("build", 0) == (Size)v2.get("build", 0))
		return 0;

	if ((Size)v1.get("major", 0) > (Size)v2.get("major", 0) ||
		((Size)v1.get("major", 0) == (Size)v2.get("major", 0) &&
			(Size)v1.get("minor", 0) > (Size)v2.get("minor", 0)) ||
		((Size)v1.get("major", 0) == (Size)v2.get("major", 0) &&
			(Size)v1.get("minor", 0) == (Size)v2.get("minor", 0) &&
			(Size)v1.get("revision", 0) > (Size)v2.get("revision", 0)) ||
		((Size)v1.get("major", 0) == (Size)v2.get("major", 0) &&
			(Size)v1.get("minor", 0) == (Size)v2.get("minor", 0) &&
			(Size)v1.get("revision", 0) == (Size)v2.get("revision", 0) &&
			(Size)v1.get("build", 0) > (Size)v2.get("build", 0)))
		return 1;

	return -1;
}

//
//
//
inline int compareVersion(const std::string& v1, const std::string& v2)
{
	return compareVersion(parseVersion(v1), parseVersion(v2));
}

//
//
//
inline Time getCurrentTime()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
}

//
//
//
inline Time getTickCount()
{
#ifdef PLATFORM_WIN
	return std::chrono::milliseconds(::GetTickCount64()).count();
#else
#error Add other OS implementations.
	// Add *NIX code here
#endif
}

//
//
//
inline std::string getIsoTime(Time time)
{
	time_t cnow = time / 1000;
	auto mil = time % 1000;
	tm tmData;
	gmtime_s(&tmData, &cnow);
	return FMT(std::put_time(&tmData, "%Y-%m-%dT%H:%M:%S.") <<
		std::setfill('0') << std::setw(3) << mil << "Z");
}

#endif // CMD_SKIP_FRAMEWORK
} // namespace openEdr

/// @}
