//
// EDRAv2.libcore project
//
// Variant serialization 
// The nlohmann JSON library is used (https://nlohmann.github.io/json/)
//
// Author: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (15.01.2019)
//
//
#pragma once
#include "pch.h"
#include <io.hpp>
#include <objects.h>

namespace cmd {
namespace variant {


// FIXME: We shall call all funcions with prifix Json if they use Json-specific code, or
// we may rename the file to serialize_json. 

//
//
//
Variant serializeObject(Variant vSrc)
{
	auto pSerializable = queryInterfaceSafe<ISerializable>(vSrc);
	if (pSerializable)
		return pSerializable->serialize();

	auto pStream = queryInterfaceSafe<io::IReadableStream>(vSrc);
	if (pStream)
		return serializeStream(pStream);

	error::TypeError(SL, "Can't serialize object").throwException();
}

//
//
//
Variant deserializeObject(Variant vSrc, bool fRecursively)
{
	if (fRecursively)
	{
		if (vSrc.getType() == variant::ValueType::Sequence)
		{
			for (size_t i = 0; i < vSrc.getSize(); ++i)
			{
				if (vSrc[i].getType() == variant::ValueType::Sequence)
					(void)deserializeObject(vSrc[i], true);
				else if (vSrc[i].getType() == variant::ValueType::Dictionary)
					vSrc.put(i, deserializeObject(vSrc[i], true));
			}
		}
		else if (vSrc.getType() == variant::ValueType::Dictionary)
		{
			for (auto kv : Dictionary(vSrc))
			{
				if (kv.second.getType() == variant::ValueType::Sequence)
					(void)deserializeObject(kv.second, true);
				else if (kv.second.getType() == variant::ValueType::Dictionary)
					vSrc.put(kv.first, deserializeObject(kv.second, true));
			}
		}
	}

	if (vSrc.getType() != Variant::ValueType::Dictionary)
		return vSrc;

	// Process DataProxy creation
	auto vProxyType = vSrc.getSafe("$$proxy");
	if (vProxyType.has_value())
	{
		if (vProxyType->getType() == ValueType::String)
		{
			std::string_view sProxyType = vProxyType.value();
			if (sProxyType == "obj")
				return createObjProxy(vSrc, false);
			if (sProxyType == "cachedObj")
				return createObjProxy(vSrc, true);
			if (sProxyType == "cmd")
				return createCmdProxy(vSrc, false);
			if (sProxyType == "cachedCmd")
				return createCmdProxy(vSrc, true);
		}
		error::InvalidUsage(SL, FMT("Invalid value of <$$proxy> field: <" << vProxyType.value() << ">."))
			.throwException();
	}

	// Process object creation
	auto vClassId = vSrc.getSafe("$$clsid");
	if (vClassId.has_value())
	{
		switch (vClassId->getType())
		{
			case Variant::ValueType::Integer:
			{
				ClassId nClassId = vClassId.value();
				return createObject(nClassId, vSrc);
			}
			case Variant::ValueType::String:
			{
				std::string sClassId = vClassId.value();
				ClassId nClassId = ClassId(std::strtoul(sClassId.c_str(), nullptr, 0));
				return createObject(nClassId, vSrc);
			}
			default:
				error::InvalidUsage(SL, FMT("Invalid value of <$$clsid> field: <" << vClassId.value() << ">."))
					.throwException();
		}
	}

	return vSrc;
}

//
// Json parser with nlohmann "Modern c++" library
//
namespace json_nlohmann {

using nljson = nlohmann::json;

//
// serializeToJson
//
nljson saveVariantToJson(const cmd::Variant& vElement)
{
	using ValueType = Variant::ValueType;
	switch (vElement.getType())
	{
		case ValueType::Null:
		{
			return nljson(nljson::value_t::null);
		}
		case ValueType::Boolean:
		{
			return nljson(static_cast<bool> (vElement));
		}
		case ValueType::Integer:
		{
			Variant::IntValue nIntKeeper = vElement;
			if (nIntKeeper.isSigned())
				return nljson(nIntKeeper.asSigned());
			return nljson(nIntKeeper.asUnsigned());
		}
		case ValueType::String:
		{
			auto sVal = static_cast<std::string_view>(vElement);
			return nljson(sVal);
		}
		case ValueType::Dictionary:
		{
			nljson JsonObject = nljson::object();
			for (auto Item : Dictionary(vElement))
			{
				std::string sKey (static_cast<Dictionary::Index>(Item.first));
				JsonObject[sKey] = saveVariantToJson(Item.second);
			}
			return JsonObject;
		}
		case ValueType::Sequence:
		{
			nljson JsonArray = nljson::array();
			for (auto& vVal: vElement)
				JsonArray.push_back(saveVariantToJson(vVal));
			return JsonArray;
		}
		case ValueType::Object:
		{
			return saveVariantToJson(serializeObject(vElement));
		}
	}
	error::TypeError(SL, "Unsupported element type in Variant").throwException();
}

//
//
//
std::string saveToJson(const cmd::Variant& vValue)
{
	try
	{
		auto jsonObject = saveVariantToJson(vValue);
		return jsonObject.dump();
	}
	catch (nljson::exception& e)
	{
		error::RuntimeError(SL, e.what()).throwException();
	}
}

//
// deserializeFromJson
//
cmd::Variant saveJsonToVariant(nljson& element)
{
	switch (element.type())
	{
		case nljson::value_t::null:
		{
			return cmd::Variant(nullptr);
		}
		case nljson::value_t::boolean:
		{
			return cmd::Variant((bool)element);
		}
		case nljson::value_t::number_integer:
		{
			return cmd::Variant((int64_t)element);
		}
		case nljson::value_t::number_unsigned:
		{
			return cmd::Variant((uint64_t)element);
		}
		case nljson::value_t::string:
		{
			std::string sUtf8Val = element;
			return cmd::Variant(sUtf8Val);
		}
		case nljson::value_t::array:
		{
			Variant seq = Sequence();
			for (auto& SubElement : element)
				seq.push_back(saveJsonToVariant(SubElement));
			return seq;
		}
		case nljson::value_t::object:
		{
			Variant dict = Dictionary();
			// special iterator member functions for objects
			for (nljson::iterator it = element.begin(); it != element.end(); ++it)
				dict.put(it.key(), saveJsonToVariant(it.value()));
			return deserializeObject(dict);
		}
	}
	error::InvalidFormat(SL, "Unsupported type of JSON element").throwException();
}

//
//
//
Variant loadfromJson(const char* sStr, size_t nSize)
{
	try
	{
		auto jsonObject = nljson::parse( sStr, sStr + nSize);
		return saveJsonToVariant(jsonObject);
	}
	catch (nljson::exception& e)
	{
		error::InvalidFormat(SL, FMT("Can't deserialize Variant: " << e.what())).throwException();
	}
}

} // namespace json_nlohmann

//
// Json parser with the jsoncpp library
//
namespace json_jsoncpp {

//
// deserializeFromJson
//
cmd::Variant saveJsonToVariant(const Json::Value& value)
{
	switch (value.type())
	{
		case Json::ValueType::nullValue:
			return {};
		case Json::ValueType::intValue:
			return Variant(value.asLargestInt());
		case Json::ValueType::uintValue:
			return Variant(value.asLargestUInt());
		case Json::ValueType::realValue:
			error::InvalidFormat(SL, "Variant doesn't support double type").throwException();
		case Json::ValueType::stringValue:
			return Variant(value.asString());
		case Json::ValueType::booleanValue:
			return Variant(value.asBool());
		case Json::ValueType::arrayValue:
		{
			Variant seq = Sequence();
			for (auto& elem : value)
				seq.push_back(saveJsonToVariant(elem));
			return seq;
		}
		case Json::ValueType::objectValue:
		{
			Variant dict = Dictionary();
			Json::Value::Members members(value.getMemberNames());
			for (auto iter = members.begin(); iter != members.end(); ++iter)
			{
				const std::string& sKey = *iter;
				try
				{
					dict.put(sKey, saveJsonToVariant(value[sKey]));
				}
				catch (error::InvalidFormat&)
				{
					dict.put(sKey, {});
				}
			}
			return deserializeObject(dict);
		}
	}
	error::TypeError(SL, "Unsupported JSON value type").throwException();
}

//
// serializeToJson
//
Json::Value saveVariantToJson(const cmd::Variant& vValue)
{
	using ValueType = Variant::ValueType;
	switch (vValue.getType())
	{
		case Variant::ValueType::Null:
			return {};
		case Variant::ValueType::Boolean:
			return Json::Value(static_cast<bool>(vValue));
		case Variant::ValueType::Integer:
		{
			Variant::Variant::IntValue nIntKeeper = vValue;
			if (nIntKeeper.isSigned())
				return Json::Value(nIntKeeper.asSigned());
			return Json::Value(nIntKeeper.asUnsigned());
		}
		case Variant::ValueType::String:
			return Json::Value(std::string(vValue));
		case Variant::ValueType::Dictionary:
		{
			Json::Value objValue(Json::objectValue);
			for (auto [sKey, vItem] : Dictionary(vValue))
			{
				objValue[std::string(sKey)] = saveVariantToJson(vItem);
			}
			return objValue;
		}
		case Variant::ValueType::Sequence:
		{
			Json::Value arrValue(Json::arrayValue);
			for (auto& vItem : vValue)
				arrValue.append(saveVariantToJson(vItem));
			return arrValue;
		}
		case ValueType::Object:
		{
			return saveVariantToJson(serializeObject(vValue));
		}
	}
	error::TypeError(SL, "Unsupported element type in Variant").throwException();
}

//
//
//
std::string saveToJson(const cmd::Variant& vValue, JsonFormat fmt)
{
	try
	{
		// Variant -> json
		auto jsonObject = saveVariantToJson(vValue);

		// json -> string
		Json::StreamWriterBuilder wbuilder;
		if (fmt == JsonFormat::SingleLine)
		{
			wbuilder["commentStyle"] = "None";
			wbuilder["indentation"] = "";
		}
		return Json::writeString(wbuilder, jsonObject);
	}
	catch (Json::Exception& e)
	{
		error::RuntimeError(SL, e.what()).throwException();
	}
}


//
//
//
Variant loadfromJson(const char* sStr, size_t nSize)
{
	// string -> json
	Json::Value jsonObject;
	{
		Json::CharReaderBuilder rbuilder;
		rbuilder["collectComments"] = false;
		rbuilder["failIfExtra"] = true;
		std::unique_ptr<Json::CharReader> const reader(rbuilder.newCharReader());

		std::string sError;
		if (!reader->parse(sStr, sStr + nSize, &jsonObject, &sError))
			error::InvalidFormat(SL, FMT("Can't deserialize Variant: " << sError)).throwException();
	}

	// json -> Variant
	return saveJsonToVariant(jsonObject);
}

} // namespace json_jsoncpp


namespace detail {

//
//
//
void serializeToJson(ObjPtr<io::IRawWritableStream> pStream, const Variant& vVal, const JsonFormat fmt)
{
	io::write(pStream, serializeToJson(vVal, fmt));
}

//
//
//
void serialize(ObjPtr<io::IRawWritableStream> pStream, const Variant& vVal, const Variant& vCfg)
{
	auto pOutStream = queryInterface<IDataReceiver>(
		createObject(CLSID_StreamSerializer, Dictionary({
			{"stream", pStream},
			{"format", vCfg.get("format", getCatalogData("app.config.serialization.format", "json"))},
			{"encryption", vCfg.get("encryption", getCatalogData("app.config.serialization.encryption", ""))},
			{"compression", vCfg.get("compression", getCatalogData("app.config.serialization.compression", ""))}
		})));
	pOutStream->put(vVal);
}

}

//
//
//
std::string serializeToJson(const Variant& vVal, const JsonFormat fmt)
{
	switch (fmt)
	{
	case JsonFormat::Pretty:
		return json_jsoncpp::saveToJson(vVal, fmt);
	case JsonFormat::SingleLine:
		return json_nlohmann::saveToJson(vVal);
	default:
		error::InvalidArgument(SL, FMT("Invalid parameter fmt: " << fmt)).throwException();
	}
}


//
//
//
Variant deserializeFromJson(const std::string_view& sUtf8Str)
{
	return json_jsoncpp::loadfromJson(sUtf8Str.data(), sUtf8Str.size());
}

//
//
//
Variant deserializeFromJson(const char* pData, Size nSize)
{
	return json_jsoncpp::loadfromJson(pData, nSize);
}

//
//
//
Variant deserializeFromJson(ObjPtr<io::IRawReadableStream> pStream)
{
	TRACE_BEGIN;

	// Unpack and decrypt stream
	auto pRawStream = queryInterface<io::IRawReadableStream>(
		createObject(CLSID_StreamDeserializer, Dictionary({
			{"stream", pStream},
		})));

	// Read data from file
	auto pReadableStream = io::convertStream(pRawStream);
	auto nSize = pReadableStream->getSize();
	if (nSize == 0)
		error::NoData(SL, "Source stream is empty").throwException();

	std::vector<char> sBuffer;
	sBuffer.resize((size_t)nSize);

	if (pReadableStream->read(sBuffer.data(), sBuffer.size()) != sBuffer.size())
		error::InvalidFormat(SL).throwException();

	// data -> string_view
	std::string_view sData(sBuffer.data(), sBuffer.size());

	// Check and delete BOM
	if (sData.size() > 3 && 
		sData[0] == '\xEF' && sData[1] == '\xBB' && sData[2] == '\xBF')
			sBuffer[0] = sBuffer[1] = sBuffer[2] = ' ';
	
	return deserializeFromJson(sData);
	TRACE_END("Can't deserialize JSON stream");
}

//
//
//
Variant deserialize(ObjPtr<io::IRawReadableStream> pStream)
{
	// Unpack and decrypt stream
	auto pRawStream = queryInterface<IDataProvider>(
		createObject(CLSID_StreamDeserializer, Dictionary({
			{"stream", pStream},
		})));
	auto data = pRawStream->get();
	if (data.has_value())
		return data.value();
	error::RuntimeError(SL, "Fail to deserialize stream").throwException();
}

} // namespace variant
} // namespace cmd 

