//
// edrav2.libcore project
//
// Author: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (11.12.2018)
//
///
/// @file Variant serialization/deserialization
///
/// @addtogroup basicDataObjects Basic data objects
/// @{
#pragma once
#include "variant.hpp"
#include "lbvs.hpp"

namespace cmd {
namespace io {

class IRawWritableStream;
class IRawReadableStream;

} // namespace io

namespace variant {

///
/// JSON format enum.
///
enum class JsonFormat
{
	Pretty, ///< Multi-line pretty-formatted output
	SingleLine, ///< Single-line output
	Default = SingleLine, /// Default is single-line output
};

namespace detail {

//
//
//
void serializeToJson(ObjPtr<io::IRawWritableStream> pStream, const Variant& vVal, const JsonFormat fmt);

//
//
//
void serialize(ObjPtr<io::IRawWritableStream> pStream, const Variant& vVal, const Variant& vConfig);

}

///
/// Serialization variant to JSON to string.
///
/// @param vVal - Variant for serialization.
/// @param fmt [opt] - JSON format. By default, single-line JSON is used.
/// @returns The function returns a text string with serialized data.
/// @throw error::TypeError If vVal or its subelement can't be serialized.
///
[[nodiscard]] std::string serializeToJson(const Variant& vVal, const JsonFormat fmt = JsonFormat::Default);

///
/// Serialization variant to JSON to stream.
///
/// @param pStream - output stream.
/// @param vVal - variant for serialization.
/// @param fmt [opt] - JSON format. By default, single-line JSON is used.
/// @throw error::TypeError If vVal or its subelement can't be serialized
///
template<typename I>
void serializeToJson(ObjPtr<I> pStream, const Variant& vVal, const JsonFormat fmt = JsonFormat::Default)
{
	detail::serializeToJson(pStream, vVal, fmt);
}

///
/// Serialization variant to stream.
///
/// @param pStream - output stream.
/// @param vVal - Variant for serialization.
/// @param vCfg [opt] - the serializer's config (default: blank).
/// @throw error::TypeError If vVal or its subelement can't be serialized.
///
template<typename I>
void serialize(ObjPtr<I> pStream, const Variant& vVal, const Variant& vCfg = {})
{
	detail::serialize(pStream, vVal, vCfg);
}

///
/// Deserialization variant from JSON string.
///
/// @param sUtf8Str - input string.
/// @returns The function returns a deserialized Variant value.
/// @throw error::InvalisFormat If JSON string has invalid format or unsupported value.
///
Variant deserializeFromJson(const std::string_view& sUtf8Str);

///
/// Deserialization of variant from JSON stream (utf8).
/// 
/// @param pStream - input stream.
/// @returns The function returns a deserialized Variant value.
/// @throw error::InvalidFormat If JSON has invalid format or unsupported value.
/// @throw error::NoData If the input stream is empty.
///
Variant deserializeFromJson(ObjPtr<io::IRawReadableStream> pStream);

///
/// Deserialization of variant from stream.
/// 
/// @param pStream - input stream.
/// @returns The function returns a deserialized Variant value.
/// @throw error::InvalidFormat If invalid format or unsupported value.
/// @throw error::NoData If the input stream is empty.
///
Variant deserialize(ObjPtr<io::IRawReadableStream> pStream);

///
/// Deserialize variant from LBVS buffer.
/// 
/// @param pBuffer - pointer to input buffer.
/// @param nBufferSize - size of input buffer.
/// @param vSchema - Variant deserialization schema.
/// @param fObjFlag [opt] - create objects defined with "$$clsid" (default: false).
/// @returns The function returns a deserialized Variant value.
/// @throw error::InvalidFormat If data has invalid format or unsupported value
///
Variant deserializeFromLbvs(const void* pBuffer, size_t nBufferSize, Variant vSchema, bool fObjFlag = false);

///
/// Deserialize variant from LBVS stream.
///
/// @param pStream - input stream.
/// @param vSchema - Variant deserialization schema.
/// @param fObjFlag [opt] - create objects defined with "$$clsid" (default: false).
/// @returns The function returns a deserialized Variant value.
/// @throw error::InvalidFormat If data has invalid format or unsupported value.
/// @throw error::NoData If the input stream is empty.
///
Variant deserializeFromLbvs(ObjPtr<io::IRawReadableStream> pStream, Variant vSchema, bool fObjFlag = false);

///
/// Serialize variant to LBVS stream.
///
/// @param vData - input variant.
/// @param vSchema - variant deserialization schema.
/// @param pData - link to data storage (std::vector).
/// @return The function returns `false` if not all fields of variant converted to LVBS.
/// @throw error::InvalidFormat If data has invalid format or formats in variant and schema are different.
///
bool serializeToLbvs(Variant vData, Variant vSchema, std::vector<uint8_t>& pData);

} // namespace variant
} // namespace cmd 

/// @}
