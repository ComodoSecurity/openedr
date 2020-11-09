//
// DictionaryProxy implementation.
// 
// This file is a part of EDRAv2.libcore project.
//
// Author: Yury Ermakov (29.12.2018)
// Reviewer: Denis Bogdanov (10.01.2019)
//
#include "pch.h"

#include "dictionaryproxy.h"

namespace cmd {
namespace variant {

//
//
//
ObjPtr<DictionaryProxy> DictionaryProxy::create(Dictionary value, PreWriteCallback fnPreWrite,
	PreModifyCallback fnPreModify, PostModifyCallback fnPostModify)
{
	auto pObj = createObject<DictionaryProxy>();
	pObj->m_value = value;
	pObj->init(fnPreWrite, fnPreModify, fnPostModify);
	return pObj;
}

//
//
//
Size DictionaryProxy::getSize() const
{
	return m_value.getSize();
}

//
//
//
bool DictionaryProxy::isEmpty() const
{
	return m_value.isEmpty();
}

//
//
//
void DictionaryProxy::clear()
{
	if (callPreModify) callPreModify();
	m_value.clear();
	if (callPostModify) callPostModify();
}

//
//
//
std::unique_ptr<variant::IIterator> DictionaryProxy::createIterator(bool fUnsafe) const
{
	auto innerIter = Variant(m_value).createIterator(fUnsafe);
	return std::make_unique<Iterator>(std::move(innerIter), getPtrFromThis(this));
}

//
//
//
bool DictionaryProxy::has(const variant::DictionaryKeyRefType& sName) const
{
	return m_value.has(sName);
}

//
//
//
Size DictionaryProxy::erase(const variant::DictionaryKeyRefType& sName)
{
	if (callPreModify) callPreModify();
	if (callPostModify)
	{
		Size nSize = m_value.erase(sName);
		callPostModify();
		return nSize;
	}
	return m_value.erase(sName);
}

//
//
//
Variant DictionaryProxy::get(const variant::DictionaryKeyRefType& sName) const
{
	return m_value.get(sName);
}

//
//
//
std::optional<Variant> DictionaryProxy::getSafe(const variant::DictionaryKeyRefType& sName) const
{
	if (!m_value.has(sName))
		return {};
	return m_value.get(sName);
}

//
//
//
void DictionaryProxy::put(const variant::DictionaryKeyRefType& sName, const Variant& Value)
{
	if (callPreModify) callPreModify();
	m_value.put(sName, (callPreWrite ? callPreWrite(Value) : Value));
	if (callPostModify) callPostModify();
}

//
//
//
Variant DictionaryProxy::clone()
{
	return m_value.clone();
}

} // namespace variant
} // namespace cmd
