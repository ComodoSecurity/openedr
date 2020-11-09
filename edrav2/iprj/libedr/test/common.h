//
// edrav2.ut_libcloud project
//
// Author: Yury Ermakov (15.03.2019)
// Reviewer:
//
#pragma once

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
