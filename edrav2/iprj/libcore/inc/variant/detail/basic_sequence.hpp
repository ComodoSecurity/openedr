//
// EDRAv2.libcore project
//
// Internal sequence container
//
// Autor: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (11.12.2018)
//
#pragma once

#include "value.hpp"

namespace openEdr {
namespace variant {
namespace detail {

//
// Safe iterator policy
//
struct CopyVector
{
public:
	typedef std::vector<Variant> Elements;
	
	void setVector(const Elements& NewElements)
	{
		m_Elements = NewElements;
	}

	const Elements& getVector() const noexcept
	{
		return m_Elements;
	}

private:
	Elements m_Elements;
};

//
// Unsafe iterator policy
//
struct KeepPointer
{
public:
	typedef std::vector<Variant> Elements;
	void setVector(const Elements& NewElements)
	{
		m_pElements = &NewElements;
	}
	const Elements& getVector() const noexcept
	{
		return *m_pElements;
	}

private:
	const Elements* m_pElements;
};

//
//
//
template<typename KeepMethod /*= KeepPointer | CopySequence*/>
class BasicSequenceIterator: public KeepMethod, public IIterator
{
private:
	typedef BasicSequenceIterator<KeepMethod> SelfType;

	Size m_nIndex = 0;
	
	void checkOutOfRange() const
	{
		if (m_nIndex >= KeepMethod::getVector().size())
			error::OutOfRange(SL, "Access to finished iterator").throwException();
	}

	friend BasicSequence;

	// deny coping and moving
	BasicSequenceIterator(const BasicSequenceIterator&) = delete;
	BasicSequenceIterator(BasicSequenceIterator&&) = delete;

	// Private parameter of constructor
	struct PrivateKey
	{
	};

public:

	// "private" constructor supporting std::make_shared 
	// Create may use std::make_unique, but constructor can't be called external
	BasicSequenceIterator(const PrivateKey&)
	{
	}

	//
	//
	//
	static std::unique_ptr<IIterator> create(const std::vector<Variant>& Elements)
	{
		auto SeqIterator = std::make_unique<SelfType>(PrivateKey());
		SeqIterator->setVector(Elements);
		return SeqIterator;
	}

	//
	// IIterator
	//

	//
	//
	//
	DictionaryKeyRefType getKey() const override
	{
		error::TypeError(SL, "Sequence does not support string key").throwException();
	}

	//
	//
	//
	const Variant getValue() const override
	{
		checkOutOfRange();
		return KeepMethod::getVector()[m_nIndex];
	}

	//
	//
	//
	void goNext() override
	{
		checkOutOfRange();
		++m_nIndex;
	}

	//
	// Checking finish iteration
	//
	bool isEnd() const override
	{
		return m_nIndex >= KeepMethod::getVector().size();
	}
};

//
// BasicSequence internal data object
//
class BasicSequence
{
public:
	using IndexType = SequenceIndex;
	typedef std::vector<Variant> ValuesVector;

private:
	ValuesVector m_Vector;
	mutable std::shared_mutex m_mtxVector;

	// Disable copying
	BasicSequence(const BasicSequence&) = delete;
	BasicSequence(BasicSequence&&) = delete;

	// Private parameter of constructor
	struct PrivateKey
	{
	};

public:

	// "private" constructor supporting std::make_shared 
	// Create may use std::make_shared, but constructor can't be called external
	BasicSequence(const PrivateKey&)
	{
	}

	//
	//
	//
	static std::shared_ptr<BasicSequence> create()
	{
		return std::make_shared<BasicSequence>(PrivateKey());
	}

	//
	//
	//
	static std::shared_ptr<BasicSequence> create(ValuesVector&& InitValues)
	{
		std::shared_ptr<BasicSequence> pThis = create();
		pThis->m_Vector = std::move(InitValues);
		return pThis;
	}

	//
	//
	//
	~BasicSequence() {}

	//
	//
	//
	Size getSize() const
	{
		std::shared_lock _lock(m_mtxVector);
		return m_Vector.size();
	}

	//
	//
	//
	bool isEmpty() const
	{
		std::shared_lock _lock(m_mtxVector);
		return m_Vector.empty();
	}

	//
	// 
	//
	Size erase(IndexType nIndex)
	{
		std::unique_lock _lock(m_mtxVector);
		if (nIndex >= m_Vector.size())
			return 0;
		m_Vector.erase(m_Vector.begin()+nIndex);
		return 1;
	}

	//
	//
	//
	void clear()
	{
		std::unique_lock _lock(m_mtxVector);
		return m_Vector.clear();
	}

	//
	//
	//
	Variant get(IndexType nIndex) const
	{
		std::shared_lock _lock(m_mtxVector);
		if (nIndex >= m_Vector.size())
			error::OutOfRange(SL, FMT("Invalid sequence index: " << nIndex << ". Size: " << m_Vector.size())).throwException();
		return m_Vector[nIndex];
	}

	//
	//
	//
	std::optional<Variant> getSafe(IndexType nIndex) const
	{
		std::shared_lock _lock(m_mtxVector);
		if (nIndex >= m_Vector.size())
			return {};
		return m_Vector[nIndex];
	}

	//
	//
	//
	void put(IndexType nIndex, const Variant& vValue)
	{
		std::unique_lock _lock(m_mtxVector);
		if (nIndex >= m_Vector.size())
			error::OutOfRange(SL, FMT("Invalid sequence index: " << nIndex << ". Size: " << m_Vector.size())).throwException();
		m_Vector[nIndex] = vValue;
	}

	//
	//
	//
	void push_back(const Variant& vValue)
	{
		std::unique_lock _lock(m_mtxVector);
		m_Vector.push_back(vValue);
	}

	//
	//
	//
	void setSize(Size nSize)
	{
		std::unique_lock _lock(m_mtxVector);
		m_Vector.resize(nSize);
	}

	//
	//
	//
	void insert(IndexType nIndex, const Variant& vValue)
	{
		std::unique_lock _lock(m_mtxVector);
		if (nIndex > m_Vector.size())
			error::OutOfRange(SL, FMT("Invalid sequence index: " << nIndex << ". Size: " << m_Vector.size())).throwException();
		m_Vector.insert(m_Vector.begin() + nIndex, vValue);
	}

	//
	//
	//
	std::unique_ptr<IIterator> createIterator(bool fUnsafe=false) const
	{
		std::shared_lock _lock(m_mtxVector);
		if (!fUnsafe)
			return BasicSequenceIterator<CopyVector>::create(m_Vector);
		return BasicSequenceIterator<KeepPointer>::create(m_Vector);
	}

	//
	// Deep clone
	//
	Variant clone(bool fResolveProxy) const
	{
		auto pNewContainer = create();

		// Container shallow copy
		{
			std::shared_lock _lock(m_mtxVector);
			pNewContainer->m_Vector = m_Vector;
		}

		// Clone interanl elements
		for (auto& vElem : pNewContainer->m_Vector)
			cloneContainer(vElem, fResolveProxy);

		return createVariant(pNewContainer);
	}
};

} // namespace detail
} // namespace variant
} // namespace openEdr
