//
// EDRAv2.libcore project
//
// LBVS serialization 
//
// Author: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (15.01.2019)
//
//
#pragma once
#include "pch.h"
#include <io.hpp>

namespace cmd {
namespace variant {
namespace lbvs {

CMD_DEFINE_ENUM_FLAG(FieldFlags)
{
	Value = 0x00,
	Dictionary = 0x01,
	Sequence = 0x02
};

//
//
//
Variant getData(Byte*& pPtr, size_t& nSize, size_t nDataSize)
{
	if (nSize < sizeof(lbvs::FieldHeader))
		error::InvalidFormat(FMT("Can't deserialize field at position " <<
			nDataSize - nSize)).throwException();

	lbvs::FieldType eType = ((lbvs::FieldHeader*)pPtr)->eType;
	pPtr += sizeof(lbvs::FieldHeader);
	nSize -= sizeof(lbvs::FieldHeader);

	Variant vData;
	switch (eType)
	{
		case lbvs::FieldType::String:
		{
			size_t nStrLen = strnlen_s((char*)pPtr, nSize);
			if (nStrLen == nSize)
				error::InvalidFormat(FMT("Can't deserialize field with type <" << int(eType) <<
					"> at position <" << nDataSize - nSize << ">")).throwException();
			// FIXME: Advice: Use iterator-like pointers
			vData = (char*)pPtr;
			pPtr += (nStrLen + 1) * sizeof(char);
			nSize -= (nStrLen + 1) * sizeof(char);
			break;
		}
		case lbvs::FieldType::WString:
		{
			size_t nStrLen = wcsnlen_s((wchar_t*)pPtr, nSize);
			if (nStrLen == nSize)
				error::InvalidFormat(FMT("Can't deserialize field with type <" << int(eType) <<
					"> at position <" << nDataSize - nSize << ">")).throwException();
			vData = (wchar_t*)pPtr;
			pPtr += (nStrLen + 1) * sizeof(wchar_t);
			nSize -= (nStrLen + 1) * sizeof(wchar_t);
			break;
		}
		case lbvs::FieldType::Stream:
		{
			if (nSize < sizeof(lbvs::StreamSize))
				error::InvalidFormat(FMT("Can't deserialize field with type <" << int(eType) <<
					"> at position <" << nDataSize - nSize << ">")).throwException();
			size_t nStreamSize = *(lbvs::StreamSize*)pPtr;
			pPtr += sizeof(lbvs::StreamSize);
			nSize -= sizeof(lbvs::StreamSize);

			if (nSize < nStreamSize)
				error::InvalidFormat(FMT("Can't deserialize field with type " << int(eType) <<
					" at position " << nDataSize - nSize)).throwException();
			ObjPtr<io::IRawWritableStream> pStream =
				queryInterface<io::IRawWritableStream>(io::createMemoryStream(nStreamSize));
			pStream->write(pPtr, nStreamSize);
			vData = pStream;
			pPtr += nStreamSize;
			nSize -= nStreamSize;
			break;
		}
		case lbvs::FieldType::Bool:
		{
			if (nSize < sizeof(bool))
				error::InvalidFormat(FMT("Can't deserialize field with type <" << int(eType) <<
					"> at position <" << nDataSize - nSize << ">")).throwException();
			vData = static_cast<bool>(*(uint8_t*)pPtr);
			pPtr += sizeof(uint8_t);
			nSize -= sizeof(uint8_t);
			break;
		}
		case lbvs::FieldType::Uint32:
		{
			if (nSize < sizeof(uint32_t))
				error::InvalidFormat(FMT("Can't deserialize field with type <" << int(eType) <<
					"> at position <" << nDataSize - nSize << ">")).throwException();
			vData = *(uint32_t*)pPtr;
			pPtr += sizeof(uint32_t);
			nSize -= sizeof(uint32_t);
			break;
		}
		case lbvs::FieldType::Uint64:
		{
			if (nSize < sizeof(uint64_t))
				error::InvalidFormat(FMT("Can't deserialize field with type <" << int(eType) <<
					"> at position <" << nDataSize - nSize << ">")).throwException();
			vData = *(uint64_t*)pPtr;
			pPtr += sizeof(uint64_t);
			nSize -= sizeof(uint64_t);
			break;
		}
		case lbvs::FieldType::SeqDict:
		{
			vData = Dictionary();
			break;
		}
		default:
			error::InvalidFormat(FMT("Can't deserialize field at position <" <<
				nDataSize - nSize << ">")).throwException();
	}

	return vData;
}

//
//
//
FieldFlags parsePath(std::string& sPath, std::string& sName)
{
/*
	// TODO: Add support of Sequence::Sequence::... hierarchy
	size_t nPos = 0;
	while (sPath[nPos] != 0 && sPath[nPos] != '.' && sPath[nPos] != '[')
		++nPos;
	FieldFlags eFlags = FieldFlags::Value;
	if (sPath[nPos] == '.')
	{
		setFlag(eFlags, FieldFlags::Dictionary);
		sName = sPath.substr(0, nPos);
		sPath = sPath.substr(nPos + 1);
	}
	else if (sPath[nPos] == '[')
	{
		setFlag(eFlags, FieldFlags::Sequence);
		sName = sPath.substr(0, nPos);
		sPath = sPath.substr(nPos + 2);
	}
	else
	{
		sName = sPath;
		sPath.clear();
	}
	return eFlags;
*/

	FieldFlags eFlags = FieldFlags::Value;
	std::string::size_type nDict = sPath.find(".");
	if (nDict == std::string::npos)
	{
		sName = sPath;
		sPath.clear();
	}
	else
	{
		sName = sPath.substr(0, nDict);
		sPath = sPath.substr(nDict + 1);
		setFlag(eFlags, FieldFlags::Dictionary);
	}

	std::string::size_type nSeq = sName.find("[]");
	if (nSeq != std::string::npos)
	{
		sName = sName.substr(0, nSeq);
		setFlag(eFlags, FieldFlags::Sequence);
	}

	return eFlags;
}

//
//
//
void putData(Variant vDst, std::string sPath, Variant vData)
{
	std::string sName;
	while (true)
	{
		FieldFlags eFlags = parsePath(sPath, sName);
		
		if (testFlag(eFlags, FieldFlags::Sequence))
		{
			if (vDst.has(sName))
			{
				if (!vDst.get(sName).isSequenceLike())
					error::InvalidFormat(FMT("Field <" << sName << "> is not a sequence")).throwException();
			}
			else
				vDst.put(sName, Sequence());
		}

		if (testFlag(eFlags, FieldFlags::Dictionary))
		{
			if (testFlag(eFlags, FieldFlags::Sequence))
			{
				Sequence vSeq = Sequence(vDst.get(sName));
				if (vSeq.isEmpty())
					vSeq.push_back(Dictionary());
				vDst = vSeq.get(vSeq.getSize() - 1);
			}
			else
			{
				if (!vDst.has(sName))
					vDst.put(sName, Dictionary());
				vDst = vDst.get(sName);
			}
		}
		else // It is just a value
		{
			if (testFlag(eFlags, FieldFlags::Sequence))
				vDst.get(sName).push_back(vData);
			else
				vDst.put(sName, vData);
			break;
		}
	}
}

//
//
//
Variant deserialize(const void* pData, size_t nDataSize, Variant vSchema)
{
	if (pData == nullptr)
		error::InvalidArgument("Pointer to pData can't be null").throwException();

	if (nDataSize < sizeof(lbvs::LbvsHeader) ||
		nDataSize < ((lbvs::LbvsHeader*)pData)->nSize)
		error::InvalidFormat("Size of data is too small").throwException();

	Variant vResult = Dictionary();
	Byte* pPtr = (Byte*)pData + sizeof(lbvs::LbvsHeader);
	size_t nSize = ((lbvs::LbvsHeader*)pData)->nSize - sizeof(lbvs::LbvsHeader);
	size_t nCount = ((lbvs::LbvsHeader*)pData)->nCount;
	Sequence sqFields = Sequence(vSchema["fields"]);
	auto nFieldsCount = sqFields.getSize();
	for (size_t i = 0; i < nCount; ++i)
	{
		FieldId eId = ((lbvs::FieldHeader*)pPtr)->eId;
		if (eId < nFieldsCount)
		{
			Variant vData = getData(pPtr, nSize, nDataSize);
			putData(vResult, sqFields[eId]["name"], vData);
		}
		else
		{
			// FIXME: We need error only if 'version' field is lower or the same
			error::InvalidFormat(FMT("Incorrect FieldId <" << eId << "> offset <" <<
				nDataSize - nSize << "> size <" << nSize << ">")).log();
			(void)getData(pPtr, nSize, nDataSize); // Try to skip field
		}
	}
	
	if (nSize > 0)
		LOGWRN("Unused data left after deserializing, offset <" << nDataSize - nSize << ">, size <" << nSize << ">");
	return vResult;
}

using FieldsArray = std::vector<std::pair<std::string, FieldType>>;

//
//
//
std::pair<FieldId, FieldType> findFieldInfo(const FieldsArray& vFields, const std::string& sFieldName)
{
	for (auto itField = vFields.begin(); itField != vFields.end(); ++itField)
	{
		if (itField->first == sFieldName)
			return std::pair(FieldId(itField - vFields.begin()), itField->second);
	}
	return std::pair(g_nInvalidFieldId, FieldType::Null);
}

//
//
// FIXME: Remove return value
// If field is absent in schema, skip it
// If write() returns false, throw exception
//
bool serializeField(Variant vData, const std::string& sPath, const FieldsArray& vFields, LbvsSerializer<FieldId>& serializer)
{
	auto eType = vData.getType();
	switch (eType)
	{
		case ValueType::Null:
		case ValueType::Object:
		{
			break;
		}
		case ValueType::Boolean:
		{
			auto field = findFieldInfo(vFields, sPath);
			if (field.first == g_nInvalidFieldId)
				return false;

			if (field.second == FieldType::Null || field.second == FieldType::Bool)
				return serializer.write(field.first, bool(vData));

			error::InvalidFormat(FMT("Invalid type of field <" << sPath << ">, " << 
				"variant <Boolean>, schema <" << uint8_t(field.second) << ">"));
		}
		case ValueType::Integer:
		{
			auto field = findFieldInfo(vFields, sPath);
			if (field.first == g_nInvalidFieldId)
				return false;

			if (field.second == FieldType::Null || field.second == FieldType::Uint64)
				return serializer.write(field.first, uint64_t(vData));
			if (field.second == FieldType::Uint32)
				return serializer.write(field.first, uint32_t(vData));

			error::InvalidFormat(FMT("Invalid type of field <" << sPath << ">., " <<
				"variant <Integer>, schema <" << uint8_t(field.second) << ">"));
		}
		case ValueType::String:
		{
			auto field = findFieldInfo(vFields, sPath);
			if (field.first == g_nInvalidFieldId)
				return false;

			if (field.second == FieldType::Null || field.second == FieldType::WString)
				return serializer.write(field.first, std::wstring(vData));
			if (field.second == FieldType::String)
				return serializer.write(field.first, std::string(vData));

			error::InvalidFormat(FMT("Invalid type of field <" << sPath << ">, " <<
				"variant <String>, schema <" << uint8_t(field.second) << ">"));
		}
		case ValueType::Dictionary:
		{
			// FIXME: Why you don't do it for other types
			if (!sPath.empty() && sPath.back() == ']')
			{
				auto field = findFieldInfo(vFields, sPath);
				if (field.first == g_nInvalidFieldId || 
					!serializer.write(field.first))
					return false;
			}

			bool fResult = true;
			auto sItemPath = sPath + (sPath.empty() ? "" : ".");
			for (auto item : Dictionary(vData))
			{
				if (!serializeField(item.second, sItemPath + std::string(item.first), vFields, serializer))
					fResult = false;
			}
			return fResult;
		}
		case ValueType::Sequence:
		{
			bool fResult = true;
			auto sItemPath = sPath + "[]";
			for (auto item : Sequence(vData))
			{
				if (!serializeField(item, sItemPath, vFields, serializer))
					fResult = false;
			}
			return fResult;
		}
	}
	LOGLVL(Trace, "Fail to parse a serialize field <" << sPath << ">");
	return false;
}

//
//
// FIXME: serialize() should return std::vector<uint8_t>
// FIXME: But we whould be sure that move semantic is used or you can just allocate memory and return smart pointer
//
bool serialize(Variant vData, Variant vSchema, std::vector<uint8_t>& pData)
{
	Sequence vFieldsList(vSchema.get("fields", Sequence()));
	FieldsArray vFields;
	for (auto field : vFieldsList)
	{
		std::string sName(field.get("name", ""));
		if (sName.empty())
			continue;

		FieldType eType = FieldType::Null;
		Variant vType = field.get("type", eType);
		if (vType.getType() == Variant::ValueType::Integer)
		{
			eType = vType;
		}
		else if(vType.getType() == Variant::ValueType::String)
		{
			eType = convertFromString<FieldType>(vType);
			if (eType == FieldType::Invalid)
				error::InvalidArgument(SL, FMT("Invalid value of the 'type' field <" <<
					vType << ">, 'name' <" << sName << ">")).throwException();
		}
		else
		{
			error::InvalidArgument(SL, FMT("Invalid type of the 'type' field <" <<
				vType << ">, 'name' <" << sName << ">")).throwException();
		}

		vFields.push_back(std::pair(sName, eType));
	}
	if (vFields.empty())
		return false;

	LbvsSerializer<FieldId> serializer;
	auto result = serializeField(vData, "", vFields, serializer);

	void* pBuffer = nullptr;
	size_t nSize = 0;
	if (!serializer.getData(&pBuffer, &nSize))
		return false;

	pData.resize(nSize);
	std::copy((uint8_t*)pBuffer, (uint8_t*)pBuffer + nSize, pData.begin());
	return result;
}

} // namespace lbvs


//
//
//
Variant deserializeFromLbvs(const void* pBuffer, size_t nBufferSize, Variant vSchema, bool fObjFlag)
{
	Variant vResult = lbvs::deserialize(pBuffer, nBufferSize, vSchema);
	return fObjFlag ? deserializeObject(vResult, true) : vResult;
}

//
//
//
Variant deserializeFromLbvs(ObjPtr<io::IRawReadableStream> pStream, Variant vSchema, bool fObjFlag)
{
	TRACE_BEGIN;
	auto pReadableStream = io::convertStream(pStream);

	auto nSize = pReadableStream->getSize();
	if (nSize == 0)
		error::NoData(SL, "Source stream is empty").throwException();

	std::vector<char> sBuffer;
	sBuffer.resize((size_t)nSize);
	if (pReadableStream->read(sBuffer.data(), sBuffer.size()) != sBuffer.size())
		error::NoData(SL).throwException();

	return deserializeFromLbvs(sBuffer.data(), sBuffer.size(), vSchema, fObjFlag);
	TRACE_END("Can't deserialize LBVS stream");
}

//
//
//
bool serializeToLbvs(Variant vData, Variant vSchema, std::vector<uint8_t>& pData)
{
	return lbvs::serialize(vData, vSchema, pData);
}

} // namespace variant
} // namespace cmd 
