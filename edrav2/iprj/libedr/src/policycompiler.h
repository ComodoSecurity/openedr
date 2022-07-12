//
// edrav2.libedr project
// 
// Author: Yury Ermakov (20.08.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
///
/// @file Policy compiler declaration
///
/// @addtogroup edr
/// @{
#pragma once
#include <objects.h>
#include <policy.hpp>
#include "policy.h"

namespace cmd {
namespace edr {
namespace policy {

///
/// Policy compiler.
///
class PolicyCompiler: public ObjectBase<CLSID_PolicyCompiler>,
	public IPolicyCompiler,
	public ICommandProcessor
{
private:
	std::string m_sPolicyCatalogPath;
	bool m_fVerbose = false;
	Variant m_vConfig;

	std::unordered_map<Size, ParserPtr> m_parsers;

	void parsePolicy(PolicyPtr pPolicy, Context& ctx);
	ParserPtr createParser(Size nVersion) const;
	ParserPtr getParser(Size nVersion);
	void activate(PolicyGroup group, Variant vScenario) const;
	Variant compileConstants(Context& ctx) const;
	Variant compileChains(Context& ctx) const;
	Variant compileEventRules(Context& ctx) const;

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - a dictionary with the configuration including
	/// the following fields:
	///   **config** [variant] - compiler's configuration
	///
	void finalConstruct(Variant vConfig);

	// IPolicyCompiler

	/// @copydoc IPolicyCompiler::compile(PolicyGroup,Variant,bool)
	Variant compile(PolicyGroup group, Variant vData, bool fActivate) override;

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute(Variant,Variant)
	Variant execute(Variant vCommand, Variant vParams) override;
};

} // namespace policy
} // namespace edr
} // namespace cmd

/// @} 
