//
// SequenceProxy implementation.
// 
// This file is a part of EDRAv2.libcore project.
//
// Author: Yury Ermakov (29.12.2018)
// Reviewer: Denis Bogdanov (10.01.2019)
//
#include "pch.h"
#include "sequenceproxy.h"

namespace cmd {
namespace variant {

//
//
//
ObjPtr<SequenceProxy> SequenceProxy::create(Sequence value, PreWriteCallback fnPreWrite,
	PreModifyCallback fnPreModify, PostModifyCallback fnPostModify)
{
	auto pObj = createObject<SequenceProxy>();
	pObj->m_value = value;
	pObj->init(fnPreWrite, fnPreModify, fnPostModify);
	return pObj;
}

//
//
//
Size SequenceProxy::getSize() const
{
	return m_value.getSize();
}

//
//
//
bool SequenceProxy::isEmpty() const
{
	return m_value.isEmpty();
}

//
//
//
Size SequenceProxy::erase(Size nIndex)
{
	if (callPreModify) callPreModify();
	if (callPostModify)
	{
		Size nSize = m_value.erase(nIndex);
		callPostModify();
		return nSize;
	}
	return m_value.erase(nIndex);
}

//
//
//
void SequenceProxy::clear()
{
	if (callPreModify) callPreModify();
	m_value.clear();
	if (callPostModify) callPostModify();
}

//
//
//
Variant SequenceProxy::get(Size nIndex) const
{
	return m_value.get(nIndex);
}

//
//
//
std::optional<Variant> SequenceProxy::getSafe(Size nIndex) const
{
	if (nIndex >= m_value.getSize())
		return {};
	return m_value.get(nIndex);
}

//
//
//
void SequenceProxy::put(Size nIndex, const Variant& Value)
{
	if (callPreModify) callPreModify();
	m_value.put(nIndex, callPreWrite ? callPreWrite(Value) : Value);
	if (callPostModify) callPostModify();
}

//
//
//
void SequenceProxy::push_back(const Variant& vValue)
{
	if (callPreModify) callPreModify();
	m_value.push_back(callPreWrite ? callPreWrite(vValue) : vValue);
	if (callPostModify) callPostModify();
}

//
//
//
void SequenceProxy::setSize(Size nSize)
{
	if (callPreModify) callPreModify();
	m_value.setSize(nSize);
	if (callPostModify) callPostModify();
}

//
//
//
void SequenceProxy::insert(Size nIndex, const Variant& vValue)
{
	if (callPreModify) callPreModify();
	m_value.insert(nIndex, callPreWrite ? callPreWrite(vValue) : vValue);
	if (callPostModify) callPostModify();

}

//
//
//
std::unique_ptr<variant::IIterator> SequenceProxy::createIterator(bool fUnsafe) const
{
	auto innerIter = Variant(m_value).createIterator(fUnsafe);
	return std::make_unique<Iterator>(std::move(innerIter), getPtrFromThis(this));
}

//
//
//
Variant SequenceProxy::clone()
{
	return m_value.clone();
}

} // namespace variant
} // namespace cmd
