//
// edrav2.libedr project
// 
// Author: Yury Ermakov (13.11.2019)
// Reviewer:
//
///
/// @file Output filter declaration
///
/// @addtogroup edr
/// @{
#pragma once
#include <objects.h>

namespace openEdr {

///
/// Output filter.
///
class OutputFilter : public ObjectBase<CLSID_OutputFilter>,
	public ICommandProcessor
{
private:
	using RegexPtr = std::shared_ptr<std::regex>;
	using RegexMap = std::unordered_map<std::string, RegexPtr>;
	using LinkedGroups = std::unordered_set<std::string>;

	RegexMap m_regexMap;

	void addRegex(std::string_view sType, RegexPtr pRegex);
	RegexPtr getRegex(const std::string& sType) const;
	RegexPtr parseEvent(Variant vData, const Variant& vEvents, LinkedGroups linkedGroups = {});
	Variant filter(const std::string& sType, Variant vData) const;

protected:
	/// Constructor
	OutputFilter();
	
	/// Destructor
	~OutputFilter() override;

public:

	///
	/// Final construct.
	///
	/// @param vConfig - object's configuration including the following fields:
	///   events [dict] - a dictionary of filtering pattern groups. Keys correspond to a particular event type.
	///     If no key with specified event type is found, the 'default' is used. Values can be a string with
	///     the name of already defined group, or a sequence of regex-expressions that are intendend to match
	///     a specific paths in the incoming data dictionary. The extended POSIX regular expression grammar
	///     is only supported.
	///
	void finalConstruct(Variant vConfig);

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute(Variant,Variant)
	Variant execute(Variant vCommand, Variant vParams) override;
};

} // namespace openEdr

/// @} 
