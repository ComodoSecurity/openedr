//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (15.06.2019)
// Reviewer:
//
///
/// @file hash map wrapper
///
/// @addtogroup kernelCmnLib
/// @{
#pragma once
#include "list.hpp"

namespace cmd {

//////////////////////////////////////////////////////////////////////////
//
// HashMap, HashSet
//

///
/// std::unordered_map analog.
///
/// @tparam Key - key type.
/// @tparam Value - value type.
/// @tparam c_ePoolType - memory pool for allocation.
/// @tparam c_nBacketCount - count of buckets.
///
template<typename Key, typename Value, size_t c_nBucketCount = 256, PoolType c_ePoolType = PoolType::NonPaged>
class HashMap
{
public:
	// Data stored in HashMap
	struct Data
	{
		Key first;
		Value second;
	};

private:

	typedef std::hash<Key> FnHasher;
	typedef List<Data, c_ePoolType> ElemList;
	typedef size_t BucketId;
	static constexpr BucketId c_nInvalidBucketId = BucketId(-1);
	
	//
	//
	//
	struct Bucket
	{
		ElemList elemList; // List of elements of Bucket
	};

	
	// TODO: dynamic c_nBucketCount and rehash

	Bucket m_Buckets[c_nBucketCount]; // Buckets
	size_t m_nItemCount = 0; // Item count

	//
	//
	//
	inline Bucket& getBacket(const Key& key)
	{
		auto hash = FnHasher()(key);
		BucketId nBucketId = hash % c_nBucketCount;
		return m_Buckets[nBucketId];
	}

public:

	//
	// IteratorBase
	//
	template<bool c_fConst>
	class IteratorBase
	{
	public:
		typedef std::conditional_t<c_fConst, typename ElemList::const_iterator, 
			typename ElemList::iterator> ListIter;
		typedef std::conditional_t<c_fConst, const HashMap*, HashMap*> ContainerPtr;
		typedef std::conditional_t<c_fConst, const Data, Data> ReturnedData;

	private:
		friend HashMap;
		ListIter m_listIter;
		BucketId m_nBucketId = c_nInvalidBucketId;
		ContainerPtr m_pContainer = nullptr;

		//
		// Find first or next non-empty bucket and init point to it.
		// If 
		//
		void selectNextBucket()
		{
			const BucketId nFirstBucket = m_nBucketId == c_nInvalidBucketId ? 0 : m_nBucketId + 1;
			for (BucketId i = nFirstBucket; i < c_nBucketCount; ++i)
			{
				auto& bucketElemList = m_pContainer->m_Buckets[i].elemList;
				if(bucketElemList.isEmpty())
					continue;
				
				m_listIter = bucketElemList.begin();
				m_nBucketId = i;
				return;
			}

			// Bucket is not found
			m_nBucketId = c_nInvalidBucketId;
		}

		//
		// for begin()
		//
		static IteratorBase createBegin(ContainerPtr pContainer)
		{
			IteratorBase iter(pContainer);
			iter.selectNextBucket();
			return iter;
		}

		//
		// for end()
		//
		static IteratorBase createEnd(ContainerPtr pContainer)
		{
			return IteratorBase(pContainer);
		}

		//
		//
		//
		explicit IteratorBase(ContainerPtr pContainer) : m_pContainer(pContainer)
		{
		}

		//
		//
		//
		IteratorBase(ContainerPtr pContainer, BucketId nBucketId, const ListIter& listIter) :
			m_pContainer(pContainer), m_nBucketId(nBucketId), m_listIter(listIter)
		{
		}

	public:
		IteratorBase() = default;

		//
		// Access to value
		//
		ReturnedData& operator* () const { return *m_listIter; }
		ReturnedData* operator-> () const { return &*m_listIter; }

		//
		// Check the end
		//
		bool operator!=(const IteratorBase& other) const
		{
			if (m_nBucketId != other.m_nBucketId)
				return true;
			if (m_nBucketId == c_nInvalidBucketId)
				return false;
			return m_listIter != other.m_listIter;
		}

		//
		// Get next item
		//
		IteratorBase& operator++()
		{
			if (m_nBucketId == c_nInvalidBucketId || m_pContainer == nullptr)
				return *this;

			// Goto next item in bucket
			++m_listIter;
			auto& bucket = m_pContainer->m_Buckets[m_nBucketId];
			if (m_listIter != bucket.elemList.end())
				return *this;

			// Goto next bucket
			selectNextBucket();
			return *this;
		}
	};

	using iterator = IteratorBase<false>;
	using const_iterator = IteratorBase<true>;


private:

	template<typename ThisPtr>
	[[nodiscard]] static auto findInt(ThisPtr pThis, const Key& key)
	{
		auto hash = FnHasher()(key);
		BucketId nBucketId = hash % c_nBucketCount;
		auto& bucket = pThis->m_Buckets[nBucketId];

		typedef decltype(pThis->end()) ReturnedIterator;

		auto endIter = bucket.elemList.end();
		for (auto iter = bucket.elemList.begin(); iter != endIter; ++iter)
			if (key == iter->first)
				return ReturnedIterator(pThis, nBucketId, iter);

		return pThis->end();
	}

	//
	// Move all elements from "other" 
	//
	void moveElements(HashMap& other)
	{
		for (BucketId i = 0; i < c_nBucketCount; ++i)
			m_Buckets[i].elemList = std::move(other.m_Buckets[i].elemList);
	}

public:

	//
	//
	//
	HashMap()
	{
	}

	//
	//
	//
	~HashMap()
	{
		clear();
	}

	// Deny copy
	HashMap(const HashMap& other) = delete;
	HashMap& operator=(const HashMap& other) = delete;

	// Allow move
	HashMap(HashMap&& other)
	{
		moveElements(other);
	}
	HashMap& operator=(HashMap&& other)
	{
		moveElements(other);
		return *this;
	}

	//
	//
	//
	void clear()
	{
		for (auto& bucket : m_Buckets)
			bucket.elemList.clear();
		m_nItemCount = 0;
	}

	//
	//
	//
	[[nodiscard]] bool isEmpty() const
	{
		return m_nItemCount == 0;
	}

	//
	//
	//
	[[nodiscard]] size_t getSize() const
	{
		return m_nItemCount;
	}

	//
	//
	//
	[[nodiscard]] iterator begin() { return iterator::createBegin(this); }
	[[nodiscard]] const_iterator begin() const { return const_iterator::createBegin(this); }
	[[nodiscard]] iterator end() { return iterator::createEnd(this); }
	[[nodiscard]] const_iterator end() const { return iterator::createEnd(this); }

	//
	//
	//
	[[nodiscard]] auto find(const Key& key) { return findInt(this, key); }
	[[nodiscard]] auto find(const Key& key) const { return findInt(this, key); }

	//
	// Insert with copy constructor
	//
	[[nodiscard]] NTSTATUS insert(const Key& key, const Value& value)
	{
		auto& bucket = getBacket(key);
		
		// Find exist
		for (auto& elem : bucket.elemList)
		{
			if(key != elem.first)
				continue;
			elem.second = value;
			return STATUS_SUCCESS;
		}

		// Create new
		Data data {std::move(key), std::move(value) };
		NTSTATUS eStatus = bucket.elemList.pushBack(std::move(data));
		if (!NT_SUCCESS(eStatus))
			return eStatus;
		++m_nItemCount;
		return eStatus;
	}

	//
	//
	//
	[[nodiscard]] NTSTATUS findOrInsert(const Key& key, Value** ppValue)
	{
		if (ppValue == nullptr)
			return STATUS_INVALID_PARAMETER;

		auto& bucket = getBacket(key);

		// Find exist
		for (auto& elem : bucket.elemList)
		{
			if (key != elem.first)
				continue;
			*ppValue = &elem.second;
			return STATUS_SUCCESS;
		}

		// Create new
		Data data{ std::move(key) };
		NTSTATUS eStatus = bucket.elemList.pushBack(std::move(data));
		if (!NT_SUCCESS(eStatus))
			return eStatus;
		++m_nItemCount;
		*ppValue = &bucket.elemList.getBack().second;
		return STATUS_SUCCESS;
	}


	//
	//
	//
	size_t remove(const Key& key)
	{
		auto& elemList = getBacket(key).elemList;

		auto endIter = elemList.end();
		for (auto iter = elemList.begin(); iter != endIter; ++iter)
			if (iter->first == key)
			{
				auto nRemovedCount = elemList.remove(iter);
				m_nItemCount -= nRemovedCount;
				return nRemovedCount;
			}
		return 0;
	}

	//
	//
	//
	iterator remove(const iterator& iter)
	{
		if (iter.m_pContainer != this || iter.m_nBucketId == c_nInvalidBucketId)
			return iter;

		iterator nextIter = iter;
		++nextIter;

		m_Buckets[iter.m_nBucketId].elemList.remove(iter.m_listIter);
		m_nItemCount -= 1;
		return nextIter;
	}
};

} // namespace cmd

/// @}
