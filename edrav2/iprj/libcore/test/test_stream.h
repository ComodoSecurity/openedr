//
//  EDRAv2. Unit test
//    Streams.
//

using namespace cmd;

static const Size c_nStreamSize = 0x10;

void readRawStream(ObjPtr<io::IRawReadableStream> pStream);
void testRawWritableStream(ObjPtr<io::IRawWritableStream> pStream);
void readStream(ObjPtr<io::IReadableStream> pStream);
void writeStream(ObjPtr<io::IWritableStream> pStream);
void streamPosition(ObjPtr<io::IReadableStream> pStream);
void streamSize(ObjPtr<io::IWritableStream> pStream);
