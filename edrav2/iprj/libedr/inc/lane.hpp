//
// edrav2.libedr project
// 
// Author: Yury Ermakov (16.04.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
///
/// @file Policy interface declaration
///
/// @addtogroup edr
/// @{
#pragma once

namespace openEdr {
namespace edr {

///
/// Lane types.
///
/// Lane types are comparable (more/less), but don't have exact numeric values (possible be changed).
/// Lane types have string analogs.
///
enum class LaneType : uint32_t
{
	Fast,
	SlowLocalFS,
	SlowNetwork,

	Undefined = 0xFFFF,
};

///
/// Check that eLaneType is defined.
///
/// @param eLaneType - a type of the lane.
/// @returns The function returns `true` if the lane type is valid.
///
bool isValidLaneType(LaneType eLaneType);

///
/// Converts LaneType to string.
///
/// @param eLaneType - a type of the lane.
/// @return The function returns a lane type description or
///	blank string ("") if LaneType is not found or Undefined.
///
std::string_view convertLaneToString(LaneType eLaneType);

///
/// Converts string to LaneType.
///
/// @param sLaneType - lane type string.
/// @return The function returns a corresponding lane type or 
/// `LaneType::Undefined` if LaneType not found.
///
LaneType convertStringToLane(std::string_view sLaneType);

///
/// Converts Variant to LaneType.
///
/// @param vLaneType - lane type (Int and String values).
/// @returns The function returns a corresponding lane type.
/// @throw InvalidArgument is LaneType is invalid.
///
LaneType convertVariantToLane(Variant vLaneType);

///
/// Returns LaneType of the current thread.
///
/// @returns The function returns a value from LaneType enum.
///
LaneType getCurLane();

///
/// Sets LaneType for the current thread.
///
/// @param eLaneType - a lane type.
///
void setCurLane(LaneType eLaneType);

///
/// Checks LaneType of the current thread.
///
/// @param eLaneType - requested lane.
/// @param sTag - operation tag. It is used in exception and statistic.
/// @param srcLoc - a source location.
/// @throw SlowLaneOperation if current LaneType less than requsted LaneType
///
void checkCurLane(LaneType eLaneType, std::string_view sTag, SourceLocation srcLoc);

///
/// This exception is occured when slow operation is executed from the fast lane.
///
class SlowLaneOperation : public error::RuntimeError
{
public:
	typedef error::RuntimeError Base;

private:
	std::string m_sTag; //< Tag which is passed to checkCurLane
	LaneType m_eThreadLaneType; //< Thread LaneType during request
	LaneType m_eRequestedLaneType; //< Requested LaneType

	std::exception_ptr getRealException() override
	{
		return std::make_exception_ptr(*this);
	}

	void incStat() const;

public:

	///
	/// Constructor.
	///
	SlowLaneOperation(LaneType eThreadLaneType, LaneType eRequestedLaneType, 
		std::string_view sTag, SourceLocation srcLoc) :
		error::RuntimeError(srcLoc, error::ErrorCode::SlowLaneOperation, sTag), m_eThreadLaneType(eThreadLaneType),
		m_eRequestedLaneType(eRequestedLaneType), m_sTag(sTag)
	{
		incStat();
	}

	///
	/// Destructor.
	///
	virtual ~SlowLaneOperation() {}

	///
	/// Serialization.
	///
	/// @returns The function returns a data packet with the following fields:
	///   * curLane [int] - current thread lane.
	///   * needLane [int] - requested lane.
	///   * curLaneStr [str] - current thread lane.
	///   * needLaneStr [str] - requested lane.
	///   * tag [str] - tag of request.
	///
	Variant serialize() const override
	{
		Variant vInfo = Base::serialize();
		vInfo.put("curLane", m_eThreadLaneType);
		vInfo.put("needLane", m_eRequestedLaneType);
		vInfo.put("curLaneStr", convertLaneToString(m_eThreadLaneType));
		vInfo.put("needLaneStr", convertLaneToString(m_eRequestedLaneType));
		vInfo.put("tag", m_sTag);
		return vInfo;
	}

	//
	//
	//
	LaneType getRequestedLaneType() const
	{
		return m_eRequestedLaneType;
	}

	//
	//
	//
	LaneType getThreadLaneType() const
	{
		return m_eThreadLaneType;
	}

	//
	//
	//
	std::string_view getTag() const
	{
		return m_sTag;
	}

	//
	//
	//
	bool isBasedOnCode(ErrorCode errCode) const override
	{
		if (errCode == error::ErrorCode::SlowLaneOperation)
			return true;
		return Base::isBasedOnCode(errCode);
	}
};

} // namespace edr

namespace error {
using SlowLaneOperation = edr::SlowLaneOperation;
} // namespace error

} // namespace openEdr

/// @} 
