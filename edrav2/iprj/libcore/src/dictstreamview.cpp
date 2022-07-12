//
// EDRAv2.libcore project.
//
// Author: Yury Ermakov (29.12.2018)
// Reviewer: Denis Bogdanov (10.01.2019)
//
///
/// @file DictionaryStreamView object implementaion
/// 
#include "pch.h"
#include "dictstreamview.h"
#include "variantproxy.h"

namespace cmd {
namespace io {

//////////////////////////////////////////////////////////////////////////////
//
// DictionaryStreamView methods
//
//////////////////////////////////////////////////////////////////////////////

//
//
//
void DictionaryStreamView::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;
	if (!vConfig.has("stream"))
		error::InvalidArgument(SL, "Missing field 'stream'").throwException();

	m_fReadOnly = vConfig.get("readOnly", false);
	m_pWriteStream = queryInterface<io::IWritableStream>(vConfig["stream"]);
	if (vConfig.has("default"))
		m_optDefault = vConfig["default"].clone();
	reload();
	TRACE_END("Failed to create an object");
}

//
//
//
Variant DictionaryStreamView::createDictionaryProxy(Variant vValue) const
{
	auto pThis = getPtrFromThis(this);
	return variant::createDictionaryProxy(vValue,
		// fnPreModify
		[pThis](const variant::DictionaryKeyRefType&, Variant vValue)
		{
			if (pThis->m_fReadOnly)
				error::InvalidUsage(SL, "Object is read-only").throwException();
			return vValue.clone();
		},
		// fnPostModify
		[pThis](const variant::DictionaryKeyRefType&, Variant)
		{
			TRACE_BEGIN;
			pThis->flush();
			TRACE_END("Can't modify data");
		},
		// fnPreClear
		[pThis]()
		{
			if (pThis->m_fReadOnly)
				error::InvalidUsage(SL, "Object is read-only").throwException();
		},
		// fnPostClear
		[pThis]()
		{
			TRACE_BEGIN;
			pThis->flush();
			TRACE_END("Can't clear data");
		},
		// fnCreateDictionaryProxy
		[pThis](Variant vValue)
		{
			return pThis->createDictionaryProxy(vValue);
		},
		// fnCreateSequenceProxy
		[pThis](Variant vValue)
		{
			return pThis->createSequenceProxy(vValue);
		});
}

//
//
//
Variant DictionaryStreamView::createSequenceProxy(Variant vValue) const
{
	auto pThis = getPtrFromThis(this);
	return variant::createSequenceProxy(vValue,
		// fnPreModify
		[pThis](Size, Variant vValue)
		{
			if (pThis->m_fReadOnly)
				error::InvalidUsage(SL, "Object is read-only").throwException();
			return vValue.clone();
		},
		// fnPostModify
		[pThis](Size, Variant)
		{
			TRACE_BEGIN;
			pThis->flush();
			TRACE_END("Can't modify data");
		},
		// fnPreResize
		[pThis](Size nSize)
		{
			if (pThis->m_fReadOnly)
				error::InvalidUsage(SL, "Object is read-only").throwException();
			return nSize;
		},
		// fnPostResize
		[pThis](Size)
		{
			TRACE_BEGIN;
			pThis->flush();
			TRACE_END("Can't resize");
		},
		// fnCreateDictionaryProxy
		[pThis](Variant vValue)
		{
			return pThis->createDictionaryProxy(vValue);
		},
		// fnCreateSequenceProxy
		[pThis](Variant vValue)
		{
			return pThis->createSequenceProxy(vValue);
		});
}

//
//
//
void DictionaryStreamView::flush()
{
	TRACE_BEGIN;
	CMD_TRY
	{
		m_pWriteStream->setSize(0);
		variant::serialize(m_pWriteStream, m_data);
		m_pWriteStream->flush();
	}
	CMD_PREPARE_CATCH
	catch (error::CrtError& e)
	{
		e.log(SL, "Can't flush DictionaryStreamView");
	}
	TRACE_END("Can't flush DictionaryStreamView");
}

//////////////////////////////////////////////////////////////////////////////
//
// IDictionaryStreamView methods
//
//////////////////////////////////////////////////////////////////////////////

//
//
//
void DictionaryStreamView::reload()
{
	TRACE_BEGIN;
	auto pReadStream = queryInterface<io::IReadableStream>(m_pWriteStream);
	pReadStream->setPosition(0);
	CMD_TRY
	{
		m_data = Dictionary(variant::deserializeFromJson(pReadStream));
	}
	CMD_PREPARE_CATCH
	catch (error::NoData&)
	{
		m_data = Dictionary();
	}
	catch (error::Exception& ex)
	{
		LOGLVL(Trace, "Failed to load data to DictionaryStreamView:\n" << variant::serializeToJson(pReadStream));
		if (!m_optDefault.has_value())
			throw;

		ex.log(SL, "Failed to load data from stream");
		m_data = Dictionary(m_optDefault.value());
		flush();
	}
	TRACE_END("Can't load data from stream");
}

//////////////////////////////////////////////////////////////////////////////
//
// IDictionaryProxy methods
//
//////////////////////////////////////////////////////////////////////////////

//
//
//
Size DictionaryStreamView::getSize() const
{
	return m_data.getSize();
}

//
//
//
bool DictionaryStreamView::isEmpty() const
{
	return m_data.isEmpty();
}

//
//
//
bool DictionaryStreamView::has(variant::DictionaryKeyRefType sName) const
{
	return m_data.has(sName);
}

//
//
//
Size DictionaryStreamView::erase(variant::DictionaryKeyRefType sName)
{
	if (m_fReadOnly)
		error::InvalidUsage(SL, "Object is read-only").throwException();
	TRACE_BEGIN;
	auto nCount = m_data.erase(sName);
	flush();
	return nCount;
	TRACE_END(FMT("Can't erase item <" << static_cast<std::string>(sName) << ">"));
}

//
//
//
void DictionaryStreamView::clear()
{
	if (m_fReadOnly)
		error::InvalidUsage(SL, "Object is read-only").throwException();
	TRACE_BEGIN;
	m_data.clear();
	flush();
	TRACE_END("Can't clear DictionaryStreamView");
}

//
//
//
Variant DictionaryStreamView::get(variant::DictionaryKeyRefType sName) const
{
	Variant vValue = m_data.get(sName);
	if (vValue.isDictionaryLike())
		return createDictionaryProxy(vValue);
	if (vValue.isSequenceLike())
		return createSequenceProxy(vValue);
	return vValue;
}

//
//
//
std::optional<Variant> DictionaryStreamView::getSafe(variant::DictionaryKeyRefType sName) const
{
	if (!has(sName))
		return {};
	return get(sName);
}

//
//
//
void DictionaryStreamView::put(variant::DictionaryKeyRefType sName, const Variant& Value)
{
	if (m_fReadOnly)
		error::InvalidUsage(SL, "Object is read-only").throwException();
	TRACE_BEGIN;
	m_data.put(sName, Value);
	flush();
	TRACE_END(FMT("Can't put item <" << sName << ">"));
}

//
//
//
std::unique_ptr<variant::IIterator> DictionaryStreamView::createIterator(bool fUnsafe) const
{
	auto innerIter = Variant(m_data).createIterator(fUnsafe);
	auto pThis = getPtrFromThis(this);
	return std::make_unique<variant::DictionaryIteratorProxy>(std::move(innerIter),
		// fnCreateDictionaryProxy
		[pThis](Variant vValue)
		{
			return pThis->createDictionaryProxy(vValue);
		},
		// fnCreateSequenceProxy
		[pThis](Variant vValue)
		{
			return pThis->createSequenceProxy(vValue);
		});
}

//
//
//
std::string DictionaryStreamView::print() const
{
	return m_data.print();
}

//
//
//
Variant DictionaryStreamView::clone()
{
	return m_data.clone();
}
} // namespace io
} // namespace cmd 
