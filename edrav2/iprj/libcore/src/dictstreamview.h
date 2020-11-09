//
// EDRAv2.libcore project.
//
// Author: Yury Ermakov (29.12.2018)
// Reviewer:
//
///
/// @file DictionaryStreamView class declaration
///
///
/// @addtogroup io IO subsystem
/// @{
#pragma once
#include <io.hpp>
#include <objects.h>

namespace openEdr {
namespace io {

///
/// Object stores a dictionary and saves it to a file on disk
/// in background.
///
class DictionaryStreamView : public ObjectBase<CLSID_DictionaryStreamView>,
	public IDictionaryStreamView,
	public variant::IDictionaryProxy,
	public IPrintable,
	public variant::IClonable
{
private:
	Dictionary m_data;
	bool m_fReadOnly=false;
	ObjPtr<io::IWritableStream>	m_pWriteStream;
	std::optional<Variant> m_optDefault;

	Variant createDictionaryProxy(Variant vValue) const;
	Variant createSequenceProxy(Variant vValue) const;
	void flush();

public:
	///
	/// Object final construction.
	///
	/// @param vConfig The object's configuration including the following fields:
	///   **stream** - stream object.
	///   **readOnly** - (Optional) read-only mode flag (default: false).
	/// represents a name of the file to read and write object's data.
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

	// IDictionaryStreamView

	/// @copydoc IDictionaryStreamView::reload()
	void reload() override;

	// IDictionaryProxy

	/// @copydoc IDictionaryProxy::getSize()
	Size getSize() const override;
	/// @copydoc IDictionaryProxy::isEmpty()
	bool isEmpty() const override;
	/// @copydoc IDictionaryProxy::has(variant::DictionaryKeyRefType)
	bool has(variant::DictionaryKeyRefType sName) const override;
	/// @copydoc IDictionaryProxy::erase(variant::DictionaryKeyRefType)
	Size erase(variant::DictionaryKeyRefType sName) override;
	/// @copydoc IDictionaryProxy::clear()
	void clear() override;
	/// @copydoc IDictionaryProxy::get(variant::DictionaryKeyRefType)
	Variant get(variant::DictionaryKeyRefType sName) const override;
	/// @copydoc IDictionaryProxy::getSafe(variant::DictionaryKeyRefType)
	std::optional<Variant> getSafe(variant::DictionaryKeyRefType sName) const override;
	/// @copydoc IDictionaryProxy::put(variant::DictionaryKeyRefType, const Variant&)
	void put(variant::DictionaryKeyRefType sName, const Variant& Value) override;
	/// @copydoc IDictionaryProxy::createIterator(bool)
	std::unique_ptr<variant::IIterator> createIterator(bool fUnsafe) const override;

	// IPrintable

	/// @copydoc IPrintable::print()
	std::string print() const override;

	// IClonable

	/// @copydoc IClonable::clone()
	Variant clone() override;
};

} // namespace io
} // namespace openEdr

/// @}
