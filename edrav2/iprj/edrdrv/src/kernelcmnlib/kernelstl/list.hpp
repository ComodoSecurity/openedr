//
// edrav2.edrdrv
//
// Author: Yury Podpruzhnikov (15.06.2019)
// Reviewer:
//
///
/// @file Double linked list (with LIST_ENTRY)
///
/// @addtogroup kernelCmnLib
/// @{
#pragma once

namespace cmd {

//////////////////////////////////////////////////////////////////////////
//
// Double linked list (with LIST_ENTRY)
//

template<typename Data, PoolType c_ePoolType = PoolType::NonPaged>
class List
{
private:
	LIST_ENTRY m_Head = {}; //< Head

	//
	// Internal element
	//
	struct ListElem
	{
		LIST_ENTRY listEntry = {};
		Data m_data;

		ListElem() = default;
		ListElem(const Data& data) : m_data(data) {}
		ListElem(Data&& data) : m_data(std::move(data)) {}
	};


	//
	// Converts LIST_ENTRY into ListElem
	//
	static ListElem* getElem(const LIST_ENTRY* pListEntry)
	{
		return (ListElem*)CONTAINING_RECORD(pListEntry, ListElem, listEntry);
	}

	//
	//
	//
	[[nodiscard]] NTSTATUS pushBack(ListElem* pElem)
	{
		if (pElem == nullptr)
			return STATUS_NO_MEMORY;
		InsertTailList(&m_Head, &pElem->listEntry);
		return STATUS_SUCCESS;
	}

	//
	//
	//
	[[nodiscard]] NTSTATUS pushFront(ListElem* pElem)
	{
		if (pElem == nullptr)
			return STATUS_NO_MEMORY;
		InsertHeadList(&m_Head, &pElem->listEntry);
		return STATUS_SUCCESS;
	}

public:

	//
	// IteratorBase
	//
	template<bool c_fConst>
	class IteratorBase
	{
	private:
		typedef std::conditional_t<c_fConst, const Data, Data> ReturnedData;
		typedef std::conditional_t<c_fConst, const LIST_ENTRY*, LIST_ENTRY*> ListEntryPtr;

		friend List;
		ListEntryPtr m_pListEntry = nullptr;
	public:
		IteratorBase() = default;
		IteratorBase(ListEntryPtr pListEntry) : m_pListEntry(pListEntry) {}

		//
		// Access to value
		//
		ReturnedData& operator* () const { return getElem(m_pListEntry)->m_data; }
		ReturnedData* operator-> () const { return &getElem(m_pListEntry)->m_data; }

		//
		// Check the end
		//
		bool operator!=(const IteratorBase& other) const
		{
			return m_pListEntry != other.m_pListEntry;
		}

		//
		// Get next item
		//
		IteratorBase& operator++()
		{
			m_pListEntry = m_pListEntry->Flink;
			return *this;
		}

		//
		// Get prev item
		//
		IteratorBase& operator--()
		{
			m_pListEntry = m_pListEntry->Blink;
			return *this;
		}
	};

	using iterator = IteratorBase<false>;
	using const_iterator = IteratorBase<true>;

private:

	//
	//
	//
	template <typename This>
	[[nodiscard]] static auto findInt(This* pThis, const Data& data)
	{
		auto endIter = pThis->end();
		for (auto iter = pThis->begin(); iter != endIter; ++iter)
			if (*iter == data)
				return iter;
		return endIter;
	}

	//
	// Move all elements from "other" to the tail of this list
	// The "other" becomes empty list
	//
	void moveElementsFromList(List& other)
	{
		if (other.isEmpty())
			return;

		PLIST_ENTRY entry = other.m_Head.Flink;
		RemoveEntryList(&other.m_Head);
		InitializeListHead(&other.m_Head);
		AppendTailList(&m_Head, entry);
	}

	void freeListEntry(PLIST_ENTRY pListElem)
	{
		ListElem* pElem = getElem(pListElem);
		delete pElem;
	}


public:

	//
	//
	//
	List()
	{
		InitializeListHead(&m_Head);
	}

	//
	//
	//
	~List()
	{
		clear();
	}

	// Deny copy
	List(const List& other) = delete;
	List& operator=(const List& other) = delete;

	// Allow move
	List(List&& other)
	{
		InitializeListHead(&m_Head);
		moveElementsFromList(other);
	}

	// Allow move
	List& operator=(List&& other)
	{
		clear();
		moveElementsFromList(other);
		return *this;
	}

	//
	//
	//
	void clear()
	{
		auto pHead = &m_Head;
		while (!IsListEmpty(pHead))
			freeListEntry(RemoveHeadList(pHead));
		InitializeListHead(&m_Head);
	}

	//
	//
	//
	[[nodiscard]] bool isEmpty() const
	{
		return IsListEmpty(&m_Head);
	}

	//
	//
	//
	[[nodiscard]] size_t getSize() const
	{
		size_t nSize = 0;
		auto endIter = end();
		for (auto iter = begin(); iter != endIter; ++iter)
			++nSize;

		return nSize;
	}

	//
	//
	//
	[[nodiscard]] NTSTATUS pushFront(const Data& data)
	{
		return pushFront(new (c_ePoolType) ListElem(data));
	}

	//
	//
	//
	[[nodiscard]] NTSTATUS pushFront(Data&& data)
	{
		return pushFront(new (c_ePoolType) ListElem(std::move(data)));
	}

	//
	//
	//
	[[nodiscard]] NTSTATUS pushBack(const Data& data)
	{
		return pushBack(new (c_ePoolType) ListElem(data));
	}

	//
	//
	//
	[[nodiscard]] NTSTATUS pushBack(Data&& data)
	{
		return pushBack(new (c_ePoolType) ListElem(std::move(data)));
	}

	//
	// Iteration support (range-based for)
	//
	[[nodiscard]] iterator begin() { return iterator(m_Head.Flink); }
	[[nodiscard]] const_iterator begin() const { return const_iterator(m_Head.Flink); }
	[[nodiscard]] iterator end() { return iterator(&m_Head); }
	[[nodiscard]] const_iterator end() const { return const_iterator(&m_Head); }

	//
	// returns first element of mutable sequence
	//
	[[nodiscard]] Data& getFront()
	{
		return (*begin());
	}

	//
	// returns first element of nonmutable sequence
	//
	[[nodiscard]] const Data& getFront() const
	{
		return (*begin());
	}

	//
	// returns last element of mutable sequence
	//
	[[nodiscard]] Data& getBack()
	{
		return (*(--end()));
	}

	//
	// returns last element of nonmutable sequence
	//
	[[nodiscard]] const Data& getBack() const
	{
		return (*(--end()));
	}

	//
	//
	//
	[[nodiscard]] iterator find(const Data& data)
	{
		return findInt(this, data);
	}

	//
	//
	//
	[[nodiscard]] const_iterator find(const Data& data) const
	{
		return findInt(this, data);
	}

	//
	//
	//
	iterator remove(const iterator& iter)
	{
		iterator result = iter;
		++result;
		RemoveEntryList(iter.m_pListEntry);
		freeListEntry(iter.m_pListEntry);
		return result;
	}

	//
	// Remove by condition (bool Predicate(const Data& val) )
	// returns count of removed items.
	//
	template<typename Predicate>
	size_t removeIf(Predicate pred)
	{
		size_t nRemovedCount = 0;
		auto endIter = end();
		for (auto iter = begin(); iter != endIter;)
		{
			if (pred(*iter))
			{
				++nRemovedCount;
				iter = remove(iter);
			}
			else
				++iter;
		}

		return nRemovedCount;
	}

	//
	//
	//
	[[nodiscard]] bool has(const Data& data)
	{
		return find(data) != end();
	}

	//
	// Move element(-s) from other list to end of this
	// Move 1 element pointed by iterator
	//
	void spliceBack(List& /*other*/, iterator& otherIter)
	{
		PLIST_ENTRY pListEntry = const_cast<PLIST_ENTRY>(otherIter.m_pListEntry);
		RemoveEntryList(pListEntry); // remove from other list
		InsertTailList(&m_Head, pListEntry); // push to this
	}

	//
	// Move element(-s) from other list to end of this
	// Move all elements of other
	//
	void spliceBack(List& other)
	{
		moveElementsFromList(other);
	}
};

} // namespace cmd

/// @}
