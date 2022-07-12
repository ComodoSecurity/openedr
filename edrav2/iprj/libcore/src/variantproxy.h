//
// EDRAv2.libcore
// 
// Author: Yury Ermakov (29.12.2018)
// Reviewer:
//
///
/// @file DictionaryProxy and SequenceProxy classes declaration
///
/// @addtogroup basicDataObjects Basic data objects
/// @{
#pragma once

namespace cmd {
namespace variant {

///
/// Dictionary iterator wrapper class.
///
class DictionaryIteratorProxy : public IIterator
{
private:
	std::unique_ptr<IIterator> m_pIter;
	CreateProxyCallback callCreateDictionaryProxy;
	CreateProxyCallback callCreateSequenceProxy;

public:
	///
	/// Constructor.
	///
	/// @param pIter - an iterator to wrap.
	/// @param fnCreateDictionaryProxy - the function to create
	///   the inner DictionaryProxy object.
	/// @param fnCreateSequenceProxy - the function to create
	///   the inner SequenceProxy object.
	///
	DictionaryIteratorProxy(std::unique_ptr<IIterator>&& pIter,
		CreateProxyCallback fnCreateDictionaryProxy, CreateProxyCallback fnCreateSequenceProxy) :
		m_pIter(std::move(pIter)),
		callCreateDictionaryProxy(fnCreateDictionaryProxy),
		callCreateSequenceProxy(fnCreateSequenceProxy)
	{}

	///
	/// Destructor.
	///
	virtual ~DictionaryIteratorProxy() {}

	// IIterator

	///
	/// @copydoc IIterator::goNext()
	///
	void goNext() override
	{
		m_pIter->goNext();
	}

	///
	/// @copydoc IIterator::isEnd()
	///
	bool isEnd() const override { return m_pIter->isEnd(); }

	///
	/// @copydoc IIterator::getKey()
	///
	DictionaryKeyRefType getKey() const override { return m_pIter->getKey(); }

	///
	/// @copydoc IIterator::getValue()
	///
	const Variant getValue() const override
	{
		Variant vValue = m_pIter->getValue();
		if (callCreateDictionaryProxy && vValue.isDictionaryLike())
			return callCreateDictionaryProxy(vValue);
		if (callCreateSequenceProxy && vValue.isSequenceLike())
			return callCreateSequenceProxy(vValue);
		return vValue;
	}
};

///
/// Sequence iterator wrapper class.
///
class SequenceIteratorProxy : public DictionaryIteratorProxy
{
public:
	///
	/// @copydoc DictionaryIteratorProxy::DictionaryIteratorProxy()
	///
	SequenceIteratorProxy(std::unique_ptr<IIterator>&& pIter,
		CreateProxyCallback fnCreateDictionaryProxy, CreateProxyCallback fnCreateSequenceProxy) :
		DictionaryIteratorProxy(std::move(pIter), fnCreateDictionaryProxy, fnCreateSequenceProxy) {}
	virtual ~SequenceIteratorProxy() {}

	///
	/// (Not supported)
	///
	/// @throws error::InvalidUsage Always.
	///
	DictionaryKeyRefType getKey() const override
	{
		error::InvalidUsage(SL, "Not supported").throwException();
	}
};

///
/// Dictionary proxy class.
///
/// Implements the callbacks support for operations with Dictionary object.
///
// TODO: ISerializable
class DictionaryProxy : public ObjectBase<>, public IDictionaryProxy, public IClonable, public IPrintable
{
private:
	Dictionary m_data;
	DictionaryPreModifyCallback callPreModify;
	DictionaryPostModifyCallback callPostModify;
	DictionaryPreClearCallback callPreClear;
	DictionaryPostClearCallback callPostClear;
	CreateProxyCallback callCreateDictionaryProxy;
	CreateProxyCallback callCreateSequenceProxy;

protected:
	///
	/// Constructor.
	///
	DictionaryProxy() {}

	///
	/// Destructor.
	///
	virtual ~DictionaryProxy() {}

public:

	///
	/// DictionaryProxy object creation
	///
	/// @param value - a value to be wrapped by proxy.
	/// @param fnPreModify - pre-modify callback function.
	/// @param fnPostModify - post-modify callback function.
	/// @param fnPreClear - pre-clear callback function.
	/// @param fnPostClear - post-clear callback function.
	/// @param fnCreateDictionaryProxy - callback function for DictionaryProxy creation.
	/// @param fnCreateSequenceProxy - callback function for SequenceProxy creation.
	/// @return The function returns a ObjPtr to DictionaryProxy.
	///
	static ObjPtr<DictionaryProxy> create(Dictionary value,
		DictionaryPreModifyCallback fnPreModify, DictionaryPostModifyCallback fnPostModify,
		DictionaryPreClearCallback fnPreClear, DictionaryPreClearCallback fnPostClear,
		CreateProxyCallback fnCreateDictionaryProxy, CreateProxyCallback fnCreateSequenceProxy);

	// IDictionaryProxy

	///
	/// @copydoc IDictionaryProxy::getSize()
	///
	Size getSize() const override;
	
	///
	/// @copydoc IDictionaryProxy::isEmpty()
	///
	bool isEmpty() const override;
	
	///
	/// @copydoc IDictionaryProxy::has(DictionaryKeyRefType)
	///
	bool has(DictionaryKeyRefType sName) const override;
	
	///
	/// @copydoc IDictionaryProxy::erase(DictionaryKeyRefType)
	///
	Size erase(DictionaryKeyRefType sName) override;
	
	///
	/// @copydoc IDictionaryProxy::clear()
	///
	void clear() override;
	
	///
	/// @copydoc IDictionaryProxy::get(DictionaryKeyRefType)
	///
	Variant get(DictionaryKeyRefType sName) const override;
	
	///
	/// @copydoc IDictionaryProxy::getSafe(DictionaryKeyRefType)
	///
	std::optional<Variant> getSafe(DictionaryKeyRefType sName) const override;
	
	///
	/// @copydoc IDictionaryProxy::put(DictionaryKeyRefType, const Variant&)
	///
	void put(DictionaryKeyRefType sName, const Variant& Value) override;
	
	///
	/// @copydoc IDictionaryProxy::createIterator(bool)
	///
	std::unique_ptr<IIterator> createIterator(bool fUnsafe) const override;

	// IClonable

	///
	/// @copydoc IClonable::clone()
	///
	Variant clone() override;

	///
	/// @copydoc IPrintable::print()
	///
	std::string print() const override;
};


///
/// Sequence proxy class
///
/// Implements the callbacks support for operations with Sequence object.
///
// TODO: ISerializable
class SequenceProxy : public ObjectBase<>, public ISequenceProxy, public IClonable, public IPrintable
{
private:
	Sequence m_data;
	SequencePreModifyCallback callPreModify;
	SequencePostModifyCallback callPostModify;
	SequencePreResizeCallback callPreResize;
	SequencePostResizeCallback callPostResize;
	CreateProxyCallback callCreateDictionaryProxy;
	CreateProxyCallback callCreateSequenceProxy;

protected:
	///
	/// Constructor
	///
	SequenceProxy() {}

	///
	/// Destructor
	///
	virtual ~SequenceProxy() {}

public:

	///
	/// SequenceProxy object creation
	///
	/// @param value Value to be wrapped by proxy.
	/// @param fnPreModify Pre-modify callback function.
	/// @param fnPostModify Post-modify callback function.
	/// @param fnPreResize Pre-resize callback function.
	/// @param fnPostResize Post-resize callback function.
	/// @param fnCreateDictionaryProxy Callback function for DictionaryProxy creation.
	/// @param fnCreateSequenceProxy Callback function for SequenceProxy creation.
	/// @return ObjPtr to SequenceProxy.
	///
	static ObjPtr<SequenceProxy> create(Sequence value,
		SequencePreModifyCallback fnPreModify, SequencePostModifyCallback fnPostModify,
		SequencePreResizeCallback fnPreResize, SequencePostResizeCallback fnPostResize,
		CreateProxyCallback fnCreateDictionaryProxy, CreateProxyCallback fnCreateSequenceProxy);

	// ISequenceProxy

	///
	/// @copydoc ISequenceProxy::getSize()
	///
	Size getSize() const override;
	
	///
	/// @copydoc ISequenceProxy::isEmpty()
	///
	bool isEmpty() const override;
	
	///
	/// @copydoc ISequenceProxy::erase(Size)
	///
	Size erase(Size nIndex) override;
	
	///
	/// @copydoc ISequenceProxy::clear()
	///
	void clear() override;
	
	///
	/// @copydoc ISequenceProxy::get(Size)
	///
	Variant get(Size nIndex) const override;
	
	///
	/// @copydoc ISequenceProxy::getSafe(Size)
	///
	std::optional<Variant> getSafe(Size nIndex) const override;
	
	///
	/// @copydoc ISequenceProxy::put(Size, const Variant&)
	///
	void put(Size nIndex, const Variant& Value) override;
	
	///
	/// @copydoc ISequenceProxy::push_back(const Variant&)
	///
	void push_back(const Variant& vValue) override;

	///
	/// @copydoc ISequenceProxy::setSize(Size)
	///
	void setSize(Size nSize) override;

	///
	/// @copydoc ISequenceProxy::insert(Size, const Variant&)
	///
	void insert(Size nIndex, const Variant& vValue) override;
	
	///
	/// @copydoc ISequenceProxy::createIterator(bool)
	///
	std::unique_ptr<variant::IIterator> createIterator(bool fUnsafe) const override;

	// IClonable

	///
	/// @copydoc IClonable::clone()
	///
	Variant clone() override;

	///
	/// @copydoc IPrintable::print()
	///
	std::string print() const override;
};

} // namespace variant
} // namespace cmd

/// @}
