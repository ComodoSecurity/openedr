///
/// \file
/// \brief SequenceProxy declaration.
/// 
/// This file is a part of EDRAv2.libcore project.
///
/// Author: Yury Ermakov (29.12.2018)
/// Reviewer: Denis Bogdanov (10.01.2019)
///
/// /ingroup variant
/// @{
#pragma once

#include "variantproxybase.h"

namespace cmd {
namespace variant {

///
/// \Brief Wraps a sequence with callbacks support.
/// Implements ISequenceProxy and IClonable interfaces.
///
// TODO: ISerializable
class SequenceProxy : public VariantProxyBase, public ISequenceProxy, public IClonable
{
private:
	Sequence m_value;

protected:
	SequenceProxy() {}
	virtual ~SequenceProxy() {}

public:

	///
	/// \brief Creates a SequenceProxy object.
	/// \param value Value to be wrapped by proxy.
	/// \param fnPreWrite Pre-write callback function.
	/// \param fnPreModify Pre-modify callback function.
	/// \param fnPostModify Post-modify callback function.
	/// \return ObjPtr to SequenceProxy.
	///
	static ObjPtr<SequenceProxy> create(Sequence value, PreWriteCallback fnPreWrite,
		PreModifyCallback fnPreModify, PostModifyCallback fnPostModify);

	// ISequenceProxy

	/// \sa ISequenceProxy::getSize()
	Size getSize() const override;
	/// \sa ISequenceProxy::isEmpty()
	bool isEmpty() const override;
	/// \sa ISequenceProxy::erase()
	Size erase(Size nIndex) override;
	/// \sa ISequenceProxy::clear()
	void clear() override;
	/// \sa ISequenceProxy::get()
	Variant get(Size nIndex) const override;
	/// \sa ISequenceProxy::getSafe()
	std::optional<Variant> getSafe(Size nIndex) const override;
	/// \sa ISequenceProxy::put()
	void put(Size nIndex, const Variant& Value) override;
	/// \sa ISequenceProxy::push_back()
	void push_back(const Variant& vValue) override;
	/// \sa ISequenceProxy::setSize()
	void setSize(Size nSize) override;
	/// \sa ISequenceProxy::insert()
	void insert(Size nIndex, const Variant& vValue) override;
	/// \sa ISequenceProxy::createIterator()
	std::unique_ptr<variant::IIterator> createIterator(bool fUnsafe) const override;

	// IClonable

	/// \sa IClonable::clone()
	Variant clone() override;
};

} // namespace variant
} // namespace cmd

/// @}
