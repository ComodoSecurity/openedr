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
namespace policy {

/// 
/// Enum class for policy groups.
///
enum class PolicyGroup : std::uint8_t
{
	Undefined = 0,
	PatternsMatching = 1,
	EventsMatching = 2,

	AutoDetect = UINT8_MAX
};

///
/// Policy compiler interface.
///
class IPolicyCompiler : OBJECT_INTERFACE
{

public:

	///
	/// Compile a policy.
	///
	/// @param group - a policy group.
	/// @param vData - a sequence of policies to compile.
	/// @param fActivate - do the policy activation after successful compiling.
	/// @return The function returns a variant value with compiled policy scenario.
	///
	virtual Variant compile(PolicyGroup group, Variant vData, bool fActivate) = 0;
};

// Forward declaration
class PolicySourceLocation;

///
/// Policy compile errors wrapper class.
///
class CompileError : public error::RuntimeError
{
private:
	std::exception_ptr getRealException() override;
	std::string createWhat(const PolicySourceLocation& policySrcLoc,  std::string_view sMsg);

public:
	///
	/// Constructor.
	///
	/// @param srcLoc - a structure with source file and line.
	/// @param policySrcLoc - a structure with policy source location information.
	/// @param sMsg [opt] - an error description (default: "").
	///
	CompileError(const SourceLocation srcLoc, const PolicySourceLocation& policySrcLoc,
		std::string_view sMsg = "");
	CompileError(const SourceLocation srcLoc, PolicySourceLocation&& policySrcLoc,
		std::string_view sMsg = "");

	///
	/// Constructor.
	///
	/// @param srcLoc - a structure with source file and line.
	/// @param sMsg [opt] - an error description (default: "").
	///
	CompileError(const SourceLocation srcLoc, std::string_view sMsg = "");

	///
	/// Constructor.
	///
	/// @param sMsg [opt] - an error description (default: "").
	///
	CompileError(std::string_view sMsg = "");

	///
	/// Destructor.
	///
	virtual ~CompileError();

	//
	//
	//
	bool isBasedOnCode(ErrorCode errCode) const override
	{
		if (errCode == error::ErrorCode::CompileError)
			return true;
		return error::RuntimeError::isBasedOnCode(errCode);
	}
};

//
//
//
class PolicySourceLocation
{
private:
	std::string m_sPolicyName;
	std::string m_sPatternName;
	std::string m_sChainName;
	std::string m_sVariableName;
	Size m_nRuleNumber = 0;
	Size m_nBindingNumber = 0;

	friend CompileError;

public:
	PolicySourceLocation(std::string_view sPolicyName, std::string_view sPatternName,
		std::string_view sChainName, Size nRuleNumber, Size nBindingNumber, std::string_view sVariableName);

	PolicySourceLocation() = default;
	PolicySourceLocation(const PolicySourceLocation&) = default;
	PolicySourceLocation(PolicySourceLocation&&) = default;
	PolicySourceLocation& operator=(const PolicySourceLocation&) = default;
	PolicySourceLocation& operator=(PolicySourceLocation&&) = default;

	void clear();
	void clearRule();
	void clearBinding();
	void enterPolicy(std::string_view sName);
	void leavePolicy();
	void enterPattern(std::string_view sName);
	void leavePattern();
	void enterChain(std::string_view sName);
	void leaveChain();
	void enterRule();
	void enterBinding();
	void enterVariable(std::string_view sName);
	void leaveVariable();
};

} // namespace policy
} // namespace edr

namespace error {
using CompileError = edr::policy::CompileError;
} // namespace error

} // namespace openEdr

///
/// Compile block begin macro
///
#define CMD_CATCH_COMPILE(location) \
	catch (const openEdr::error::CompileError&) { throw; } \
	catch (const openEdr::error::Exception& e) { \
		openEdr::error::CompileError(SL, location, e.what()).throwException(); \
	}

/// @} 
