//
// edrav2.libcore project
//
// Author: Yury Ermakov (23.02.2019)
// Reviewer: Denis Bogdanov (27.02.2019)
//

///
/// @file DictionaryMixer object declaration
///

///
/// @addtogroup basicDataObjects Basic data objects
/// @{

#pragma once
#include <objects.h>

namespace openEdr {
namespace variant {

//
// Interface to access to a dictionary via the proxy
// using a relative path
//
class IRootDictionaryProxy : OBJECT_INTERFACE
{
public:

	//
	// Get number of elements
	//
	virtual Size getSize(std::string_view sPath) const = 0;

	//
	// Check if dictionary is empty
	//
	virtual bool isEmpty(std::string_view sPath) const = 0;

	//
	// Check existence element with specified name
	//
	virtual bool has(std::string_view sPath, variant::DictionaryKeyRefType sName) const = 0;

	//
	// Remove element with specified name
	// Return number of removed elements
	//
	virtual Size erase(std::string_view sPath, variant::DictionaryKeyRefType sName) = 0;

	//
	// Remove all elements
	//
	virtual void clear(std::string_view sPath) = 0;

	//
	// Returns element with specified name
	// If element does not exist or error occurred, 
	// the method throws appropriate error
	//
	virtual Variant get(std::string_view sPath, variant::DictionaryKeyRefType sName) const = 0;

	//
	// Returns element with specified name
	// If element does not exist or error occurred, 
	// return empty element
	//
	virtual std::optional<Variant> getSafe(std::string_view sPath, variant::DictionaryKeyRefType sName) const = 0;

	//
	// Add element into container
	//
	virtual void put(std::string_view sPath, variant::DictionaryKeyRefType sName, const Variant& vValue) = 0;

	//
	// Return dictionary iterator
	// if fUnsafe == false, IIterator must be thread safe
	//
	virtual std::unique_ptr<variant::IIterator> createIterator(std::string_view sPath, bool fUnsafe) const = 0;
};

///
/// Object to aggregate several dictionary objects
/// into a one meta-dictionary.
///
// TODO: IClonable (?)
// TODO: IPrintable (?)
// TODO: ISerializable
class DictionaryMixer : public ObjectBase<CLSID_DictionaryMixer>,
	public IDictionaryProxy, public IRootDictionaryProxy, public IPrintable
{
private:
	std::vector<Variant> m_vecLayers;
	std::unordered_set<std::string> m_erasedKeys;
	mutable std::mutex m_mtxLayers;

	bool isErased(std::string_view sName) const;
	Variant createDictionaryProxy(std::string_view sPath) const;
	std::pair<std::string_view, std::string_view> splitDictPath(std::string_view sPath) const;

public:
	///
	/// Object's final construction.
	///
	/// The first dictionary in the sequence of layers is read-write but other
	/// is read-only.
	///
	/// @param vConfig the object's configuration including the following fields:
	///   **layers** - a sequence of dictionaries with data to be mixed
	/// @throw error::InvalidArgument if configuration is not valid.
	///
	void finalConstruct(Variant vConfig);

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
	/// @copydoc IDictionaryProxy::has(variant::DictionaryKeyRefType)
	///
	bool has(variant::DictionaryKeyRefType sName) const override;
	
	///
	/// @copydoc IDictionaryProxy::erase(variant::DictionaryKeyRefType)
	///
	Size erase(variant::DictionaryKeyRefType sName) override;

	///
	/// @copydoc IDictionaryProxy::clear()
	///
	void clear() override;

	///
	/// @copydoc IDictionaryProxy::get(variant::DictionaryKeyRefType)
	///
	Variant get(variant::DictionaryKeyRefType sName) const override;

	///
	/// @copydoc IDictionaryProxy::getSafe(variant::DictionaryKeyRefType)
	///
	std::optional<Variant> getSafe(variant::DictionaryKeyRefType sName) const override;

	///
	/// @copydoc IDictionaryProxy::put(variant::DictionaryKeyRefType, const Variant&)
	///
	void put(variant::DictionaryKeyRefType sName, const Variant& Value) override;

	///
	/// @copydoc IDictionaryProxy::createIterator(bool)
	///
	std::unique_ptr<variant::IIterator> createIterator(bool fUnsafe) const override;

	// IRootDictionaryProxy

	///
	/// @copydoc IRootDictionaryProxy::getSize(std::string_view)
	///
	Size getSize(std::string_view sPath) const override;
	
	///
	/// @copydoc IRootDictionaryProxy::isEmpty(std::string_view)
	///
	bool isEmpty(std::string_view sPath) const override;
	
	///
	/// @copydoc IRootDictionaryProxy::has(std::string_view,const variant::DictionaryKeyRefType&)
	///
	bool has(std::string_view sPath, 
		variant::DictionaryKeyRefType sName) const override;
	
	///
	/// @copydoc IRootDictionaryProxy::erase(std::string_view,variant::DictionaryKeyRefType)
	///
	Size erase(std::string_view sPath, variant::DictionaryKeyRefType sName) override;

	///
	/// @copydoc IRootDictionaryProxy::clear(std::string_view)
	///
	void clear(std::string_view sPath) override;

	///
	/// @copydoc IRootDictionaryProxy::get(std::string_view,variant::DictionaryKeyRefType)
	///
	Variant get(std::string_view sPath, variant::DictionaryKeyRefType sName) const override;

	///
	/// @copydoc IRootDictionaryProxy::getSafe(std::string_view,variant::DictionaryKeyRefType)
	///
	std::optional<Variant> getSafe(std::string_view sPath, variant::DictionaryKeyRefType sName) const override;

	///
	/// @copydoc IRootDictionaryProxy::put(std::string_view,variant::DictionaryKeyRefType,const Variant&)
	///
	void put(std::string_view sPath, variant::DictionaryKeyRefType sName, const Variant& vValue) override;

	///
	/// @copydoc IRootDictionaryProxy::createIterator(std::string_view,bool)
	///
	std::unique_ptr<variant::IIterator> createIterator(std::string_view sPath, bool fUnsafe) const override;

	// IPrintable

	///
	/// @copydoc IPrintable::print()
	///
	std::string print() const override;
};

} // namespace variant
} // namespace openEdr
/// @}
