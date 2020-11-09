///
/// @file
/// @brief DictionaryProxy declaration.
/// 
/// This file is a part of EDRAv2.libcore project.
///
/// Author: Yury Ermakov (29.12.2018)
/// Reviewer: Denis Bogdanov (10.01.2019)
///
/// @ingroup variant
/// @{
#pragma once

#include "variantproxybase.h"

namespace cmd {
namespace variant {

/// @brief Wraps a dictionary with callbacks support.
/// @details Implements IDictionaryProxy and IClonable interfaces.
// TODO: ISerializable
class DictionaryProxy : public VariantProxyBase, public IDictionaryProxy, public IClonable
{
private:
	Dictionary m_value;

protected:
	DictionaryProxy() {}
	virtual ~DictionaryProxy() {}

public:

	/// @brief Creates a DictionaryProxy object.
	/// @param value Value to be wrapped by proxy.
	/// @param fnPreWrite Pre-write callback function.
	/// @param fnPreModify Pre-modify callback function.
	/// @param fnPostModify Post-modify callback function.
	/// @return ObjPtr to DictionaryProxy.
	static ObjPtr<DictionaryProxy> create(Dictionary value, PreWriteCallback fnPreWrite,
		PreModifyCallback fnPreModify, PostModifyCallback fnPostModify);

	// IDictionaryProxy

	/// @copydoc IDictionaryProxy::getSize()
	Size getSize() const override;
	/// @copydoc IDictionaryProxy::isEmpty()
	bool isEmpty() const override;
	/// @copydoc IDictionaryProxy::has(const DictionaryKeyRefType&)
	bool has(const DictionaryKeyRefType& sName) const override;
	/// @copydoc IDictionaryProxy::erase(const DictionaryKeyRefType&)
	Size erase(const DictionaryKeyRefType& sName) override;
	/// @copydoc IDictionaryProxy::clear()
	void clear() override;
	/// @copydoc IDictionaryProxy::get(const DictionaryKeyRefType&)
	Variant get(const DictionaryKeyRefType& sName) const override;
	/// @copydoc IDictionaryProxy::getSafe()
	std::optional<Variant> getSafe(const DictionaryKeyRefType& sName) const override;
	/// @copydoc IDictionaryProxy::put(const DictionaryKeyRefType&, const Variant&)
	void put(const DictionaryKeyRefType& sName, const Variant& Value) override;
	/// @copydoc IDictionaryProxy::createIterator(bool)
	std::unique_ptr<IIterator> createIterator(bool fUnsafe) const override;

	// IClonable

	// @copydoc IClonable::clone()
	Variant clone() override;
};

} // namespace variant
} // namespace cmd

/// @}
