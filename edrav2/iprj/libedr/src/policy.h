//
// edrav2.libedr project
// 
// Author: Yury Ermakov (20.08.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
///
/// @file Policy internals declaration
///
/// @addtogroup edr
/// @{
#pragma once
#include <policy.hpp>

namespace openEdr {
namespace edr {
namespace policy {

constexpr Size c_nVersion = 2;
constexpr Size c_nDefaultPriority = 1000;

/// 
/// Enum class for sections.
///
enum class Section : std::uint8_t
{
	Undefined = 0,
	Const = 1,			///< constant
	Var = 2,			///< variable
	Event = 3,			///< event
	Context = 4,		///< context
	Rule = 5,			///< rule
	Policy = 6,			///< policy

};

///
/// Get a section name string by given code.
///
/// @param section - a code of the section.
/// @return The function returns a string with a name of the section
///	corresponding to a given code.
///
const char* getSectionString(Section section);

///
/// Get section code by given name.
///
/// @param sSection - a name of the section.
/// @return The function returns a code of the section corresponding 
/// to a given name.
///
Section getSectionFromString(std::string_view sSection);

///
/// Get policy group name string by given code.
///
/// @param group - a code of the policy group.
/// @return The function returns a string with a name of the policy 
/// group corresponding to a given code.
///
const char* getPolicyGroupString(PolicyGroup group);

///
/// Get policy group short name string by given code.
///
/// @param group - a code of the policy group.
/// @return The function returns a string with a short name of the policy 
/// group corresponding to a given code.
///
const char* getPolicyGroupShortString(PolicyGroup group);

///
/// Get policy group code by given name.
///
/// @param sGroup - a name of the policy group.
/// @return The function returns a code of the policy group corresponding 
/// to a given name.
///
PolicyGroup getPolicyGroupFromString(std::string_view sGroup);

/// 
/// Enum class for rule states.
///
enum class RuleState : std::uint8_t
{
	Matched = 0,
	CheckContextNotFound = 1,
	FalseCondition = 2,
	LoadContextNotFound = 3,
	Undefined = UINT8_MAX
};

/// 
/// Enum class for destination (eventHandler.call.createEvent).
///
enum class Destination : std::uint8_t
{
	Undefined = 0,
	In = 1,
	Out = 2,
	InOut = 3
};

///
/// Get destination name string by given code.
///
/// @param destination - a code of the destination.
/// @return The function returns a string with a name of the destination 
/// corresponding to a given code.
///
const char* getDestinationString(Destination destination);

///
/// Get destination code by given name.
///
/// @param sDestination - a name of the destination.
/// @return The function returns a code of the destination corresponding 
/// to a given name.
///
Destination getDestinationFromString(std::string_view sDestination);

// Forward declaration
struct Context;

//
//
//
class IDirective
{
public:
	virtual ~IDirective() {};

	virtual void parse(const Variant& vData, Context& ctx) = 0;
	virtual bool hasAccessToContext() const { return false; }
	virtual Variant compile([[maybe_unused]] Context& ctx) const { return Sequence(); }
};
using DirectivePtr = std::shared_ptr<IDirective>;

//
//
//
struct Variable
{
	std::string sName;

	virtual void parse(const Variant& vData, Context& ctx) = 0;
	virtual Variant compile([[maybe_unused]] Context& ctx) const { return Sequence(); }
};
using VariablePtr = std::shared_ptr<Variable>;

//
//
//
struct Rule
{
	Size nPriority = c_nDefaultPriority;
	Size nVersion = c_nVersion;

	virtual void parse(const Variant& vData, Context& ctx) = 0;
	virtual Variant compile(Context& ctx) = 0;
};

using RulePtr = std::shared_ptr<Rule>;
using EventRuleMap = std::map<std::string, std::multimap<Size, RulePtr, std::greater<Size>>>;
using RuleMap = std::multimap<Size, RulePtr, std::greater<Size>>;

//
//
//
struct Chain
{
private:
	RuleMap m_rules;

public:
	std::string sName;

	Chain(std::string_view _sName) :
		sName(_sName)
	{}

	//
	//
	//
	static std::shared_ptr<Chain> create(std::string_view sName)
	{
		return std::make_shared<Chain>(sName);
	}

	void addRule(RulePtr pRule);
	Variant compileBody(Context& ctx) const;
	Variant compileCall(Context& ctx) const;
	const RuleMap getRules() const;
};

using ChainPtr = std::shared_ptr<Chain>;
using EventChainMap = std::map<Event, std::multimap<Size, ChainPtr, std::greater<Size>>>;

//
//
//
struct Pattern
{
private:
	EventRuleMap m_rules;

public:
	std::string sName;

	Pattern(std::string_view sName_) :
		sName(sName_)
	{}

	//
	//
	//
	static std::shared_ptr<Pattern> create(std::string_view sName)
	{
		return std::make_shared<Pattern>(sName);
	}

	void addRule(const std::string& sEventType, RulePtr pRule);
	const EventRuleMap getRules() const;
	//Variant compile(Context& ctx);
};

using PatternPtr = std::shared_ptr<Pattern>;


//
// Single policy class
//
struct Policy
{
	Size nVersion = 0;
	std::string sName;
	std::vector<std::string> dependencies;
	PolicyGroup group = PolicyGroup::Undefined;
	Size nPriority = c_nDefaultPriority;
	Variant vData;
	Variant vConstants = Dictionary();
	bool fAllLinksResolved = false;
	bool fParsed = false;

	static std::shared_ptr<Policy> create(std::string_view sName, Variant vData);
};

using PolicyPtr = std::shared_ptr<Policy>;

//
//
//
struct Context
{
private:
	Variant m_vObjects;
	Variant m_vEventClasses;
	std::unordered_map<std::string, VariablePtr> m_variables;

	void resolveValueDict(Variant vData, const std::string& sPath, bool fRuntime);
	void resolveValueSeq(Variant vData, const std::string& sPath, bool fRuntime);
	std::optional<Variant> getConstValue(std::string_view sName, bool fRuntime, bool fRecursive);
	std::optional<Variant> getVarValue(std::string_view sName, bool fRuntime);
	std::optional<Variant> getRuleValue(std::string_view sName, bool fRuntime);
	std::optional<Variant> getContextValue(std::string_view sName, bool fRuntime);
	std::optional<Variant> getEventValue(std::string_view sName, bool fRuntime) const;
	std::optional<Variant> getPolicyValue(std::string_view sName, bool fRuntime) const;
	std::optional<Variant> getObjectFieldInfo(Variant vRoot, const std::string_view& sPath) const;
	std::optional<Variant> getDefaultValue(std::string_view sPath) const;

public:
	bool fVerbose = false;

	// All policies-related data
	PolicyGroup policyGroup = PolicyGroup::Undefined;
	Variant vConstants = Dictionary();
	std::unordered_map<std::string, PolicyPtr> policies;
	std::unordered_map<std::string, Variant> runtimeConstants;
	std::unordered_map<std::string, ChainPtr> chains;
	std::unordered_map<std::string, PatternPtr> patterns;
	std::multimap<std::string, ChainPtr> bindings;

	// Current policy-related data
	PolicyPtr pPolicy;
	PolicySourceLocation location;

	// Policy v1 data
	std::string sEventCode;
	Variant vEventConfig;
	ObjPtr<IStringMatcher> pRegistryReplacer;
	ObjPtr<IStringMatcher> pFileReplacer;

	Context(Variant vObjects, Variant vEventClasses, bool fVerbose = false);

	bool isLink(std::string_view sValue) const;
	bool isLink(const Variant& vValue) const;
	Variant normalizeDictSchema(const Variant& vData) const;
	Variant resolveValue(const Variant& vValue, bool fRuntime = true);
	std::optional<Variant> getValueByLink(std::string_view sLink, bool fRuntime, bool fRecursive);

	void addPolicy(PolicyPtr pPolicy);
	void addChain(ChainPtr pChain);
	void addPattern(PatternPtr pPattern);
	void addBinding(const std::string& sEventType, ChainPtr pChain);
	void addVariable(VariablePtr pVariable);
	ChainPtr getChain(std::string_view sName) const;
	PolicyPtr getPolicy(std::string_view sName) const;
	VariablePtr getVariable(std::string_view sName) const;
};

//
//
//
class IParser
{
public:
	virtual void parse(PolicyPtr pPolicy, Context& ctx) = 0;
};

using ParserPtr = std::shared_ptr<IParser>;

} // namespace policy
} // namespace edr
} // namespace openEdr

/// @} 
