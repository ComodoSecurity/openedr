//
// edrav2.libcore project
//
// Author: Yury Ermakov (31.01.2019)
// Reviewer: Denis Bogdanov (06.02.2019)
//
///
/// @file JSON-RPC support tools
///
/// @addtogroup ipc Inter-process communication
/// @{
#pragma once

namespace openEdr {
namespace ipc {

constexpr char c_sEncryptedFieldName[] = "5811728c";

///
/// Enum class for channel modes.
///
enum class ChannelMode : uint8_t
{
	Invalid = 0,	///< invalid
	Plain = 1,		///< plain channel is used (no encryption)
	Encrypted = 2,	///< encrypted channel is used
	Both = 3,		///< supports both plain and encrypted channels
};

///
/// Converts a channel mode (string) into enum value.
///
/// @param sMode - a channel mode string.
/// @return The function returns the value of ChannelMode enum.
///
ChannelMode getChannelModeFromString(std::string_view sMode);

///
/// Gets channel mode name string.
///
/// @param mode - a code of the operation.
/// @return The function returns a string with a name of the channel 
/// mode corresponding to a given code.
///
const char* getChannelModeString(ChannelMode mode);

///
/// JSON Value to Variant conversion.
///
/// @param value - JSON Value.
/// @return Variant value.
/// @throw error::TypeError for unsupported JSON value types
///
Variant convertJsonValueToVariant(const Json::Value& value);

///
/// Variant to JSON Value conversion.
///
/// @param vValue - Variant value.
/// @return JSON Value.
/// @throw error::TypeError for unsupported Variant value types.
///
Json::Value convertVariantToJsonValue(const Variant& vValue);

///
/// JSON Value encryption.
///
/// @param vData - Variant value to encrypt.
/// @param[out] result - result an encrypted JSON value.
/// @param vConfig - a configuration for the serializer.
///
void encryptJsonValue(const Variant& vData, Json::Value& result, const Variant& vConfig);

///
/// JSON Value decryption.
///
/// @param data - JSON value to decrypt.
/// @return Decrypted Variant value.
///
Variant decryptJsonValue(const Json::Value& data);

} // namespace ipc
} // namespace openEdr

/// @}
