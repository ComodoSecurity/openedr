//
// edrav2.libcore project
//
// Author: Yury Ermakov (13.02.2019)
// Reviewer: Denis Bogdanov (25.02.2019)
//
///
/// @file Variant operations support
///
///
/// @addtogroup basicDataObjects Basic data objects
/// @{
#include "pch.h"
#include <command.hpp>
#include <common.hpp>
#include <objects.h>

namespace openEdr {
namespace variant {
namespace operation {

//
//
//
bool logicalNot(Variant vValue)
{
	return !convert<ValueType::Boolean>(vValue);
}

//
//
//
bool logicalAnd(Variant vValue1, Variant vValue2)
{
	return convert<ValueType::Boolean>(vValue1) == convert<ValueType::Boolean>(vValue2);
}

//
//
//
bool logicalOr(Variant vValue1, Variant vValue2)
{
	if (convert<ValueType::Boolean>(vValue1))
		return true;
	if (convert<ValueType::Boolean>(vValue2))
		return true;
	return false;
}

//
//
//
bool equal(Variant vValue1, Variant vValue2)
{
	if (vValue1.isDictionaryLike())
		return (vValue1 == convert<ValueType::Dictionary>(vValue2));
	if (vValue1.isSequenceLike())
		return (vValue1 == convert<ValueType::Sequence>(vValue2));
	switch (vValue1.getType())
	{
		case ValueType::Boolean: return (vValue1 == convert<ValueType::Boolean>(vValue2));
		case ValueType::Integer: return (vValue1 == convert<ValueType::Integer>(vValue2));
		case ValueType::String: return (vValue1 == convert<ValueType::String>(vValue2));
	}
	return (vValue1 == vValue2);
}

//
//
//
bool greater(Variant vValue1, Variant vValue2)
{
	return ((Variant::IntValue)convert<ValueType::Integer>(vValue1) > 
		(Variant::IntValue)convert<ValueType::Integer>(vValue2));
}

//
//
//
bool less(Variant vValue1, Variant vValue2)
{
	return ((Variant::IntValue)convert<ValueType::Integer>(vValue1) <
		(Variant::IntValue)convert<ValueType::Integer>(vValue2));
}

//
//
//
bool match(Variant vValue, Variant vPattern, bool fCaseInsensitive)
{
	if (vPattern.isNull())
		return !vValue.isNull();
	if (vValue.isNull())
		return false;

	if (!vPattern.isString() && !vPattern.isSequenceLike())
		return false;
	if (!vValue.isString() && !vValue.isSequenceLike())
		return false;
	if (vPattern.isEmpty() || (vPattern.isString() && std::string(vPattern).empty()))
		return true;

	auto eMode = std::regex_constants::basic; //::ECMAScript;
	if (fCaseInsensitive)
		eMode |= std::regex_constants::icase;

	if (vPattern.isString())
	{

		// FIXME: Regex compilation takes a lot of time. We shall create in on the 
		// first access and use context objects... We can discuss it
		std::regex regexExpr(std::string(vPattern), eMode);
		if (vValue.isString())
			return std::regex_search(std::string(vValue), regexExpr);
		else if (vValue.isSequenceLike())
		{
			for (const auto& vInnerValue : vValue)
				if (vInnerValue.isString() && 
					std::regex_search(std::string(vInnerValue), regexExpr))
					return true;
		}
		else
			error::InvalidArgument(SL, "Value must be a string or sequence").throwException();
	}
	else if (vPattern.isSequenceLike())
	{
		bool fEmpty = true;
		for (const auto& vInnerPattern : vPattern)
		{
			if (vInnerPattern.isEmpty() || (vInnerPattern.isString() && std::string(vInnerPattern).empty()))
				continue;
			if (fEmpty)
				fEmpty = false;
			std::regex regexExpr(std::string(vInnerPattern), eMode);
			if (vValue.isString())
			{
				if (std::regex_search(std::string(vValue), regexExpr))
					return true;
			}
			else if (vValue.isSequenceLike())
			{
				for (const auto& vInnerValue : vValue)
					if (vInnerValue.isString() && 
						std::regex_search(std::string(vInnerValue), regexExpr))
						return true;
			}
			else
				error::InvalidArgument(SL, "Value must be a string or sequence").throwException();
		}
		if (fEmpty)
			return true;
	}
	else
		error::InvalidArgument(SL, "Pattern must be a string or sequence").throwException();

	return false;
}

//
//
//
bool contain(std::string_view sValue1, std::string_view sValue2)
{
	return ((sValue1.empty() && sValue2.empty()) ||
		(!sValue2.empty() && (std::string::npos != sValue1.find(sValue2))));
}

//
//
//
bool isStringContain(std::string_view sValue1, const Variant& vValue2)
{
	if (vValue2.isSequenceLike())
	{
		for (const auto& vInnerValue2 : vValue2)
		{
			std::string sInnerValue2 = convert<ValueType::String>(vInnerValue2);
			if (contain(sValue1, sInnerValue2))
				return true;
		}
		return false;
	}
	std::string sValue2 = convert<ValueType::String>(vValue2);
	return contain(sValue1, sValue2);
}

//
//
//
bool contain(Variant vValue1, Variant vValue2)
{
	if (vValue1.isString())
		return isStringContain(std::string(vValue1), vValue2);

	if (vValue1.isSequenceLike())
	{
		for (const auto& vInnerValue1 : vValue1)
		{
			if (vValue2.isSequenceLike())
			{
				for (const auto& vInnerValue2 : vValue2)
					if (equal(vInnerValue1, vInnerValue2))
						return true;
			}
			else if (equal(vInnerValue1, vValue2))
				return true;
		}
	}
	else if (vValue2.isSequenceLike())
	{
		for (const auto& vInnerValue2 : vValue2)
			if (equal(vValue1, vInnerValue2))
				return true;
	}
	else if (equal(vValue1, vValue2))
		return true;

	return false;
}

//
//
//
Variant replace(Variant vValue, Variant vSchema)
{
	auto pStringMatcher = queryInterfaceSafe<IStringMatcher>(vSchema);
	if (!pStringMatcher)
		pStringMatcher = queryInterface<IStringMatcher>(createObject(CLSID_StringMatcher,
			Dictionary({ { "schema", vSchema } })));

	if (vValue.isString())
	{
		Variant::StringValuePtr pStrValue = vValue;
		if (pStrValue->hasWChar())
			return pStringMatcher->replace(std::wstring_view(*pStrValue));
		return pStringMatcher->replace(std::string_view(*pStrValue));
	}
	
	if (vValue.isSequenceLike())
	{
		Sequence seqResult;
		for (const auto& vInnerValue : vValue)
		{
			if (vInnerValue.isString())
			{
				Variant::StringValuePtr pStrValue = vInnerValue;
				if (pStrValue->hasWChar())
					seqResult.push_back(pStringMatcher->replace(std::wstring_view(*pStrValue)));
				else
					seqResult.push_back(pStringMatcher->replace(std::string_view(*pStrValue)));
			}
			else
				seqResult.push_back(vInnerValue);
		}
		return seqResult;
	}

	return vValue;
}

//
//
//
Variant add(Variant vValue1, Variant vValue2)
{
	TRACE_BEGIN;
	if (vValue1.isNull())
		return vValue2;
	if (vValue2.isNull())
		return vValue1;

	if (vValue1.isDictionaryLike())
		return vValue1.clone().merge(convert<ValueType::Dictionary>(vValue2), MergeMode::All);
	if (vValue1.isSequenceLike())
		return vValue1.clone().merge(convert<ValueType::Sequence>(vValue2), MergeMode::All);
	
	switch (vValue1.getType())
	{
		case ValueType::Boolean: return (vValue1 || convert<ValueType::Boolean>(vValue2));
		case ValueType::Integer:
		{
			Variant::IntValue nValue1 = vValue1;
			Variant::IntValue nValue2 = convert<ValueType::Integer>(vValue2);
			// TODO: Implement an overflow/undeflow detection
			if (nValue1.isSigned())
			{
				if (nValue2.isSigned())
					return nValue1.asSigned() + nValue2.asSigned();
				return nValue2.asUnsigned() + nValue1.asSigned();
			}
			if (nValue2.isSigned())
				return nValue1.asUnsigned() + nValue2.asSigned();
			return nValue1.asUnsigned() + nValue2.asUnsigned();
		}
		case ValueType::String:
		{
			Variant::StringValuePtr pStrValue1 = vValue1;
			Variant::StringValuePtr pStrValue2 = convert<ValueType::String>(vValue2);
			if (pStrValue1->hasWChar())
				return std::wstring(*pStrValue1) + std::wstring(*pStrValue2);
			return std::string(*pStrValue1) + std::string(*pStrValue2);
		}
	}
	return nullptr;
	TRACE_END("Error during operation execution");
}

//
//
//
Variant extract(Variant vData, std::string_view sPath)
{
	return getByPath(vData, sPath);
}

//
//
//
void applyTransformRule(Dictionary& vResult, const Variant& vRule, const Variant& vData)
{
	if (!vRule.has("item"))
		error::InvalidArgument(SL, "Missing field <item> in a rule").throwException();
	std::string sItem = vRule["item"];

	if (vRule.has("value"))
	{
		if (vRule["value"].isNull())
			variant::deleteByPath(vResult, sItem);
		else
			variant::putByPath(vResult, sItem, vRule["value"]);
		return;
	}

	auto optDefault = vRule.getSafe("default");

	if (vRule.has("localPath"))
	{
		auto optValue = variant::getByPathSafe(vData, vRule["localPath"]);
		if (optValue.has_value() && !optValue.value().isNull())
			variant::putByPath(vResult, sItem, optValue.value(), true);
		else if (optDefault.has_value())
			variant::putByPath(vResult, sItem, optDefault.value(), true);
		return;
	}

	if (vRule.has("catalogPath"))
	{
		auto optValue = getCatalogDataSafe(vRule["catalogPath"]);
		if (optValue.has_value() && !optValue.value().isNull())
			variant::putByPath(vResult, sItem, optValue.value(), true);
		else if (optDefault.has_value())
			variant::putByPath(vResult, sItem, optDefault.value(), true);
		return;
	}

	if (vRule.has("command"))
	{
		Variant vValue = execCommand(vRule["command"]);
		variant::putByPath(vResult, sItem, vValue);
		return;
	}

	error::InvalidArgument(SL, 
		"Missing value-related field (<value>, <localPath>, <catalogPath>, <command>)").throwException();
}

//
//
//
Variant transform(Variant vData, Variant vSchema, TransformSourceMode eMode)
{
	if (!vData.isDictionaryLike() && !vData.isSequenceLike())
		error::InvalidArgument(SL, "Value must be a dictionary or a sequence").throwException();

	Sequence seqData;
	if (vData.isSequenceLike())
		seqData = Sequence(vData);
	else
		seqData.push_back(vData);

	Sequence seqRes;
	for (auto& vSrcData : seqData)
	{
		Dictionary vResData;
		if (eMode == TransformSourceMode::ChangeSource)
			vResData = vSrcData;
		else if (eMode == TransformSourceMode::CloneSource)
			vResData = vSrcData.clone();

		if (vSchema.isDictionaryLike())
			applyTransformRule(vResData, vSchema, vSrcData);
		else if (vSchema.isSequenceLike())
			for (const auto& vRule : vSchema)
				applyTransformRule(vResData, vRule, vSrcData);
		else
			error::InvalidArgument(SL, "Schema must be a dictionary or sequence").throwException();
	
		seqRes.push_back(vResData);
	}

	//return vData.clone().merge(vResData, MergeMode::All);
	return vData.isSequenceLike() ? seqRes : seqRes[0];
}

} // namespace operation
} // namespace variant
} // namespace openEdr
// @}
