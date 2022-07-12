//
// edrav2.libcore project
//
// Author: Yury Ermakov (31.01.2019)
// Reviewer: Denis Bogdanov (06.02.2019)
//
///
/// @file JSON-RPC support tools
///
#include "pch.h"
#include <io.hpp>
#include <objects.h>
#include "jsonrpc.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "jsonrpc"

namespace cmd {
namespace ipc {

constexpr char c_sInvalid[] = "invalid";
constexpr char c_sPlain[] = "plain";
constexpr char c_sEncrypted[] = "encrypted";
constexpr char c_sBoth[] = "both";

//
//
//
ChannelMode getChannelModeFromString(std::string_view sMode)
{
	if (sMode == c_sPlain)
		return ChannelMode::Plain;
	if (sMode == c_sEncrypted)
		return ChannelMode::Encrypted;
	if (sMode == c_sBoth)
		return ChannelMode::Both;
	return ChannelMode::Invalid;
}

//
//
//
const char* getChannelModeString(ChannelMode mode)
{
	switch (mode)
	{
		case ChannelMode::Invalid: return c_sInvalid;
		case ChannelMode::Plain: return c_sPlain;
		case ChannelMode::Encrypted: return c_sEncrypted;
		case ChannelMode::Both: return c_sBoth;
	}
	error::InvalidArgument(SL, FMT("Channel mode <" <<
		+(std::uint8_t)mode << "> has no text string")).throwException();
}

//
//
//
Variant convertJsonValueToVariant(const Json::Value& value)
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
			error::TypeError(SL, "Variant doesn't support double type").throwException();
		case Json::ValueType::stringValue:
			return Variant(value.asString());
		case Json::ValueType::booleanValue:
			return Variant(value.asBool());
		case Json::ValueType::arrayValue:
		{
			Sequence seq = Sequence();
			Json::ArrayIndex nSize = value.size();
			for (Json::ArrayIndex i = 0; i < nSize; ++i)
				seq.insert(i, convertJsonValueToVariant(value[i]));
			return seq;
		}
		case Json::ValueType::objectValue:
		{
			Variant dict = Dictionary();
			Json::Value::Members members(value.getMemberNames());
			for (auto iter = members.begin(); iter != members.end(); ++iter)
			{
				const std::string& sKey = *iter;
				dict.put(sKey, convertJsonValueToVariant(value[sKey]));
			}
			return deserializeObject(dict);
		}
	}
	error::TypeError(SL, "Unsupported JSON value type").throwException();
}

//
//
//
Json::Value convertVariantToJsonValue(const Variant& vValue)
{
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
			for (auto item : Dictionary(vValue))
			{
				std::string sKey(item.first);
				objValue[sKey] = convertVariantToJsonValue(item.second);
			}
			return objValue;
		}
		case Variant::ValueType::Sequence:
		{
			Json::Value arrValue(Json::arrayValue);
			for (auto& vItem : vValue)
				arrValue.append(convertVariantToJsonValue(vItem));
			return arrValue;
		}
		// TODO: Process IDataProxy as object without auto conversion
		case Variant::ValueType::Object:
		{
			try
			{
				if (queryInterfaceSafe<variant::IDictionaryProxy>(vValue))
				{
					Json::Value objValue(Json::objectValue);
					for (auto iter : Dictionary(vValue))
						objValue[std::string(iter.first)] = convertVariantToJsonValue(iter.second);
					return objValue;
				}
				if (queryInterfaceSafe<variant::ISequenceProxy>(vValue))
				{
					Json::Value arrValue(Json::arrayValue);
					for (auto iter : vValue)
						arrValue.append(convertVariantToJsonValue(iter));
					return arrValue;
				}
				auto pStream = queryInterfaceSafe<io::IReadableStream>(vValue);
				if (pStream)
					return convertVariantToJsonValue(io::serializeStream(pStream));

				Json::Value objValue(Json::objectValue);
				ObjPtr<IObject> pObj = vValue;
				objValue["clsid"] = string::convertToHex(pObj->getClassId());
				objValue["objid"] = string::convertToHex(pObj->getId());
				objValue["warning"] = "This object wasn't serializad during JSON RPC transfering";
				return objValue;
			}
			// Workaround because of some objects return exception then iterator is requested
			catch (error::LogicError&)
			{
				Json::Value objValue(Json::nullValue);
				return objValue;
			}
			//	return convertVariantToJsonValue(serializeObject(vValue));
		}
	}
	error::TypeError(SL, FMT("Unsupported Variant value type <" << vValue.getType() << ">")).throwException();
}

//
//
//
void encryptJsonValue(const Variant& vData, Json::Value& result, const Variant& vConfig)
{
	if (vData.isNull())
		return;
	auto pWriteStream = queryInterface<io::IWritableStream>(io::createMemoryStream());
	variant::serialize(pWriteStream, vData, vConfig);

	auto pReadStream = queryInterface<io::IReadableStream>(pWriteStream);
	pReadStream->setPosition(0);

	auto pResultStream = io::createMemoryStream();
	auto pBase64Stream = queryInterface<io::IRawWritableStream>(createObject(CLSID_Base64Encoder,
		Dictionary({ {"stream", pResultStream} })));

	io::write(pBase64Stream, pReadStream);
	pBase64Stream->flush();
	const auto[pData, nDataSize] = queryInterface<io::IMemoryBuffer>(pResultStream)->getData();
	result[c_sEncryptedFieldName] = std::string((char*)pData, nDataSize);
}

//
//
//
Variant decryptJsonValue(const Json::Value& data)
{
	if (data.isNull())
		return {};
	if (!data.isObject() || !data.isMember(c_sEncryptedFieldName))
		error::InvalidFormat(SL, "Data is not encrypted").throwException();

	auto encryptedData = data[c_sEncryptedFieldName];
	if (encryptedData.empty())
		return {};

	auto sData = encryptedData.asCString();
	auto pDataStream = io::createMemoryStream(sData, strlen(sData));
	auto pBase64Stream = queryInterface<io::IRawReadableStream>(createObject(CLSID_Base64Decoder,
		Dictionary({ {"stream", pDataStream} })));
	return variant::deserialize(pBase64Stream);
}

} // namespace ipc
} // namespace cmd
