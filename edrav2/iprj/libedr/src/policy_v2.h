//
// edrav2.libedr project
// 
// Author: Yury Ermakov (20.08.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
///
/// @file Policy compiler declaration
///
/// @addtogroup edr
/// @{
#pragma once
#include <objects.h>
#include <policy.hpp>
#include "policy.h"
#include "policyoperation.h"

namespace cmd {
namespace edr {
namespace policy {
namespace v2 {

#undef CreateEvent

/// 
/// Enum class for directives.
///
enum class Directive : std::uint8_t
{
	Undefined = 0,
	Condition = 1,
	LoadContext = 2,
	SaveContext = 3,
	DeleteContext = 4,
	CreateEvent = 5,
	Discard = 6,
	Debug = 7,
};

///
/// Get directive name string by given code.
///
/// @param directive - a code of the directive.
/// @return The function returns a string with a name of the directive 
/// corresponding to a given code.
///
const char* getDirectiveString(Directive directive);

///
/// Get directive code by given name.
///
/// @param sDirective - a name of the directive.
/// @return The function returns a code of the directive corresponding
/// to a given name.
///
Directive getDirectiveFromString(std::string_view sDirective);

//
//
//
Variant createOperation(Variant& vData, Context& ctx);

//
//
//
struct VariableImpl : public Variable
{
private:
	OperationPtr m_pOperation;
	Variant m_vData;

public:
	static VariablePtr create(std::string_view sName);

	// Variable

	void parse(const Variant& vData, Context& ctx) override;
	Variant compile(Context& ctx) const override;
};

//
//
//
struct RuleImpl : public Rule
{
public:
	std::unordered_map<Directive, DirectivePtr> directives;

	static RulePtr create();

	// Rule

	void parse(const Variant& vData, Context& ctx) override;
	Variant compile(Context& ctx) override;
};

//
//
//
class Parser: public IParser
{
private:
	void parseConstants(const Variant& vData, Context& ctx) const;
	void parseConstDict(const Variant& vData, Context& ctx, const std::string& sPath) const;
	void parseConstSeq(const Variant& vData, Context& ctx, const std::string& sPath) const;
	void parseVariables(const Variant& vData, Context& ctx) const;
	void parseChains(const Variant& vData, Context& ctx) const;
	void parseBindings(const Variant& vData, Context& ctx) const;
	void parsePatterns(const Variant& vData, Context& ctx) const;
	RulePtr parseRule(const Variant& vData, Context& ctx) const;

public:
	static ParserPtr create();

	// IParser
	void parse(PolicyPtr pPolicy, Context& ctx) override;
};

} // namespace v2
} // namespace policy
} // namespace edr
} // namespace cmd

/// @} 
