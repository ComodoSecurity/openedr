//
// EDRAv2.libcore project
//
// Internal dictionary container
//
// Autor: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (11.12.2018)
//
#pragma once

#include "value.hpp"
#include "dictkey.hpp"

namespace openEdr {
namespace variant {
namespace detail {

//
// Element for storing and iterating dictionary
//
struct DictElem
{
	DictKey key;
	Variant val;

	DictElem() = default;
	DictElem(const DictElem&) = default;
	DictElem(DictElem&&) noexcept = default;
	DictElem& operator=(const DictElem&) noexcept = default;
	DictElem& operator=(DictElem&&) noexcept = default;

	DictElem(DictKey _key, const Variant& _val) : key(_key), val(_val) {}
	DictElem(DictKey _key, Variant&& _val) : key(_key), val(std::move(_val)) {}
};

//////////////////////////////////////////////////////////////////////////
//
// BasicDictionary storages
//

//
// Base class for applyForEach
//
class IDictStorageVisitor
{
public:
	virtual void visit(DictKey key, Variant& val) = 0;
};

//
// Interface of BasicDictionary storage
// Storage should not use sync
//
class IDictStorage
{
public:
	//
	// Obligatory virtual destructor
	//
	virtual ~IDictStorage() {}

	//
	// Returns size
	//
    virtual Size getSize() const = 0;

	//
	// Returns is empty
	//
	virtual bool isEmpty() const = 0;

	//
	// Checks key existence
	//
	virtual bool has(DictKey key) const = 0;

	//
	// Erases key and returns erased elements count
	//
	virtual Size erase(DictKey key) = 0;

	//
	// Clears
	//
	virtual void clear() = 0;

	//
	// Returns value by key or std::nullopt
	// 
	virtual std::optional<Variant> getSafe(DictKey key) const = 0;

	//
	// Adds a value
	// Returns true if success, false if storage is full
	// 
	virtual bool put(DictKey key, const Variant& value) = 0;

	//
	// Move a value
	// Returns true if success, false if storage is full
	// 
	virtual bool put(DictKey key, Variant&& value) = 0;

	//
	// Process each element
	// It used for storage transformation and clone and iteration
	// 
	virtual void applyForEach(IDictStorageVisitor* pVisitor) = 0;
};

//
//
//
template<typename FnVisit>
void applyForEach(std::unique_ptr<IDictStorage>& pStorage, FnVisit fnVisit)
{
	struct DictStorageVisitor : public IDictStorageVisitor
	{
		FnVisit& fn;
		DictStorageVisitor(FnVisit& _fn) : fn(_fn) {}

		void visit(DictKey key, Variant& val) override
		{
			fn(key, val);
		}
	};

	if (pStorage == nullptr)
		return;

	DictStorageVisitor visitor(fnVisit);
	pStorage->applyForEach(&visitor);
}


//
// std::unordered_map storage
//
class UnorderedMapDictStorage : public IDictStorage
{
private:
	typedef std::unordered_map<DictKey, Variant> TMap;
	TMap m_map;

public:
	UnorderedMapDictStorage() = default;
	~UnorderedMapDictStorage() {}

	// Deny copy and move
	UnorderedMapDictStorage(const UnorderedMapDictStorage&) = delete;
	UnorderedMapDictStorage(UnorderedMapDictStorage&&) = delete;
	UnorderedMapDictStorage& operator=(const UnorderedMapDictStorage&) = delete;
	UnorderedMapDictStorage& operator=(UnorderedMapDictStorage&&) = delete;

	//////////////////////////////////////////////////////////////////////////
	//
	// IDictStorage
	//

	Size getSize() const override
	{
		return m_map.size();
	}

	bool isEmpty() const override
	{
		return m_map.empty();
	}

	bool has(DictKey key) const override
	{
		return (m_map.find(key) != m_map.end());
	}

	Size erase(DictKey key) override
	{
		return m_map.erase(key);
	}

	void clear() override
	{
		m_map.clear();
	}

	std::optional<Variant> getSafe(DictKey key) const override
	{
		TMap::const_iterator iter = m_map.find(key);
		if (iter == m_map.end()) 
			return std::nullopt;
		return iter->second;
	}

	bool put(DictKey key, const Variant& value) override
	{
		m_map[key] = value;
		return true;
	}

	bool put(DictKey key, Variant&& value) override
	{
		m_map[key] = std::move(value);
		return true;
	}

	void applyForEach(IDictStorageVisitor* pVisitor) override
	{
		for (auto& kv : m_map)
			pVisitor->visit(kv.first, kv.second);
	}
};

//
// array storage
//
template<Size c_nMaxSize>
class ArrayDictStorage : public IDictStorage
{
private:
	DictElem m_array[c_nMaxSize];
	Size m_nSize = 0;

	//
	// Find element position
	//
	const DictElem* find(DictKey key) const
	{
		for (Size i = 0; i < m_nSize; ++i)
			if (m_array[i].key == key)
				return &m_array[i];

		return nullptr;
	}

	// It's ugly but its safe
	DictElem* find(DictKey key)
	{
		return const_cast<DictElem*>(static_cast<const ArrayDictStorage&>(*this).find(key));
	}

	//
	// Find new place for put
	// Returns nullptr if storage is full
	//
	DictElem* findPutPosition(DictKey key)
	{
		// Find exist
		auto pElem = find(key);
		if (pElem != nullptr)
			return pElem;

		// Check full storage
		if (m_nSize == c_nMaxSize)
			return nullptr;

		++m_nSize;
		m_array[m_nSize-1].key = key;
		return &m_array[m_nSize-1];
	}


public:
	ArrayDictStorage() = default;
	~ArrayDictStorage() {}

	// Deny copy and move
	ArrayDictStorage(const ArrayDictStorage&) = delete;
	ArrayDictStorage(ArrayDictStorage&&) = delete;
	ArrayDictStorage& operator=(const ArrayDictStorage&) = delete;
	ArrayDictStorage& operator=(ArrayDictStorage&&) = delete;

	//////////////////////////////////////////////////////////////////////////
	//
	// IDictStorage
	//

	Size getSize() const override
	{
		return m_nSize;
	}

	bool isEmpty() const override
	{
		return m_nSize == 0;
	}

	bool has(DictKey key) const override
	{
		return (find(key) != nullptr);
	}

	Size erase(DictKey key) override
	{
		auto pElem = find(key);
		if (pElem == nullptr)
			return 0;

		// move data from item (erase data)
		Variant vData = std::move(pElem->val);

		// move last element to deleted position
		--m_nSize;
		if (&m_array[m_nSize] != pElem)
			*pElem = std::move(m_array[m_nSize]);
		return 1;
	}

	void clear() override
	{
		// erase data
		for (Size i = 0; i < m_nSize; ++i)
			m_array[i].val = nullptr;
		m_nSize = 0;
	}

	std::optional<Variant> getSafe(DictKey key) const override
	{
		auto pElem = find(key);
		if (pElem == nullptr)
			return std::nullopt;
		return pElem->val;
	}

	bool put(DictKey key, const Variant& value) override
	{
		auto pElem = findPutPosition(key);
		if (pElem == nullptr)
			return false;
		pElem->val = value;
		return true;
	}

	bool put(DictKey key, Variant&& value) override
	{
		auto pElem = findPutPosition(key);
		if (pElem == nullptr)
			return false;
		pElem->val = std::move(value);
		return true;
	}

	void applyForEach(IDictStorageVisitor* pVisitor) override
	{
		for (Size i = 0; i < m_nSize; ++i)
			pVisitor->visit(m_array[i].key, m_array[i].val);
	}
};


//////////////////////////////////////////////////////////////////////////
//
// BasicDictionary Iterator
//
class BasicDictionaryIterator: public IIterator
{
private:

	typedef std::vector<DictElem> Elements;
	mutable std::string m_sCurKey;
	Elements m_Elements;
	size_t m_nIndex = 0;

	void checkOutOfRange() const
	{
		if (m_nIndex >= m_Elements.size())
			error::OutOfRange(SL, "Access to finished iterator").throwException();
	}

	// Only BasicDictionary can create BasicDictionaryIterator
	friend BasicDictionary;

	// deny coping and moving
	BasicDictionaryIterator(const BasicDictionaryIterator&) = delete;
	BasicDictionaryIterator(BasicDictionaryIterator&&) = delete;

	// Private parameter of constructor
	struct PrivateKey
	{
	};
public:

	// "private" constructor supporting std::make_shared 
	// Create may use std::make_unique, but constructor can't be called external
	BasicDictionaryIterator(const PrivateKey&)
	{
	}

	//
	// IIterator
	//

	//
	//
	//
	DictionaryKeyRefType getKey() const override
	{
		checkOutOfRange();
		if (m_sCurKey.empty())
			m_sCurKey = m_Elements[m_nIndex].key.convertToStr();
		return m_sCurKey;
	}

	//
	//
	//
	const Variant getValue() const override
	{
		checkOutOfRange();
		return m_Elements[m_nIndex].val;
	}

	//
	//
	//
	void goNext() override
	{
		checkOutOfRange();
		m_nIndex++;
		m_sCurKey.clear();
	}

	//
	//
	//
	bool isEnd() const override
	{
		return m_nIndex >= m_Elements.size();
	}
};

//////////////////////////////////////////////////////////////////////////
//
// BasicDictionary
//
class BasicDictionary
{
public:
	typedef DictionaryKeyRefType IndexType;

private:
	std::unique_ptr<IDictStorage> m_pStorage;
	mutable std::shared_mutex m_mtxStorage;

	// Disable copy
	BasicDictionary(const BasicDictionary&) = delete;
	BasicDictionary(BasicDictionary&&) = delete;

	// Private parameter of constructor
	struct PrivateKey {};

	//
	// Create storage with required capacity
	//
	std::unique_ptr<IDictStorage> createStorage(Size nCapacity)
	{
		if(nCapacity <= 3)
			return std::make_unique<ArrayDictStorage<3>>();
		if (nCapacity <= 6)
			return std::make_unique<ArrayDictStorage<6>>();
		if (nCapacity <= 12)
			return std::make_unique<ArrayDictStorage<12>>();
		return std::make_unique<UnorderedMapDictStorage>();
	}

public:

	// "private" constructor supporting std::make_shared 
	// Create may use std::make_shared, but constructor can't be called external
	BasicDictionary(const PrivateKey&)
	{
	}

	//
	// Alloc BasicDictionary with specified capacity
	// Optimization for Dictionary with known size
	//
	static std::shared_ptr<BasicDictionary> create(Size nCapacity = 0)
	{
		auto pDict = std::make_shared<BasicDictionary>(PrivateKey());
		if (nCapacity != 0)
			pDict->m_pStorage = pDict->createStorage(nCapacity);
		return pDict;
	}

	~BasicDictionary() {}

	//
	//
	//
	Size getSize() const
	{
		std::shared_lock _lock(m_mtxStorage);
		if (m_pStorage == nullptr)
			return 0;
		return m_pStorage->getSize();
	}

	//
	//
	//
	bool isEmpty() const
	{
		std::shared_lock _lock(m_mtxStorage);
		if (m_pStorage == nullptr)
			return true;
		return m_pStorage->isEmpty();
	}

	//
	//
	//
	bool has(const IndexType sName) const
	{
		std::shared_lock _lock(m_mtxStorage);
		if (m_pStorage == nullptr)
			return false;
		return m_pStorage->has(DictKey(sName));
	}

	//
	//
	//
	Size erase(const IndexType sName)
	{
		std::unique_lock _lock(m_mtxStorage);
		if (m_pStorage == nullptr)
			return 0;
		return m_pStorage->erase(DictKey(sName));
	}

	//
	//
	//
	void clear()
	{
		std::unique_lock _lock(m_mtxStorage);
		if (m_pStorage == nullptr)
			return;
		return m_pStorage->clear();
	}

	//
	//
	//
	Variant get(const IndexType sName) const
	{
		std::shared_lock _lock(m_mtxStorage);
		if (m_pStorage != nullptr)
		{
			auto vVal = m_pStorage->getSafe(DictKey(sName));
			if (vVal.has_value())
				return vVal.value();
		}
		
		error::OutOfRange(SL, FMT("Invalid dictionary index <" << sName << ">")).throwException();
	}

	//
	//
	//
	std::optional<Variant> getSafe(const IndexType sName) const
	{
		std::shared_lock _lock(m_mtxStorage);
		if (m_pStorage == nullptr)
			return std::nullopt;
		return m_pStorage->getSafe(DictKey(sName));
	}

	//
	//
	//
	std::unique_ptr<IIterator> createIterator(bool /*fUnsafe*/ = false)
	{
		// Unsafe iterator is not supported. Safe iter is always returned
		auto DictIterator = std::make_unique<BasicDictionaryIterator>(BasicDictionaryIterator::PrivateKey());

		std::shared_lock _lock(m_mtxStorage);
		Size nSize = m_pStorage == nullptr ? 0 : m_pStorage->getSize();
		if (nSize == 0)
			return DictIterator;

		auto& iteratorElements = DictIterator->m_Elements;
		iteratorElements.reserve(nSize);

		applyForEach(m_pStorage, [&iteratorElements](DictKey key, Variant& val)
		{
			iteratorElements.emplace_back(key, val);
		});
		return DictIterator;
	}

	//
	// Deep clone
	//
	Variant clone(bool fResolveProxy)
	{
		std::unique_ptr<IDictStorage> pNewStorage;

		// do shallow copy of storage
		do
		{
			std::shared_lock _lock(m_mtxStorage);
			if(m_pStorage == nullptr || m_pStorage->isEmpty())
				break;
			pNewStorage = createStorage(m_pStorage->getSize());

			applyForEach(m_pStorage, [&pNewStorage](DictKey key, Variant& val)
			{
				// It is impossible, but check it
				if (!pNewStorage->put(key, val))
					error::LogicError(SL, "Invalid dictionary storage state.").throwException();
			});

		} while (false);

		// Deep clone internal elements
		applyForEach(pNewStorage, [fResolveProxy](DictKey /*key*/, Variant& val)
		{
			cloneContainer(val, fResolveProxy);
		});

		// Create new BasicDictionary
		auto pNewContainer = create();
		pNewContainer->m_pStorage = std::move(pNewStorage);
		return createVariant(pNewContainer);
	}

	//
	//
	//
	void put(const IndexType sName, const Variant& value)
	{
		std::unique_lock _lock(m_mtxStorage);
		if (m_pStorage == nullptr)
			m_pStorage = createStorage(1);

		DictKey key(sName, true);
		// if storage is not full just return
		if (m_pStorage->put(key, value))
			return;

		// Storage is full. Transform it.
		std::unique_ptr<IDictStorage> pNewStorage = createStorage(m_pStorage->getSize() + 1);
		std::unique_ptr<IDictStorage> pOldStorage = std::move(m_pStorage);
		applyForEach(pOldStorage, [&pNewStorage](DictKey key, Variant& val)
		{
			// It is impossible, but check it
			if (!pNewStorage->put(key, std::move(val)))
				error::LogicError(SL, "Can't copy dictionary storage state").throwException();
		});

		if (!pNewStorage->put(key, value))
			error::LogicError(SL, "Invalid dictionary storage state").throwException();

		m_pStorage = std::move(pNewStorage);
	}
};

} // namespace detail
} // namespace variant
} // namespace openEdr
