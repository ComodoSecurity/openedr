//
// edrav2.libcore project
//
// Author: Yury Ermakov (23.02.2019)
// Reviewer: Denis Bogdanov (27.02.2019)
//

///
/// @file DictionaryMixer object implementation
///

///
/// @addtogroup basicDataObjects Basic data objects
/// @{

#include "pch.h"
#include "dictionarymixer.h"

namespace openEdr {
namespace variant {

typedef std::string DictionaryKeyType;

//
//
//
class LayeredDictIterator : public IIterator
{
private:

	struct Item
	{
		DictionaryKeyType index;
		Variant value;

		Item(const DictionaryKeyType& index, Variant value) :
			index(index), value(value) {}
	};

	using Items = std::vector<Item>;
	Items m_items;
	size_t m_nIndex = 0;

	void checkOutOfRange() const
	{
		if (m_nIndex >= m_items.size())
			error::OutOfRange(SL, "Access to finished iterator").throwException();
	}

	// Only DictionaryMixer can create LayeredDictIterator
	friend DictionaryMixer;

	// Deny coping and moving
	LayeredDictIterator(const LayeredDictIterator&) = delete;
	LayeredDictIterator(LayeredDictIterator&&) = delete;

	// Private parameter of constructor
	struct PrivateKey {};

public:

	// "private" constructor supporting std::make_shared 
	// Create may use std::make_unique, but constructor can't be called external
	LayeredDictIterator(const PrivateKey&) {}

	// IIterator

	//
	//
	//
	DictionaryKeyRefType getKey() const override
	{
		checkOutOfRange();
		return m_items[m_nIndex].index;
	}

	//
	//
	//
	const Variant getValue() const override
	{
		checkOutOfRange();
		return m_items[m_nIndex].value;
	}

	//
	//
	//
	void goNext() override
	{
		checkOutOfRange();
		m_nIndex++;
	}

	//
	//
	//
	bool isEnd() const override
	{
		return m_nIndex >= m_items.size();
	}
};

//////////////////////////////////////////////////////////////////////////////
//
// LayeredDictProxy
//
//////////////////////////////////////////////////////////////////////////////

///
/// Layered dictionary proxy class.
///
// TODO: IClonable (?)
// TODO: IPrintable (?)
// TODO: ISerializable
class LayeredDictProxy : public ObjectBase<>, public IDictionaryProxy
{
private:
	ObjPtr<IRootDictionaryProxy> m_pRoot;
	std::string m_sPath;

protected:
	///
	/// Constructor
	///
	LayeredDictProxy() {}

	///
	/// Destructor
	///
	virtual ~LayeredDictProxy() {}

public:

	///
	/// LayeredDictProxy object creation
	///
	/// @param pRoot ObjPtr to the root object
	/// @param sPath a relative path inside the root object
	/// @return Returns ObjPtr to created object.
	///
	static ObjPtr<LayeredDictProxy> create(ObjPtr<IRootDictionaryProxy> pRoot, std::string_view sPath)
	{
		auto pObj = createObject<LayeredDictProxy>();
		pObj->m_pRoot = pRoot;
		pObj->m_sPath = sPath;
		return pObj;
	}

	// IDictionaryProxy

	///
	/// @copydoc IDictionaryProxy::getSize()
	///
	Size getSize() const override
	{
		return m_pRoot->getSize(m_sPath);
	}

	///
	/// @copydoc IDictionaryProxy::isEmpty()
	///
	bool isEmpty() const override
	{
		return m_pRoot->isEmpty(m_sPath);
	}

	///
	/// @copydoc IDictionaryProxy::has(DictionaryKeyRefType)
	///
	bool has(DictionaryKeyRefType sName) const override
	{
		return m_pRoot->has(m_sPath, sName);
	}

	///
	/// @copydoc IDictionaryProxy::erase(DictionaryKeyRefType)
	///
	Size erase(DictionaryKeyRefType sName) override
	{
		return m_pRoot->erase(m_sPath, sName);
	}

	///
	/// @copydoc IDictionaryProxy::clear()
	///
	void clear() override
	{
		m_pRoot->clear(m_sPath);
	}

	///
	/// @copydoc IDictionaryProxy::get(DictionaryKeyRefType)
	///
	Variant get(DictionaryKeyRefType sName) const override
	{
		return m_pRoot->get(m_sPath, sName);
	}

	///
	/// @copydoc IDictionaryProxy::getSafe(DictionaryKeyRefType)
	///
	std::optional<Variant> getSafe(DictionaryKeyRefType sName) const override
	{
		return m_pRoot->getSafe(m_sPath, sName);
	}

	///
	/// @copydoc IDictionaryProxy::put(DictionaryKeyRefType, const Variant&)
	///
	void put(DictionaryKeyRefType sName, const Variant& Value) override
	{
		m_pRoot->put(m_sPath, sName, Value);
	}

	///
	/// @copydoc IDictionaryProxy::createIterator(bool)
	///
	std::unique_ptr<IIterator> createIterator(bool fUnsafe) const override
	{
		return m_pRoot->createIterator(m_sPath, fUnsafe);
	}
};

//////////////////////////////////////////////////////////////////////////////
//
// DictionaryMixer
//
//////////////////////////////////////////////////////////////////////////////

//
//
//
void DictionaryMixer::finalConstruct(Variant vConfig)
{
	if (!vConfig.has("layers"))
		error::InvalidArgument(SL, "Missing field <layers>").throwException();
	if (!vConfig["layers"].isSequenceLike())
		error::InvalidArgument(SL, "Layers must be a sequence").throwException();
	if (vConfig["layers"].isEmpty())
		error::InvalidArgument(SL, "No layers found").throwException();
	TRACE_BEGIN;
	for (const auto& item : vConfig["layers"])
	{
		if (!item.isDictionaryLike())
			error::InvalidArgument(SL, "Layer must be a dictionary").throwException();
		m_vecLayers.emplace_back(item);
	}
	TRACE_END("Can't construct the object: " + variant::serializeToJson(vConfig));
}

//
//
//
Size DictionaryMixer::getSize() const
{
	std::scoped_lock _lock(m_mtxLayers);
	std::unordered_set<std::string> aliveKeys;
	for (const Dictionary& dictLayer : m_vecLayers)
		for (const auto& [sKey, vValue] : dictLayer)
			if (!isErased(sKey) && (aliveKeys.find(std::string(sKey)) == aliveKeys.end()))
				aliveKeys.emplace(sKey);
	return aliveKeys.size();
}

//
//
//
bool DictionaryMixer::isEmpty() const
{
	std::scoped_lock _lock(m_mtxLayers);
	for (const Dictionary& dictLayer : m_vecLayers)
		for (const auto& [sKey, vValue] : dictLayer)
			if (!isErased(sKey))
				return false;
	return true;
}

//
//
//
bool DictionaryMixer::has(variant::DictionaryKeyRefType sName) const
{
	std::scoped_lock _lock(m_mtxLayers);
	if (isErased(sName))
		return false;
	for (const Dictionary& dictLayer : m_vecLayers)
		if (dictLayer.has(sName))
			return true;
	return false;
}

//
//
//
Size DictionaryMixer::erase(variant::DictionaryKeyRefType sName)
{
	TRACE_BEGIN;
	std::scoped_lock _lock(m_mtxLayers);
	if (isErased(sName))
		return 0;
	auto& firstLayer = m_vecLayers[0];
	Size nLayer = m_vecLayers.size() - 1;
	for (auto it = m_vecLayers.crbegin(); (nLayer > 0) && (it != m_vecLayers.crend()); ++it, --nLayer)
		if (it->has(sName))
		{
			m_erasedKeys.emplace(sName);
			firstLayer.erase(sName);
			return 1;
		}
	return firstLayer.erase(sName);
	TRACE_END(FMT("Can't erase the item <" << std::string(sName) << ">"));
}

//
//
//
void DictionaryMixer::clear()
{
	TRACE_BEGIN;
	std::scoped_lock _lock(m_mtxLayers);
	auto& firstLayer = m_vecLayers[0];
	Size nLayer = m_vecLayers.size() - 1;
	for (auto it = m_vecLayers.crbegin(); (nLayer > 0) && (it != m_vecLayers.crend()); ++it, --nLayer)
		for (const auto& [sKey, vValue] : Dictionary(*it))
			if (!isErased(sKey))
			{
				m_erasedKeys.emplace(sKey);
				firstLayer.erase(sKey);
			}
	firstLayer.clear();
	TRACE_END("Can't clear data");
}

//
//
//
Variant DictionaryMixer::get(variant::DictionaryKeyRefType sName) const
{
	std::scoped_lock _lock(m_mtxLayers);
	if (!isErased(sName))
		for (const Dictionary& dictLayer : m_vecLayers)
			if (dictLayer.has(sName))
			{
				Variant vValue = dictLayer.get(sName);
				return (vValue.isDictionaryLike() ? createDictionaryProxy(sName) : vValue);
			}
	error::OutOfRange(SL, FMT("Invalid dictionary index <" << sName << ">")).throwException();
}

//
//
//
std::optional<Variant> DictionaryMixer::getSafe(variant::DictionaryKeyRefType sName) const
{
	if (!has(sName))
		return {};
	return get(sName);
}

//
//
//
void DictionaryMixer::put(variant::DictionaryKeyRefType sName, const Variant& Value)
{
	TRACE_BEGIN;
	std::scoped_lock _lock(m_mtxLayers);
	// Unerase only the root key, already erased inner keys doesn't spoil
	m_erasedKeys.erase(std::string(sName));
	m_vecLayers[0].put(sName, Value);
	TRACE_END(FMT("Can't put the item <" << sName << ">"));
}

//
//
//
std::unique_ptr<variant::IIterator> DictionaryMixer::createIterator(bool /*fUnsafe*/) const
{
	// Unsafe iterators are not supported
	auto pIter = std::make_unique<LayeredDictIterator>(LayeredDictIterator::PrivateKey());
	pIter->m_items.reserve(getSize());

	std::scoped_lock _lock(m_mtxLayers);
	std::unordered_set<std::string> keptKeys;
	for (const Dictionary& dictLayer : m_vecLayers)
		for (const auto& [sKey, vValue] : dictLayer)
			if (!isErased(sKey) && (keptKeys.find(std::string(sKey)) == keptKeys.end()))
			{
				pIter->m_items.emplace_back(DictionaryKeyType(sKey),
					vValue.isDictionaryLike() ? createDictionaryProxy(sKey) : vValue);
				keptKeys.emplace(sKey);
			}
	return pIter;
}

//
//
//
Size DictionaryMixer::getSize(std::string_view sPath) const
{
	if (sPath.empty())
		return getSize();

	std::scoped_lock _lock(m_mtxLayers);
	std::unordered_set<std::string> aliveKeys;
	for (const Dictionary& dictLayer : m_vecLayers)
	{
		auto vNode = variant::getByPath(dictLayer, sPath, Variant());
		if (vNode.isNull() || !vNode.isDictionaryLike())
			continue;
		for (const auto& [sKey, vValue] : Dictionary(vNode))
		{
			std::string sFullPath = std::string(sPath) + "." + std::string(sKey);
			if (!isErased(sFullPath) && (aliveKeys.find(std::string(sKey)) == aliveKeys.end()))
				aliveKeys.emplace(sKey);
		}
	}
	return aliveKeys.size();
}

//
//
//
bool DictionaryMixer::isEmpty(std::string_view sPath) const
{
	if (sPath.empty())
		return isEmpty();

	std::scoped_lock _lock(m_mtxLayers);
	for (const Dictionary& dictLayer : m_vecLayers)
	{
		auto vNode = variant::getByPath(dictLayer, sPath, Variant());
		if (!vNode.isDictionaryLike())
			break;
		if (!vNode.isNull() && !vNode.isEmpty())
		{
			for (const auto& [sKey, vValue] : Dictionary(vNode))
			{
				std::string sFullPath = std::string(sPath) + "." + std::string(sKey);
				if (!isErased(sFullPath))
					return false;
			}
		}
	}
	return true;
}

//
//
//
bool DictionaryMixer::has(std::string_view sPath, variant::DictionaryKeyRefType sName) const
{
	if (sPath.empty())
		return has(sName);

	std::scoped_lock _lock(m_mtxLayers);
	std::string sFullPath = std::string(sPath) + "." + std::string(sName);
	if (isErased(sFullPath))
		return false;
	for (const Dictionary& dictLayer : m_vecLayers)
	{
		auto vNode = variant::getByPath(dictLayer, sPath, Variant());
		if (!vNode.isNull() && vNode.has(sName))
			return true;
	}
	return false;
}

//
//
//
Size DictionaryMixer::erase(std::string_view sPath, variant::DictionaryKeyRefType sName)
{
	if (sPath.empty())
		return erase(sName);

	TRACE_BEGIN;
	std::scoped_lock _lock(m_mtxLayers);
	std::string sFullPath = std::string(sPath) + "." + std::string(sName);
	if (isErased(sFullPath))
		return 0;

	bool fFirstLayer = true;
	for (Dictionary dictLayer : m_vecLayers)
	{
		auto vNode = variant::getByPath(dictLayer, sPath, Variant());
		if (!vNode.isNull() && vNode.has(sName))
		{
			m_erasedKeys.emplace(sFullPath);
			return ((fFirstLayer) ? vNode.erase(sName) : 1);
		}
		fFirstLayer = false;
	}

	return 0;
	TRACE_END(FMT("Can't erase the item <" << sPath << "." << sName << ">"));
}

//
//
//
void DictionaryMixer::clear(std::string_view sPath)
{
	if (sPath.empty())
	{
		clear();
		return;
	}

	TRACE_BEGIN;
	std::scoped_lock _lock(m_mtxLayers);
	if (isErased(sPath))
		return;
	bool fFirstLayer = true;
	for (Dictionary dictLayer : m_vecLayers)
	{
		auto vNode = variant::getByPath(dictLayer, sPath, Variant());
		if (vNode.isNull() || !vNode.isDictionaryLike())
			continue;
		for (const auto& [sKey, vValue] : Dictionary(vNode))
		{
			std::string sFullPath = std::string(sPath) + "." + std::string(sKey);
			m_erasedKeys.emplace(sFullPath);
		}
		if (fFirstLayer)
			vNode.clear();
		fFirstLayer = false;
	}
	TRACE_END(FMT("Can't clear data in <" << sPath << ">"));
}

//
//
//
Variant DictionaryMixer::get(std::string_view sPath, variant::DictionaryKeyRefType sName) const
{
	if (sPath.empty())
		return get(sName);

	std::scoped_lock _lock(m_mtxLayers);
	std::string sFullPath = std::string(sPath) + "." + std::string(sName);
	if (!isErased(sFullPath))
		for (const Dictionary& dictLayer : m_vecLayers)
		{
			auto vNode = variant::getByPath(dictLayer, sPath, Variant());
			if (!vNode.isNull() && vNode.has(sName))
				return vNode.get(sName);
		}
	error::OutOfRange(SL, FMT("Invalid dictionary index <" << sFullPath << ">")).throwException();
}

//
//
//
std::optional<Variant> DictionaryMixer::getSafe(std::string_view sPath,
	variant::DictionaryKeyRefType sName) const
{
	if (!has(sPath, sName))
		return {};
	return get(sPath, sName);
}

//
//
//
void DictionaryMixer::put(std::string_view sPath, variant::DictionaryKeyRefType sName, const Variant& vValue)
{
	if (sPath.empty())
	{
		put(sName, vValue);
		return;
	}

	TRACE_BEGIN;
	std::scoped_lock _lock(m_mtxLayers);
	std::string sFullPath = std::string(sPath) + "." + std::string(sName);
	variant::putByPath(m_vecLayers[0], sFullPath, vValue, /* fCreatePaths */ true);

	// Restore erased keys
	m_erasedKeys.erase(sFullPath);
	auto path = splitDictPath(sPath);
	do
	{
		m_erasedKeys.erase(std::string(path.second));
		path = splitDictPath(path.second);
	} while (!path.second.empty());

	TRACE_END(FMT("Can't put the item <" << sPath << "." << sName << ">"));
}

//
//
//
std::unique_ptr<variant::IIterator> DictionaryMixer::createIterator(std::string_view sPath,
	bool fUnsafe) const
{
	if (sPath.empty())
		return createIterator(fUnsafe);

	// Unsafe iterators are not supported
	auto pIter = std::make_unique<LayeredDictIterator>(LayeredDictIterator::PrivateKey());
	pIter->m_items.reserve(getSize(sPath));

	std::scoped_lock _lock(m_mtxLayers);
	std::unordered_set<std::string> keptKeys;
	for (const Dictionary& dictLayer : m_vecLayers)
	{
		auto vNode = variant::getByPath(dictLayer, sPath, Variant());
		if (vNode.isNull() || !vNode.isDictionaryLike())
			continue;
		for (const auto& [sKey, vValue] : Dictionary(vNode))
		{
			std::string sFullPath = std::string(sPath) + "." + std::string(sKey);
			if (!isErased(sFullPath) && (keptKeys.find(std::string(sKey)) == keptKeys.end()))
			{
				pIter->m_items.emplace_back(DictionaryKeyType(sKey),
					vValue.isDictionaryLike() ? createDictionaryProxy(sFullPath) : vValue);
				keptKeys.emplace(sKey);
			}
		}
	}
	return pIter;
}

//
//
//
std::string DictionaryMixer::print() const
{
	std::scoped_lock _lock(m_mtxLayers);
	Variant vResult = Dictionary();
	for (auto it = m_vecLayers.crbegin(); it != m_vecLayers.crend(); ++it)
		vResult = vResult.merge(*it, MergeMode::All);
	for (auto sPath : m_erasedKeys)
		variant::deleteByPath(vResult, sPath);
	return vResult.print();
}

//
//
//
std::pair<std::string_view, std::string_view> DictionaryMixer::splitDictPath(std::string_view sPath) const
{
	std::string_view sKey;
	std::string_view sTail;

	auto nEndOfKey = sPath.find_first_of(".");
	if (nEndOfKey == std::string_view::npos)
		sKey = sPath;
	else
	{
		sKey = sPath.substr(0, nEndOfKey);
		sTail = sPath.substr(sPath[nEndOfKey] == '.' ? nEndOfKey + 1 : nEndOfKey);
	}
	return std::make_pair(sKey, sTail);
}

//
//
//
bool DictionaryMixer::isErased(std::string_view sName) const
{
	return (m_erasedKeys.find(std::string(sName)) != m_erasedKeys.end());
}

//
//
//
Variant DictionaryMixer::createDictionaryProxy(std::string_view sPath) const
{
	auto pThis = getPtrFromThis(this);
	return LayeredDictProxy::create(pThis, sPath);
}

} // namespace variant
} // namespace openEdr 
/// @}
