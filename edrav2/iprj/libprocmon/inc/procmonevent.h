#pragma once
#include "objects.h"
#include "procmon.hpp"

namespace cmd {
namespace win {	
	class ProcmonEvent
	{
		const void* pMessageBuf;
		unsigned long dwMessageLen;
		void* pAnswerBuf;
		unsigned long dwAnswerLen;
		bool fServiceEnabled;
		ObjPtr<IDataReceiver> receiver;
		const Variant& eventSchema;
		const Variant& injectionConfig;
		const Variant& configSchema;
		ClassId nClassId;

		void log(const Variant& vEvent);
		void parseEvent(edrpm::RawEvent eEvent, Variant vEvent);
		void updateConfig(bool& fHasAnswer);
		edrpm::RawEvent adjustEventId(edrpm::RawEvent eventId);

	public:
		ProcmonEvent(const Byte* _pMessageBuf,
			unsigned long _dwMessageLen, Byte* _pAnswerBuf, unsigned long _dwAnswerLen,
			bool _fServiceEnabled, ObjPtr<IDataReceiver>& _receiver, const Variant& _eventSchema,
			const Variant& _injectionConfig, const Variant& _configSchema, ClassId _nClassId);
		bool handle(bool& fHasAnswer);
		
	};
}
}