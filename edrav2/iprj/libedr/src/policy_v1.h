//
// edrav2.libedr project
// 
// Author: Yury Ermakov (16.04.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
///
/// @file Policy compiler (legacy) declaration
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
namespace v1 {

//
//
//
class RuleImpl : public Rule
{
private:
	OperationPtr m_pCondition;
	std::optional<Size> m_optBaseEventType;
	Variant m_vEventType;
	std::string m_sType;

	Operation getOperation(std::string_view sName) const;
	void parseCondition(const Variant vData, Context& ctx);
	Variant parseOperation(const Variant& vData, Context& ctx);

public:

	static RulePtr create();

	// RuleBase

	void parse(const Variant& vData, Context& ctx) override;
	Variant compile(Context& ctx) override;
};

//
//
//
class Parser: public IParser
{
private:
	Variant m_vEventConfig;
	ObjPtr<IStringMatcher> m_pRegistryReplacer;
	ObjPtr<IStringMatcher> m_pFileReplacer;

	RulePtr parseRule(const Variant& vData, Context& ctx);
	Variant loadEventConfig(const Variant& vEventGroups, const Variant& vEventConfig) const;
	void parseEvent(std::string_view sCode, Variant vSource, Context& ctx);

public:
	static ParserPtr create(Variant vConfig);

	// IParser
	void parse(PolicyPtr pPolicy, Context& ctx) override;
};

} // namespace v1
} // namespace policy
} // namespace edr
} // namespace cmd

/// @} 
