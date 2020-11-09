///
/// \file
/// \brief VariantProxyBase declaration.
/// 
/// This file is a part of EDRAv2.libcore project.
///
/// Author: Yury Ermakov (29.12.2018)
/// Reviewer:
///
/// /ingroup variant
/// @{
#pragma once

namespace cmd {
namespace variant {

///
/// @brief Wraps Dictionary with callbacks support.
///
// TODO: ISerializable
class DictionaryProxy : public ObjectBase<>, public IDictionaryProxy, public IClonable
{
private:
	Dictionary m_data;
	DictionaryPreModifyCallback callPreModify;
	DictionaryPostModifyCallback callPostModify;
	DictionaryPreClearCallback callPreClear;
	DictionaryPostClearCallback callPostClear;
	CreateProxyCallback callCreateDictionaryProxy;
	CreateProxyCallback callCreateSequenceProxy;

	//
	// Iterator wrapper
	//
	class Iterator : public IIterator
	{
	private:
		std::unique_ptr<IIterator> m_pIter;
		ObjPtr<DictionaryProxy> m_pProxy;

	public:
		Iterator(std::unique_ptr<IIterator>&& pIter, ObjPtr<DictionaryProxy> pProxy) :
			m_pIter(std::move(pIter)), m_pProxy{ pProxy } {}
		virtual ~Iterator() {}

		// IIterator
		void goNext() override { m_pIter->goNext(); }
		bool isEnd() const override { return m_pIter->isEnd(); }
		DictionaryKeyRefType getKey() const override { return m_pIter->getKey(); }
		const Variant& getValue() const override { return m_pIter->getValue(); }
	};

protected:
	DictionaryProxy() {}
	virtual ~DictionaryProxy() {}

public:

	/// @brief Creates a DictionaryProxy object.
	/// @param value Value to be wrapped by proxy.
	/// @param fnPreModify Pre-modify callback function.
	/// @param fnPostModify Post-modify callback function.
	/// @param fnPreClear Pre-clear callback function.
	/// @param fnPostClear Post-clear callback function.
	/// @param fnCreateDictionaryProxy Callback function for DictionaryProxy creation.
	/// @param fnCreateSequenceProxy Callback function for SequenceProxy creation.
	/// @return ObjPtr to DictionaryProxy.
	static ObjPtr<DictionaryProxy> create(Dictionary value,
		DictionaryPreModifyCallback fnPreModify, DictionaryPostModifyCallback fnPostModify,
		DictionaryPreClearCallback fnPreClear, DictionaryPreClearCallback fnPostClear,
		CreateProxyCallback fnCreateDictionaryProxy, CreateProxyCallback fnCreateSequenceProxy);

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

///
/// @brief Wraps Sequence with callbacks support.
///
// TODO: ISerializable
class SequenceProxy : public ObjectBase<>, public ISequenceProxy, public IClonable
{
private:
	Sequence m_data;
	SequencePreModifyCallback callPreModify;
	SequencePostModifyCallback callPostModify;
	SequencePreResizeCallback callPreResize;
	SequencePostResizeCallback callPostResize;
	CreateProxyCallback callCreateDictionaryProxy;
	CreateProxyCallback callCreateSequenceProxy;

	//
	// Iterator wrapper
	//
	class Iterator : public IIterator
	{
	private:
		std::unique_ptr<IIterator> m_pIter;
		ObjPtr<SequenceProxy> m_pProxy;

	public:
		Iterator(std::unique_ptr<IIterator>&& pIter, ObjPtr<SequenceProxy> pProxy) :
			m_pIter(std::move(pIter)), m_pProxy{ pProxy } {}
		virtual ~Iterator() {}

		// IIterator
		void goNext() override { m_pIter->goNext(); }
		bool isEnd() const override { return m_pIter->isEnd(); }
		DictionaryKeyRefType getKey() const override { return m_pIter->getKey(); }
		const Variant& getValue() const override { return m_pIter->getValue(); }
	};

protected:
	SequenceProxy() {}
	virtual ~SequenceProxy() {}

public:

	/// @brief Creates a SequenceProxy object.
	/// @param value Value to be wrapped by proxy.
	/// @param fnPreModify Pre-modify callback function.
	/// @param fnPostModify Post-modify callback function.
	/// @param fnPreResize Pre-resize callback function.
	/// @param fnPostResize Post-resize callback function.
	/// @param fnCreateDictionaryProxy Callback function for DictionaryProxy creation.
	/// @param fnCreateSequenceProxy Callback function for SequenceProxy creation.
	/// @return ObjPtr to SequenceProxy.
	static ObjPtr<SequenceProxy> create(Sequence value,
		SequencePreModifyCallback fnPreModify, SequencePostModifyCallback fnPostModify,
		SequencePreResizeCallback fnPreResize, SequencePostResizeCallback fnPostResize,
		CreateProxyCallback fnCreateDictionaryProxy, CreateProxyCallback fnCreateSequenceProxy);

	// ISequenceProxy

	/// @copydoc ISequenceProxy::getSize()
	Size getSize() const override;
	/// @copydoc ISequenceProxy::isEmpty()
	bool isEmpty() const override;
	/// @copydoc ISequenceProxy::erase(Size)
	Size erase(Size nIndex) override;
	/// @copydoc ISequenceProxy::clear()
	void clear() override;
	/// @copydoc ISequenceProxy::get(Size)
	Variant get(Size nIndex) const override;
	/// @copydoc ISequenceProxy::getSafe(Size)
	std::optional<Variant> getSafe(Size nIndex) const override;
	/// @copydoc ISequenceProxy::put(Size, const Variant&)
	void put(Size nIndex, const Variant& Value) override;
	/// @copydoc ISequenceProxy::push_back(const Variant&)
	void push_back(const Variant& vValue) override;
	/// @copydoc ISequenceProxy::setSize(Size)
	void setSize(Size nSize) override;
	/// @copydoc ISequenceProxy::insert(Size, const Variant&)
	void insert(Size nIndex, const Variant& vValue) override;
	/// @copydoc ISequenceProxy::createIterator(bool)
	std::unique_ptr<variant::IIterator> createIterator(bool fUnsafe) const override;

	// IClonable

	/// @copydoc IClonable::clone()
	Variant clone() override;
};

} // namespace variant
} // namespace cmd

/// @}
