//
// edrav2.libcore project
//
// IO common functions
// 
// Author: Yury Ermakov (25.02.2019)
// Reviewer:
//
#include "pch.h"
#include <io.hpp>
#include <objects.h>

namespace cmd {
namespace io {

//
//
//
Variant serializeStream(ObjPtr<IReadableStream> pStream)
{
	// Converting stream to Base64 stream
	ObjPtr<IMemoryBuffer> pEncodedMemBuffer;
	pEncodedMemBuffer = queryInterface<IMemoryBuffer>(createObject(CLSID_MemoryStream));

	auto pDecoderStream = queryInterface<IRawWritableStream>(
		createObject(CLSID_Base64Encoder, Dictionary({ {"stream", pEncodedMemBuffer} })));
	auto nOldPos = pStream->setPosition(0);
	write(pDecoderStream, pStream, pStream->getSize());
	pDecoderStream->flush();
	pStream->setPosition(nOldPos);

	// Converting Base64 stream to string
	Variant vStringData;
	auto data = pEncodedMemBuffer->getData();
	std::string_view sData((char*)data.first, data.second);
	vStringData = sData;

	// Creating deserializer
	Variant vRes = Dictionary({
		{ "$$clsid", CLSID_RawReadableStreamConverter },
		{ "stream", Dictionary({
			{ "$$clsid", CLSID_Base64Decoder },
			{ "stream", Dictionary({
				{ "$$clsid", CLSID_StringReadableStream },
				{ "data", vStringData },
			})},
		})},
	});

	return vRes;
}

} // namespace io
} // namespace cmd 
