//
// EDRAv2.libcore project
//
// Library classes registration
//
#include "pch.h"

#include "object.hpp"
#include "application.hpp"

#include "logsink.h"
#include "stream.h"
#include "filestream.h"
#include "filestream-win.h"
#include "memstream.h"
#include "streamcache.h"
#include "command.h"
#include "ctxcmds.h"
#include "variantcmd.h"
#include "servicemgr.h"
#include "console.h"
#include "base64.h"
#include "dictstreamview.h"
#include "jsonrpcclient.h"
#include "jsonrpcserver.h"
#include "scenariomgr.h"
#include "scenario.h"
#include "queue.h"
#include "queuemgr.h"
#include "dictionarymixer.h"
#include "stringmatcher.h"
#include "datagenerator.h"
#include "fsdatastorage.h"
#include "aes.h"
#include "serializer.h"
#include "cache.h"
#include "localeventslogfile.h"

//
// Classes registration
//
CMD_BEGIN_LIBRARY_DEFINITION(libcore)
CMD_DEFINE_LIBRARY_CLASS(logging::Log4CPlusSink)
CMD_DEFINE_LIBRARY_CLASS(io::MemoryStream)
CMD_DEFINE_LIBRARY_CLASS(io::RawReadableStreamCache)
CMD_DEFINE_LIBRARY_CLASS(io::ReadableStreamCache)
CMD_DEFINE_LIBRARY_CLASS(io::RawReadableStreamConverter)
CMD_DEFINE_LIBRARY_CLASS(io::ChildStream)
CMD_DEFINE_LIBRARY_CLASS(io::StringReadableStream)
CMD_DEFINE_LIBRARY_CLASS(io::File)
CMD_DEFINE_LIBRARY_CLASS(io::win::File)
CMD_DEFINE_LIBRARY_CLASS(VariantCmd)
CMD_DEFINE_LIBRARY_CLASS(VariantCtxCmd)
CMD_DEFINE_LIBRARY_CLASS(CallCtxCmd)
CMD_DEFINE_LIBRARY_CLASS(ConditionalCtxCmd)
CMD_DEFINE_LIBRARY_CLASS(ForEachCtxCmd)
CMD_DEFINE_LIBRARY_CLASS(MakeDictionaryCtxCmd)
CMD_DEFINE_LIBRARY_CLASS(MakeSequenceCtxCmd)
CMD_DEFINE_LIBRARY_CLASS(CachedValueCtxCmd)
CMD_DEFINE_LIBRARY_CLASS(Command)
CMD_DEFINE_LIBRARY_CLASS(Scenario)
CMD_DEFINE_LIBRARY_CLASS(ScenarioManager)
CMD_DEFINE_LIBRARY_CLASS(CommandDataReceiver)
CMD_DEFINE_LIBRARY_CLASS(ServiceManager)
CMD_DEFINE_LIBRARY_CLASS(crypt::Base64Encoder)
CMD_DEFINE_LIBRARY_CLASS(crypt::Base64Decoder)
CMD_DEFINE_LIBRARY_CLASS(io::DictionaryStreamView)
CMD_DEFINE_LIBRARY_CLASS(Application)
CMD_DEFINE_LIBRARY_CLASS(io::Console)
CMD_DEFINE_LIBRARY_CLASS(ipc::JsonRpcClient)
CMD_DEFINE_LIBRARY_CLASS(ipc::JsonRpcServer)
CMD_DEFINE_LIBRARY_CLASS(Queue)
CMD_DEFINE_LIBRARY_CLASS(QueueManager)
CMD_DEFINE_LIBRARY_CLASS(QueueProxy)
CMD_DEFINE_LIBRARY_CLASS(variant::DictionaryMixer)
CMD_DEFINE_LIBRARY_CLASS(string::StringMatcher)
CMD_DEFINE_LIBRARY_CLASS(io::NullStream)
CMD_DEFINE_LIBRARY_CLASS(DataGenerator)
CMD_DEFINE_LIBRARY_CLASS(RandomDataGenerator)
CMD_DEFINE_LIBRARY_CLASS(io::FsDataStorage)
CMD_DEFINE_LIBRARY_CLASS(crypt::AesEncoder)
CMD_DEFINE_LIBRARY_CLASS(crypt::AesDecoder)
CMD_DEFINE_LIBRARY_CLASS(variant::StreamSerializer)
CMD_DEFINE_LIBRARY_CLASS(variant::StreamDeserializer)
CMD_DEFINE_LIBRARY_CLASS(StringCache)
CMD_DEFINE_LIBRARY_CLASS(io::LocalEventsLogFile)
CMD_END_LIBRARY_DEFINITION(libcore)
