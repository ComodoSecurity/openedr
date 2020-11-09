//
// edrav2.libcore
// 
// Author: Yury Ermakov (29.12.2018)
// Reviewer: Denis Bogdanov (25.02.2018)
//
///
/// @file DictionaryProxy and SequenceProxy classes implementation
///
#include "pch.h"
#include "variantproxy.h"

namespace openEdr {
namespace variant {

//////////////////////////////////////////////////////////////////////////////
//
// DictionaryProxy
//
//////////////////////////////////////////////////////////////////////////////

//
//
//
ObjPtr<DictionaryProxy> DictionaryProxy::create(Dictionary value,
	DictionaryPreModifyCallback fnPreModify, DictionaryPostModifyCallback fnPostModify,
	DictionaryPreClearCallback fnPreClear, DictionaryPostClearCallback fnPostClear,
	CreateProxyCallback fnCreateDictionaryProxy, CreateProxyCallback fnCreateSequenceProxy)
{
	auto pObj = createObject<DictionaryProxy>();
	pObj->m_data = value;
	pObj->callPreModify = fnPreModify;
	pObj->callPostModify = fnPostModify;
	pObj->callPreClear = fnPreClear;
	pObj->callPostClear = fnPostClear;
	pObj->callCreateDictionaryProxy = fnCreateDictionaryProxy;
	pObj->callCreateSequenceProxy = fnCreateSequenceProxy;
	return pObj;
}

//
//
//
Size DictionaryProxy::getSize() const
{
	return m_data.getSize();
}

//
//
//
bool DictionaryProxy::isEmpty() const
{
	return m_data.isEmpty();
}

//
//
//
void DictionaryProxy::clear()
{
	if (callPreClear) callPreClear();
	m_data.clear();
	if (callPostClear) callPostClear();
}

//
//
//
std::unique_ptr<IIterator> DictionaryProxy::createIterator(bool fUnsafe) const
{
	auto innerIter = Variant(m_data).createIterator(fUnsafe);
	return std::make_unique<DictionaryIteratorProxy>(std::move(innerIter),
		callCreateDictionaryProxy, callCreateSequenceProxy);
}

//
//
//
bool DictionaryProxy::has(DictionaryKeyRefType sName) const
{
	return m_data.has(sName);
}

//
//
//
Size DictionaryProxy::erase(DictionaryKeyRefType sName)
{
	if (callPreModify) (void)callPreModify(sName, m_data.get(sName));
	if (callPostModify)
	{
		Variant vValue = m_data.get(sName);
		Size nSize = m_data.erase(sName);
		callPostModify(sName, vValue);
		return nSize;
	}
	return m_data.erase(sName);
}

//
//
//
Variant DictionaryProxy::get(DictionaryKeyRefType sName) const
{
	Variant vValue = m_data.get(sName);
	if (callCreateDictionaryProxy && vValue.isDictionaryLike())
		return callCreateDictionaryProxy(vValue);
	if (callCreateSequenceProxy && vValue.isSequenceLike())
		return callCreateSequenceProxy(vValue);
	return vValue;
}

//
//
//
std::optional<Variant> DictionaryProxy::getSafe(DictionaryKeyRefType sName) const
{
	if (!m_data.has(sName))
		return {};
	return m_data.get(sName);
}

//
//
//
void DictionaryProxy::put(DictionaryKeyRefType sName, const Variant& Value)
{
	Variant vNewValue = callPreModify ? callPreModify(sName, Value) : Value;
	m_data.put(sName, vNewValue);
	if (callPostModify) callPostModify(sName, vNewValue);
}

//
//
//
Variant DictionaryProxy::clone()
{
	return m_data.clone();
}

//
//
//
std::string DictionaryProxy::print() const
{
	return m_data.print();
}

//
//
//
Variant createDictionaryProxy(Variant vValue,
	DictionaryPreModifyCallback fnPreModify, DictionaryPostModifyCallback fnPostModify,
	DictionaryPreClearCallback fnPreClear, DictionaryPreClearCallback fnPostClear,
	CreateProxyCallback fnCreateDictionaryProxy, CreateProxyCallback fnCreateSequenceProxy)
{
	if (!vValue.isDictionaryLike())
		error::TypeError(SL, "Only Dictionary or IDictionaryProxy-based object are supported").throwException();
	return DictionaryProxy::create(vValue, fnPreModify, fnPostModify, fnPreClear, fnPostClear,
		fnCreateDictionaryProxy, fnCreateSequenceProxy);
}

//////////////////////////////////////////////////////////////////////////////
//
// SequenceProxy
//
//////////////////////////////////////////////////////////////////////////////

//
//
//
ObjPtr<SequenceProxy> SequenceProxy::create(Sequence value,
	SequencePreModifyCallback fnPreModify, SequencePostModifyCallback fnPostModify,
	SequencePreResizeCallback fnPreResize, SequencePostResizeCallback fnPostResize,
	CreateProxyCallback fnCreateDictionaryProxy, CreateProxyCallback fnCreateSequenceProxy)
{
	auto pObj = createObject<SequenceProxy>();
	pObj->m_data = value;
	pObj->callPreModify = fnPreModify;
	pObj->callPostModify = fnPostModify;
	pObj->callPreResize = fnPreResize;
	pObj->callPostResize = fnPostResize;
	pObj->callCreateDictionaryProxy = fnCreateDictionaryProxy;
	pObj->callCreateSequenceProxy = fnCreateSequenceProxy;
	return pObj;
}

//
//
//
Size SequenceProxy::getSize() const
{
	return m_data.getSize();
}

//
//
//
bool SequenceProxy::isEmpty() const
{
	return m_data.isEmpty();
}

//
//
//
Size SequenceProxy::erase(Size nIndex)
{
	if (callPreModify) (void)callPreModify(nIndex, m_data.get(nIndex));
	if (callPostModify)
	{
		Variant vValue = m_data.get(nIndex);
		Size nSize = m_data.erase(nIndex);
		callPostModify(nIndex, vValue);
		return nSize;
	}
	return m_data.erase(nIndex);
}

//
//
//
void SequenceProxy::clear()
{
	if (callPreModify) (void)callPreModify(c_nMaxSize, nullptr);
	m_data.clear();
	if (callPostModify) callPostModify(c_nMaxSize, nullptr);
}

//
//
//
Variant SequenceProxy::get(Size nIndex) const
{
	return m_data.get(nIndex);
}

//
//
//
std::optional<Variant> SequenceProxy::getSafe(Size nIndex) const
{
	if (nIndex >= m_data.getSize())
		return {};
	return m_data.get(nIndex);
}

//
//
//
void SequenceProxy::put(Size nIndex, const Variant& Value)
{
	Variant vNewValue = callPreModify ? callPreModify(nIndex, Value) : Value;
	m_data.put(nIndex, vNewValue);
	if (callPostModify) callPostModify(nIndex, vNewValue);
}

//
//
//
void SequenceProxy::push_back(const Variant& vValue)
{
	Variant vNewValue = callPreModify ? callPreModify(m_data.getSize(), vValue) : vValue;
	m_data.push_back(vNewValue);
	if (callPostModify) callPostModify(m_data.getSize() - 1, vNewValue);
}

//
//
//
void SequenceProxy::setSize(Size nSize)
{
	if (callPreResize) nSize = callPreResize(nSize);
	m_data.setSize(nSize);
	if (callPostResize) callPostResize(nSize);
}

//
//
//
void SequenceProxy::insert(Size nIndex, const Variant& vValue)
{
	Variant vNewValue = callPreModify ? callPreModify(nIndex, vValue) : vValue;
	m_data.insert(nIndex, vNewValue);
	if (callPostModify) callPostModify(nIndex, vNewValue);
}

//
//
//
std::unique_ptr<IIterator> SequenceProxy::createIterator(bool fUnsafe) const
{
	auto innerIter = Variant(m_data).createIterator(fUnsafe);
	return std::make_unique<SequenceIteratorProxy>(std::move(innerIter),
		callCreateDictionaryProxy, callCreateSequenceProxy);
}

//
//
//
Variant SequenceProxy::clone()
{
	return m_data.clone();
}

//
//
//
std::string SequenceProxy::print() const
{
	return m_data.print();
}

//
//
//
Variant createSequenceProxy(Variant vValue,
	SequencePreModifyCallback fnPreModify, SequencePostModifyCallback fnPostModify,
	SequencePreResizeCallback fnPreResize, SequencePostResizeCallback fnPostResize,
	CreateProxyCallback fnCreateDictionaryProxy, CreateProxyCallback fnCreateSequenceProxy)
{
	if (!vValue.isSequenceLike())
		error::TypeError(SL, "Only Dictionary or IDictionaryProxy-based object are supported").throwException();
	return SequenceProxy::create(Sequence(vValue), fnPreModify, fnPostModify, fnPreResize, fnPostResize,
		fnCreateDictionaryProxy, fnCreateSequenceProxy);
}

} // namespace variant
} // namespace openEdr
