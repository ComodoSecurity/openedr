//
// edrav2.libcore project
// 
// Author: Podpruzhnikov Yury (11.09.2019)
// Reviewer: 
//
///
/// @file Common hash calculation functions
///
#pragma once

namespace openEdr {
namespace crypt {

///
/// Hasher requirements:
/// - creates context in constructor (if needed)
/// - has typedef: Hash - hash type
/// - has method: void update(const void*, size)
/// - has method: Hash finalize()
///

///
/// Calculates and adds hash for Buffer
///
/// @param[in] hasher Hasher of specified algorithm
/// @param[in] pBuffer  Pointer to data
/// @param[in] nSize  Size of data
///
template<typename Hasher>
void updateHash(Hasher& hasher, const void* pBuffer, size_t nSize)
{
	hasher.update(pBuffer, nSize);
}

///
/// Calculates and adds hash for io::IRawReadableStream
///
/// @param[in] hasher Hasher of specified algorithm
/// @param[in] pStream  Pointer to stream with data
/// @param[in] nSize  Size of stream to proceed
/// @throw error::InvalidArgument Throw if pointer to buffer is NULL
/// @throw error::NoData Throw if nSize passed and not fully been read
///
template<typename Hasher>
void updateHash(Hasher& hasher, ObjPtr<io::IRawReadableStream> pStream, Size nSize = c_nMaxSize)
{
	if (pStream == nullptr)
		error::InvalidArgument("Source stream is null").throwException();

	Size nTotalSize = 0;
	while (nTotalSize < nSize)
	{
		Byte pBuffer[0x20000];
		Size nSize2Read = std::min(nSize - nTotalSize, sizeof(pBuffer));
		Size nReadSize = pStream->read(pBuffer, nSize2Read);
		hasher.update(pBuffer, nReadSize);
		nTotalSize += nReadSize;
		if (nReadSize < nSize2Read) break;
	}
	if (nSize != c_nMaxSize && nTotalSize < nSize)
		error::NoData(SL, "Can't calculate SHA1 of stream").throwException();
}

///
/// Calculates and adds hash for std::string_view<T>
///
/// @param[in] hasher Hasher of specified algorithm
/// @param[in] sStr String to calculate hash for
///
template<typename Hasher>
void updateHash(Hasher& hasher, std::wstring_view sStr)
{
	hasher.update(sStr.data(), sStr.length() * sizeof(wchar_t));
}

///
/// Calculates and adds hash for std::string_view<T>
///
/// @param[in] hasher Hasher of specified algorithm
/// @param[in] sStr  String to calculate hash for
///
template<typename Hasher>
void updateHash(Hasher& hasher, std::string_view sStr)
{
	hasher.update(sStr.data(), sStr.length());
}

///
/// Calculates and adds hash for integer and enum types
///
/// @param[in] hasher Hasher of specified algorithm
/// @param[in] value
///
template<typename Hasher, typename T, IsIntOrEnumType<T, int> = 0 >
void updateHash(Hasher& hasher, const T& value)
{
	hasher.update((const void*)&value, sizeof(value));
}

///
/// Calculates and adds hash for bool type
///
/// @param[in] hasher Hasher of specified algorithm
/// @param[in] value
///
template<typename Hasher, typename T, IsBoolType<T, int> = 0 >
void updateHash(Hasher& hasher, T fValue)
{
	updateHash(hasher, (uint8_t)fValue);
}


///
/// Way to calculate hash for variant
///
enum class VariantHash
{
	AsEvent, /// Special way for event and internal objects
	ViaJson,   /// Serialize to json and hasher hash
};

namespace detail {

//
//
//
template<typename Hasher>
void updateVariantHashViaJson(Hasher& hasher, const Variant& vVal)
{
	updateHash(hasher, serializeToJson(vVal, variant::JsonFormat::SingleLine));
}

//
//
//
template<typename Hasher>
void updateVariantHashAsEvent(Hasher& hasher, const Variant& vVal)
{
	switch (vVal.getType())
	{
	case variant::ValueType::Boolean:
		return updateHash(hasher, bool(vVal));
	case variant::ValueType::Integer:
		return updateHash(hasher, ((Variant::IntValue)(vVal)).asUnsigned());
	case variant::ValueType::String:
		return updateHash(hasher, std::string(vVal));
	}

	if (vVal.isDictionaryLike())
	{
		auto vId = vVal.getSafe("id");
		if(vId.has_value())
			return updateVariantHashAsEvent(hasher, vId.value());

		auto vHash = vVal.getSafe("hash");
		if (vHash.has_value())
			return updateVariantHashAsEvent(hasher, vHash.value());

		error::InvalidArgument(SL, "Fail to get unique id").throwException();
	}

	if (vVal.isSequenceLike())
	{
		for (auto vElem : vVal)
			updateVariantHashAsEvent(hasher, vElem);
		return;
	}

	error::InvalidArgument(SL, "Invalid data type for hash calculation").throwException();
}

} // namespace detail 

///
/// Calculates and adds hash for Varinant
///
/// @param[in] hasher Hasher of specified algorithm
/// @param[in] value
///
template<typename Hasher, typename T, IsVariantType<T, int> = 0 >
void updateHash(Hasher& hasher, T value, VariantHash eVariantHash)
{
	switch (eVariantHash)
	{
	case VariantHash::AsEvent:
		return detail::updateVariantHashAsEvent(hasher, value);
	case VariantHash::ViaJson:
		return detail::updateVariantHashViaJson(hasher, value);
	default:
		error::InvalidArgument(SL, FMT("Invalid Variant hash type: " << (size_t)eVariantHash)).throwException();
	}
}


///
/// Common function for fast hash calculation
/// Creates specified Hasher and 
/// Pass all params to updateHash
///
template<typename Hasher, class... Args>
auto getHash(Args&&... args)
{
	Hasher hasher;
	updateHash(hasher, std::forward<Args>(args)...);
	return hasher.finalize();
}



} // namespace crypt
} // namespace openEdr
