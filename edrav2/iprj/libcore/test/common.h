//
// Common test utility functions
//
#pragma once

namespace openEdr {

//
//
//
template <class T>
inline ObjPtr<T> createTestObject(ClassId clsid, Variant vConfig)
{
	return queryInterface<T>(createObject(clsid, vConfig));
}

//
//
//
inline std::string getTestName()
{
	std::string sTestName = Catch::getResultCapture().getCurrentTestName();
	// Catch2 replaces '.' to ',' in getCurrentTestName() call, so we do replace back
	std::replace(sTestName.begin(), sTestName.end(), ',', '.');
	return sTestName;
}

//
// Get current test data path
//
inline std::filesystem::path getTestDataPath()
{
	return std::filesystem::current_path() / string::convertUtf8ToWChar(getTestName());
}

//
//
//
inline std::filesystem::path getTestTempPath()
{
	auto t = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	std::string sName("ats.temp-");
	sName += std::to_string(t);
	auto sPath = std::filesystem::temp_directory_path() / sName;
	std::filesystem::create_directories(sPath);
	return sPath;
}

//
//
//
inline std::filesystem::path initTest()
{
	// TODO: Remove string below when logging inifialization subsystem is fixed
	return getTestDataPath();
}

//
// Minimal data receiver
//
class TestDataReceiver : public ObjectBase<>,
	public IDataReceiver
{
	Size m_nCount = 0;
public:

	Size getCount()
	{
		return m_nCount;
	}

	virtual void put(const Variant& vData) override
	{
		++m_nCount;
	}
};

//
//
//
inline Variant generateData(Size nId)
{
	return Dictionary({
		{"id", nId},
		{"size", 6},
		{"b", true},
		{"s", "string"},
		{"d", Dictionary({ {"a", 1} })},
		{"q", Sequence({ 1, 2, 3})}
		});
}

//
//
//
inline uint64_t putDataRange(ObjPtr<IDataReceiver> pObj, Size nStart, Size nEnd)
{
	uint64_t nPutSum = 0;
	for (Size nCounter = nStart; nCounter < nEnd; ++nCounter)
	{
		pObj->put(generateData(nCounter));
		nPutSum += nCounter;
	}
	return nPutSum;
};

//
// Function reads data from privider until the stop flag is appeared and
// the source returned no data
//
inline uint64_t getData(ObjPtr<IDataProvider> pObj, std::atomic<bool>& fStop)
{
	uint64_t nGetSum = 0;
	while (true)
	{
		auto v = pObj->get();
		if (v.has_value())
		{
			if (v.value().has("id"))
				nGetSum += Size(v.value()["id"]);
			if (v.value()["size"] != v.value().getSize())
				throw error::InvalidArgument("Exception from test");
		}
		else if (fStop) break;
	}
	return nGetSum;
};

//
// Class automatically set flag then destroyed
//
class AutoSetFlag
{
	std::atomic<bool>& m_flag;
public:
	AutoSetFlag(std::atomic<bool>& flag) : m_flag(flag) {}
	~AutoSetFlag() { m_flag = true; }
};

//
// Generate a UTC ISO8601-formatted timestamp
// and return as std::string
//
// @param nShift (Optional) shift from current time (in milliseconds)
//   By defalft, no shift is applied.
// @return Returns a string with timestamp.
//
inline std::string getCurrentISO8601TimeUTC(int nShift = 0)
{
	auto now = std::chrono::system_clock::now();
	if (nShift != 0)
		now += std::chrono::milliseconds(nShift);
	auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	auto fraction = now - seconds;
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(fraction);
	auto itt = std::chrono::system_clock::to_time_t(now);

	tm tmTime;
	gmtime_s(&tmTime, &itt);
	std::ostringstream ss;
	ss << std::put_time(&tmTime, "%FT%T.") <<
		std::setfill('0') << std::setw(3) << milliseconds.count() << "Z";
	return ss.str();
}

} // namespace openEdr