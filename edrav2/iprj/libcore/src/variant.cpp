//
// edrav2.libcore
//
// Utility functions for Variant 
// which should not be in the "inc" directory
// 
// Author: Yury Podpruzhnikov (21.12.2018)
// Reviewer: Denis Bogdanov (22.02.2019)
//
#include "pch.h"
#include <command.hpp>

namespace openEdr {

// TODO: withError
// * work like std::optional, but contains error code
template<typename T>
using WithError = std::pair<error::ErrorCode, T>;
	
namespace string {
namespace detail {

// FIXME: put the string operations to strings.hpp/cpp

template<typename IntType, typename CharT>
WithError<IntType> convertToIntSafe(const std::basic_string<CharT>& sStr, int nBase)
{
	static_assert(std::is_integral_v<IntType>, "Only integer types are supported");

	using error::ErrorCode;
	try
	{
		if constexpr (std::is_signed_v<IntType>)
		{
			if constexpr (sizeof(IntType) <= sizeof(unsigned long))
				return { ErrorCode::OK, static_cast<IntType>(std::stol(sStr, nullptr, nBase)) };
			return { ErrorCode::OK, static_cast<IntType>(std::stoll(sStr, nullptr, nBase)) };
		}
		else
		{
			if constexpr (sizeof(IntType) <= sizeof(unsigned long))
				return { ErrorCode::OK, static_cast<IntType>(std::stoul(sStr, nullptr, nBase)) };
			return { ErrorCode::OK, static_cast<IntType>(std::stoull(sStr, nullptr, nBase)) };
		}
	}
	catch (std::invalid_argument&)
	{
		return { ErrorCode::InvalidArgument, 0 };
	}
	catch (std::out_of_range&)
	{
		return { ErrorCode::OutOfRange, 0 };
	}
}

} // namespace detail


// TODO: convertToInt
//   * move to string.hpp
//   * Add convertToInt
//   * Add reimplement via std::strtoul. See std::stoul implementation (not convert exception)
//   * add unittest

//
// Convert std::string or std::wstring into selected integer type.
// Exceptions:
// * error::InvalidArgument if no conversion could be performed
// * error::OutOfRange if the converted value would fall out of the range 
//   of the result type or if the underlying function(std::strto*) sets 
//   errno to ERANGE.
//
// Usage:
// auto nVal = convertToInt<Size>(std::wstring(L"123"));
//
// NOTE: std::(w)string_view is not used because std::stoll() function
// (and others) receive std::(w)string parameter, and we cannot avoid
// a conversion anyway. The most effective way here is to use a const
// links.
//
template<typename IntType>
WithError<IntType> convertToIntSafe(const std::string& sStr, int nBase=0)
{
	return detail::convertToIntSafe<IntType>(sStr, nBase);
}

//
//
//
template<typename IntType>
WithError<IntType> convertToIntSafe(const std::wstring& sStr, int nBase = 0)
{
	return detail::convertToIntSafe<IntType>(sStr, nBase);
}

} // namespace string

	
namespace variant {

//
//
//
Variant createCmdProxy(Variant vCmd, bool fCached)
{
	auto fnCalculator = [vCmd]() {return execCommand(vCmd); };
	if (fCached)
		return detail::CachedDataProxy::create(fnCalculator);
	else
		return detail::NonCachedDataProxy::create(fnCalculator);
}

//
//
//
Variant createObjProxy(Variant vObj, bool fCached)
{
	auto fnCalculator = [vObj]() {return createObject(vObj); };
	if (fCached)
		return detail::CachedDataProxy::create(fnCalculator);
	else
		return detail::NonCachedDataProxy::create(fnCalculator);
}

//
//
//
Variant createLambdaProxy(DataProxyCalculator fnCalculator, bool fCached)
{
	if (fCached)
		return detail::CachedDataProxy::create(fnCalculator);
	else
		return detail::NonCachedDataProxy::create(fnCalculator);
}

namespace detail {

//
//
//
enum class AbsentPathAction
{
	Off, //< Not check an absence (container applies its action, e.g. throw exception)
	CreatePathWithDictionaryEnd,
	CreatePathWithSequenceEnd,
	ReturnNullopt
};

//
//
//

std::optional<Variant> getByPath(Variant vRoot, const std::string_view sPath, AbsentPathAction createPaths)
{
	if (sPath.empty())
		return vRoot;

	// Sequence index
	if (sPath.front() == '[')
	{
		Size nIndex;
		std::string_view sTail;
		
		auto nEndOfIndex = sPath.find_first_of(']');
		if (nEndOfIndex == std::string_view::npos)
			error::InvalidArgument(SL, FMT("Invalid Variant path format <" << sPath << ">")).throwException();
			
		// next char should be '.' or '[' or end of string
		Size nSkippedCharCount = 1;
		if (nEndOfIndex + 1 < sPath.size())
		{
			char nextChar = sPath[nEndOfIndex + 1];
			if (nextChar != '.' && nextChar != '[')
				error::InvalidArgument(SL, FMT("Invalid Variant path format <" << sPath << ">")).throwException();
			if (nextChar == '.')
				++nSkippedCharCount;
		}

		auto IndexWithError = string::convertToIntSafe<Size>(std::string(sPath.substr(1, nEndOfIndex - 1)));
		if(IndexWithError.first != error::ErrorCode::OK)
			error::InvalidArgument(SL, FMT("Invalid Variant path format <" << sPath << ">")).throwException();
		nIndex = IndexWithError.second;

		sTail = sPath.substr(nEndOfIndex + nSkippedCharCount);

		// get next Item + process absent items
		Variant vNextLevel;
		do 
		{
			if (createPaths == AbsentPathAction::Off)
			{
				vNextLevel = vRoot.get(nIndex);
				break;
			}

			auto voptRes = vRoot.getSafe(nIndex);
			if (voptRes.has_value())
			{
				vNextLevel = *voptRes;
				break;
			}

			// Process an item absence

			if (createPaths == AbsentPathAction::ReturnNullopt)
				return std::nullopt;

			// Sequence creation is not supported
			error::OutOfRange(SL, FMT("Invalid sequence index <" << nIndex << ">")).throwException();
		} while (false);

		return getByPath(vNextLevel, sTail, createPaths);
	}

	// Dictionary index
	std::string_view sKey;
	std::string_view sTail;
	bool fCreateDict = false;
		
	auto nEndOfKey = sPath.find_first_of("[.");
	if (nEndOfKey == std::string_view::npos)
		sKey = sPath;
	else
	{
		sKey = sPath.substr(0, nEndOfKey);
		if (sPath[nEndOfKey] == '.')
		{
			sTail = sPath.substr(nEndOfKey + 1);
			fCreateDict = true;
		}
		else
			sTail = sPath.substr(nEndOfKey);
	}

	if (sKey.empty())
		error::InvalidArgument(SL, FMT("Invalid Variant path format <" << sPath << ">")).throwException();

	// get next Item + process absent items
	Variant vNextLevel;
	do
	{
		if (createPaths == AbsentPathAction::Off)
		{
			vNextLevel = vRoot.get(sKey);
			break;
		}

		auto voptRes = vRoot.getSafe(sKey);
		if (voptRes.has_value())
		{
			vNextLevel = *voptRes;
			break;
		}

		// Process an item absence

		if (createPaths == AbsentPathAction::ReturnNullopt)
			return std::nullopt;

		// Add finish item 
		if (sTail.empty())
		{
			if (createPaths == AbsentPathAction::CreatePathWithDictionaryEnd)
				vRoot.put(sKey, Dictionary());
			if (createPaths == AbsentPathAction::CreatePathWithSequenceEnd)
				vRoot.put(sKey, Sequence());
			return vRoot.get(sKey);
		}

		// Add intermediate dictionary
		if (fCreateDict)
		{
			vRoot.put(sKey, Dictionary());
			vNextLevel = vRoot.get(sKey);
			break;
		}
		
		// Add intermediate sequence (not supported)
		error::InvalidArgument(SL, FMT("No sequence found at <" << sPath << ">")).throwException();
	} while (false);

	return getByPath(vNextLevel, sTail, createPaths);
}

//
//
//
void putByPath(Variant vRoot, const std::string_view& sPath, const std::optional<Variant>& optValue, bool fCreatePaths)
{
	if (sPath.empty())
		error::InvalidArgument(SL, FMT("Invalid Variant path format <" << sPath << ">")).throwException();

	// Last is Sequence index
	if (sPath.back() == ']')
	{
		Size nIndex;
		std::string_view sHead;
		auto nStartOfIndex = sPath.find_last_of('[');
		if (nStartOfIndex == std::string_view::npos)
			error::InvalidArgument(SL, FMT("Invalid Variant path format <" << sPath << ">")).throwException();

		auto IndexWithError = string::convertToIntSafe<Size>(
			std::string(sPath.substr(nStartOfIndex + 1, sPath.size() - nStartOfIndex - 2)));
		if (IndexWithError.first != error::ErrorCode::OK)
			error::InvalidArgument(SL, FMT("Invalid Variant path format <" << sPath << ">")).throwException();
		nIndex = IndexWithError.second;
		sHead = sPath.substr(0, nStartOfIndex);


		// Process put
		if (optValue.has_value())
		{
			auto vCont = getByPath(vRoot, sHead, 
				fCreatePaths ? AbsentPathAction::CreatePathWithSequenceEnd : AbsentPathAction::Off);
			vCont->put(nIndex, optValue.value());
		}
		// Process del
		else
		{
			// Use ReturnNullopt to suppress path-does-not-exist exceptions
			auto vCont = getByPath(vRoot, sHead, AbsentPathAction::ReturnNullopt);
			if (vCont.has_value())
				vCont->erase(nIndex);
		}

		return;
	}

	// Last is Dictionary key
	std::string_view sKey;
	std::string_view sHead;
		
	auto nStartOfKey = sPath.find_last_of(".[]");
	if (nStartOfKey == std::string_view::npos)
		sKey = sPath;
	else if (sPath[nStartOfKey] != '.')
		error::InvalidArgument(SL, FMT("Invalid Variant path format <" << sPath << ">")).throwException();
	else
	{
		sKey = sPath.substr(nStartOfKey + 1);
		sHead = sPath.substr(0, nStartOfKey);
	}

	if (sKey.empty())
		error::InvalidArgument(SL, FMT("Invalid Variant path format <" << sPath << ">")).throwException();

	// Process put
	if (optValue.has_value())
	{
		auto vCont = getByPath(vRoot, sHead,
			fCreatePaths ? AbsentPathAction::CreatePathWithDictionaryEnd : AbsentPathAction::Off);
		vCont->put(sKey, optValue.value());
	}
	// Process del
	else
	{
		// Use ReturnNullopt to suppress path-does-not-exist exceptions
		auto vCont = getByPath(vRoot, sHead, AbsentPathAction::ReturnNullopt);
		if (vCont.has_value())
			vCont->erase(sKey);
	}
}

} // namespace detail

//
//
//
Variant getByPath(Variant vRoot, const std::string_view sPath)
{
	return detail::getByPath(vRoot, sPath, detail::AbsentPathAction::Off).value();
}

//
//
//
Variant getByPath(Variant vRoot, const std::string_view sPath, Variant vDefault)
{
	return detail::getByPath(vRoot, sPath, detail::AbsentPathAction::ReturnNullopt).value_or(vDefault);
}

//
//
//
std::optional<Variant> getByPathSafe(Variant vRoot, const std::string_view& sPath)
{
	return detail::getByPath(vRoot, sPath, detail::AbsentPathAction::ReturnNullopt);
}


//
//
//
void putByPath(Variant vRoot, const std::string_view& sPath, Variant vValue)
{
	detail::putByPath(vRoot, sPath, vValue, /* fCreatePaths */ false);
}

//
//
//
void putByPath(Variant vRoot, const std::string_view& sPath, Variant vValue, bool fCreatePaths)
{
	detail::putByPath(vRoot, sPath, vValue, fCreatePaths);
}

//
//
//
void deleteByPath(Variant vRoot, const std::string_view& sPath)
{
	detail::putByPath(vRoot, sPath, std::nullopt, /* fCreatePaths */ false);
}

//
//
//
template<>
Variant convert<ValueType::Boolean>(Variant vValue)
{
	if (vValue.isDictionaryLike() || vValue.isSequenceLike())
		return !vValue.isEmpty();

	switch (vValue.getType())
	{
		case ValueType::Null:
			return false;
		case ValueType::Integer:
			return (vValue != 0);
		case ValueType::String:
		{
			Variant::StringValuePtr pStrValue = vValue;
			if (pStrValue->hasWChar())
				return !std::wstring_view(*pStrValue).empty();
			return !std::string_view(*pStrValue).empty();
		}
		case ValueType::Boolean:
			return vValue;
		case ValueType::Object:
		{
			ObjPtr<IObject> pObj = vValue;
			return (pObj != nullptr);
		}
	}
	error::TypeError(SL, "Unsupported type for boolean conversion").throwException();
}

//
// FIXME: I'm not fure that these functions shall return Variant
//
template<>
Variant convert<ValueType::Integer>(Variant vValue)
{
	if (vValue.isDictionaryLike() || vValue.isSequenceLike())
		return vValue.getSize();

	switch (vValue.getType())
	{
		case ValueType::Null:
			return 0;
		case ValueType::Integer:
			return vValue;
		case ValueType::String:
		{
			Variant::StringValuePtr pStrValue = vValue;
			if (pStrValue->hasWChar())
			{
				auto res = string::convertToIntSafe<int64_t>(std::wstring(*pStrValue), 0);
				if (res.first == ErrorCode::OK)
					return res.second;
				res = string::convertToIntSafe<uint64_t>(std::wstring(*pStrValue), 0);
				if (res.first == ErrorCode::OK)
					return res.second;
			}
			else
			{
				// FIXME: Use string_view
				auto res = string::convertToIntSafe<int64_t>(std::string(*pStrValue), 0);
				if (res.first == ErrorCode::OK)
					return res.second;
				res = string::convertToIntSafe<int64_t>(std::string(*pStrValue), 0);
				if (res.first == ErrorCode::OK)
					return res.second;
			}
			return 0;
		}
		case ValueType::Boolean:
			return (vValue ? 1 : 0);
		case ValueType::Object:
		{
			ObjPtr<IObject> pObj = vValue;
			return ((pObj != nullptr) ? 1 : 0);
		}
	}
	
	error::TypeError(SL, "Unsupported type for integer conversion").throwException();
}
	
//
//
//
template<>
Variant convert<ValueType::String>(Variant vValue)
{
	if (vValue.isDictionaryLike())
		return "{}";
	if (vValue.isSequenceLike())
		return "[]";

	switch (vValue.getType())
	{
		case ValueType::Null:
			return "";
		case ValueType::Integer:
		{
			Variant::IntValue nIntValue = vValue;
			if (nIntValue.isSigned())
				return std::to_string(nIntValue.asSigned());
			return std::to_string(nIntValue.asUnsigned());
		}
		case ValueType::String:
			return vValue;
		case ValueType::Boolean:
			return (vValue ? "true" : "false");
		case ValueType::Object:
		{
			ObjPtr<IObject> pObj = vValue;
			if (pObj == nullptr)
				return "";
			return string::convertToHex(pObj->getClassId());
		}
	}

	error::TypeError(SL, "Unsupported type for integer conversion").throwException();
}

//
//
//
template<>
Variant convert<ValueType::Dictionary>(Variant vValue)
{
	if (vValue.isDictionaryLike())
		return vValue;
	if (vValue.isNull())
		return Dictionary();

	return Dictionary({ { "data", vValue } });
}

//
//
//
template<>
Variant convert<ValueType::Sequence>(Variant vValue)
{
	if (vValue.isSequenceLike())
		return vValue;
	if (vValue.isNull())
		return Sequence();

	return Sequence({ vValue });
}


namespace detail {

//
//
//
Variant mergeSequeceLike(Variant vDst, Variant vSrc, MergeMode eMode)
{
	if (!vSrc.isSequenceLike())
	{
		if (testFlag(eMode, MergeMode::CheckType) && vDst.getType() != vSrc.getType()) /*both are object*/
			error::InvalidArgument(SL, FMT("Merge parameters have different types (dst: <" << 
				vDst.getType() << ">, src: <" << vSrc.getType() << ">)" )).throwException();
		return vSrc;
	}

	for (auto vElem : vSrc)
		vDst.push_back(vElem);
	return vDst;
}

//
//
//
Variant mergeDictionaryLike(Variant vDst, Variant vSrc, MergeMode eMode)
{
	if (!vSrc.isDictionaryLike())
	{
		if (testFlag(eMode, MergeMode::CheckType) && vDst.getType() != vSrc.getType()) /*both are object*/
			error::InvalidArgument(SL, FMT("Merge parameters have different types (dst: <" <<
				vDst.getType() << ">, src: <" << vSrc.getType() << ">)")).throwException();
		return vSrc;
	}
	if (!testMaskAny(eMode, MergeMode::New | MergeMode::Exist))
		error::InvalidArgument(SL, "MergeMode::New or/and MergeMode::Exist are expected").throwException();

	for (auto kv : Dictionary(vSrc))
	{
		auto& sName = kv.first;
		auto& vSrcVal = kv.second;

		// remove elements
		if (!testFlag(eMode, MergeMode::NonErasingNulls) && vSrcVal.isRawNull())
		{
			vDst.erase(sName);
			continue;
		}

		// Process new elements
		if (!vDst.has(sName))
		{
			if(testFlag(eMode, MergeMode::New))
				vDst.put(sName, vSrcVal);
			continue;
		}

		// Process existent elements
		if (!testFlag(eMode, MergeMode::Exist))
			continue;
		
		// if need just replace
		if (!testMaskAny(eMode, MergeMode::CheckType | MergeMode::MergeNestedDict | MergeMode::MergeNestedDict))
		{
			vDst.put(sName, vSrcVal);
			continue;
		}

		Variant vDstVal = vDst.get(sName);

		// merge nested dictionary
		if (vDstVal.isDictionaryLike())
		{
			if (!vSrcVal.isDictionaryLike())
			{
				if (testFlag(eMode, MergeMode::CheckType))
					error::InvalidArgument(SL, FMT("Merge parameters have different types (dst: <" <<
						vDst.getType() << ">, src: <" << vSrc.getType() << ">)")).throwException();
				vDst.put(sName, vSrcVal);
				continue;
			}

			if (!testFlag(eMode, MergeMode::MergeNestedDict))
			{
				vDst.put(sName, vSrcVal);
				continue;
			}

			(void)mergeDictionaryLike(vDstVal, vSrcVal, eMode);
			continue;
		}

		// merge nested dictionary
		if (vDstVal.isSequenceLike())
		{
			if (!vSrcVal.isSequenceLike())
			{
				if (testFlag(eMode, MergeMode::CheckType))
					error::InvalidArgument(SL, FMT("Merge parameters have different types (dst: <" <<
						vDst.getType() << ">, src: <" << vSrc.getType() << ">)")).throwException();
				vDst.put(sName, vSrcVal);
				continue;
			}

			if (!testFlag(eMode, MergeMode::MergeNestedSeq))
			{
				vDst.put(sName, vSrcVal);
				continue;
			}

			(void)mergeSequeceLike(vDstVal, vSrcVal, eMode);
			continue;
		}

		// merge others
		{
			if (testFlag(eMode, MergeMode::CheckType) && vDstVal.getType() != vSrcVal.getType())
				error::InvalidArgument(SL, FMT("Merge parameters have different types (dst: <" <<
					vDst.getType() << ">, src: <" << vSrc.getType() << ">)")).throwException();
			vDst.put(sName, vSrcVal);
			continue;
		}
	}
	return vDst;
}

//
//
//
Variant merge(Variant vDst, const Variant vSrc, MergeMode eMode)
{
	const auto eSrcType = vSrc.getType();

	// null merge
	if (eSrcType == ValueType::Null)
		return Variant();

	const auto eDstType = vDst.getType();

	switch (eDstType)
	{
		case ValueType::Dictionary:
		{
			return mergeDictionaryLike(vDst, vSrc, eMode);
		}
		case ValueType::Sequence:
		{
			return mergeSequeceLike(vDst, vSrc, eMode);
		}
		case ValueType::Object:
		{
			// Smart checking for objects which has IDictionaryProxy and ISequenceProxy both.
			bool fIsDstDict = vDst.isDictionaryLike();
			bool fIsDstSeq = vDst.isSequenceLike();

			if (!fIsDstDict && !fIsDstSeq)
			{
				if (testFlag(eMode, MergeMode::CheckType) && eSrcType != ValueType::Object)
					error::InvalidArgument(SL, FMT("Merge parameters have different types: dst: <" <<
						eDstType << ">, src: <" << eSrcType << ">")).throwException();
				return vSrc;
			}
			else if (fIsDstDict && !fIsDstSeq)
			{
				return mergeDictionaryLike(vDst, vSrc, eMode);
			}
			else if (!fIsDstDict && fIsDstSeq)
			{
				return mergeSequeceLike(vDst, vSrc, eMode);
			}
			else  // (fIsDstDict && fIsDstSeq)
			{
				if (vSrc.isDictionaryLike())
					return mergeDictionaryLike(vDst, vSrc, eMode);
				else
					return mergeSequeceLike(vDst, vSrc, eMode);
			}
		}
		default:
		{
			if (testFlag(eMode, MergeMode::CheckType) && eSrcType != eDstType)
				error::InvalidArgument(SL, FMT("Merge parameters have different types: dst: <" <<
					eDstType << ">, src: <" << eSrcType << ">")).throwException();
			return vSrc;
		}
	}
}

//
//
//
std::string print(const Variant& v, Size nIndent, bool fQuoteString)
{
	using RawValueType = detail::RawValueType;

	switch (getRawType(v))
	{
		case RawValueType::Null:
		{
			return "null";
		}
		case RawValueType::Boolean:
		{
			if ((bool)v)
				return "true";
			else
				return "false";
		}
		case RawValueType::Integer:
		{
			Variant::IntValue val = v;
			if (val.isSigned())
				return std::to_string(val.asSigned());
			else
				return std::to_string(val.asUnsigned());
		}
		case RawValueType::String:
		{
			if (!fQuoteString)
				return std::string(v);

			std::ostringstream osResult;
			osResult << std::quoted(std::string_view(v));
			return osResult.str();
		}
		case RawValueType::Object:
		{
			ObjPtr<IObject> pObj = v;

			// Printing IPrintable
			auto pPrintable = queryInterfaceSafe<IPrintable>(pObj);
			if (pPrintable)
				return pPrintable->print();

			// Printing not IPrintable
			bool fSupportDictionaryProxy = (bool)queryInterfaceSafe<IDictionaryProxy>(v);
			bool fSupportSequenceProxy = (bool)queryInterfaceSafe<ISequenceProxy>(v);

			std::ostringstream osResult;
			osResult << pObj;
			if (fSupportDictionaryProxy && fSupportSequenceProxy)
				osResult << "(IDictionaryProxy, ISequenceProxy)";
			else if (fSupportDictionaryProxy)
				osResult << "(IDictionaryProxy)";
			else if (fSupportSequenceProxy)
				osResult << "(ISequenceProxy)";
			return osResult.str();
		}
		case RawValueType::Dictionary:
		{
			if (v.isEmpty())
				return "{}";
			//boost::io::ios_flags_saver  ifs(os);

			std::string sIndent(nIndent * 4, ' ');

			std::ostringstream osResult;
			osResult << "{\n";
			for (auto kv : Dictionary(v))
				osResult << sIndent << std::quoted(kv.first) << ": " << print(kv.second, nIndent + 1, true) << ",\n";
			osResult << std::string ((nIndent-1) * 4, ' ') << "}";
			return osResult.str();
		}
		case RawValueType::Sequence:
		{
			if (v.isEmpty())
				return "[]";

			std::string sIndent(nIndent * 4, ' ');
			std::ostringstream osResult;
			osResult << "[\n";
			for (auto elem : v)
				osResult << sIndent << print(elem, nIndent + 1, true) << ",\n";
			osResult << std::string((nIndent - 1) * 4, ' ') << "]";
			return osResult.str();
		}
		case RawValueType::DataProxy:
		{
			return (**convertVariantSafe<std::shared_ptr<IDataProxy>>(v)).print();
		}
		default:
		return "<unknown type>";
	}
}


//
// DictKeyStorage declaration
// We can't use inline variable because MSVC bug, which fixed in MSVS 2019 only.
//
DictKeyStorage g_DictKeyStorage;

} // namespace detail 
} // namespace variant
} // namespace openEdr 
