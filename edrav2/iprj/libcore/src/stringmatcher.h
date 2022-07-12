//
// edrav2.libcore project
//
// Author: Yury Ermakov (27.02.2019)
// Reviewer: Denis Bogdanov (06.03.2019)
//
///
/// @file StringMatcher object declaration
///
/// @addtogroup basicDataObjects Basic data objects
/// @{

#pragma once
#include <objects.h>
#include <common.hpp>
#include <command.hpp>

namespace cmd {
namespace string {

///
/// Class for matching and replacing strings 
///
class StringMatcher : public ObjectBase<CLSID_StringMatcher>, 
	public ICommandProcessor,
	public IStringMatcher
{
private:

	//
	//
	//
	struct Rule
	{
		Variant::StringValuePtr sTarget;
		Variant::StringValuePtr sValue;
		variant::operation::MatchMode mode;

		Rule(Variant::StringValuePtr sTarget, Variant::StringValuePtr sValue,
			variant::operation::MatchMode mode) :
			sTarget(sTarget), sValue(sValue), mode(mode) {}
	};

	std::vector<Rule> m_vecPlainRules;
	std::vector<Rule> m_vecPlainRulesCI;

	void addRule(const Variant& vRule);
	Variant convertToLower(Variant::StringValuePtr pStrValue) const;
	template <typename CharT>
	bool matchPlain(std::basic_string_view<CharT> sValue, const std::vector<Rule>& vecRules) const;
	template <typename CharT>
	std::basic_string<CharT> replacePlain(std::basic_string_view<CharT> sValue) const;

public:

	///
	/// Object's final construction
	///
	/// @param vConfig the object's configuration including the following fields:
	///   **schema** - a dictionary or a sequence of dictionaries with rules
	///     @sa cmd::variant::operation::replace()
	/// @throw error::InvalidArgument if configuration is not valid.
	///
	void finalConstruct(Variant vConfig);

	// IStringMatcher

	///
	/// @copydoc IStringMatcher::match(std::string_view,bool)
	///
	bool match(std::string_view sValue, bool fCaseInsensitive) const override;

	///
	/// @copydoc IStringMatcher::match(std::wstring_view,bool)
	///
	bool match(std::wstring_view sValue, bool fCaseInsensitive) const override;

	///
	/// @copydoc IStringMatcher::replace(std::string_view)
	///
	std::string replace(std::string_view vValue) const override;

	///
	/// @copydoc IStringMatcher::replace(std::wstring_view)
	///
	std::wstring replace(std::wstring_view vValue) const override;

	// ICommandProcessor

	///
	/// @copydoc ICommandProcessor::execute()
	///
	virtual Variant execute(Variant vCommand, Variant vParams) override;

};

} // namespace string
} // namespace cmd

/// @}
