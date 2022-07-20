//
// EDRAv2.libcore project
//
// IIterator interface for Variant type
//
// Autor: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (06.12.2018)
//
#pragma once
#include "forward.hpp"

namespace cmd {
namespace variant {

////////////////////////////////////////////////////////////////////////////////
//
// ConstSequenceIterator class
//
class ConstSequenceIterator
{
public:
	typedef Variant ValueType;

private:
	// Keeps iterator or null for end iterator
	std::unique_ptr<IIterator> m_pIterator;

	mutable bool m_fHasCache = false; //< m_CachedValue has value
	mutable ValueType m_CachedValue; // Current element

	//
	//
	//
	void checkHasIterator() const
	{
		if (!m_pIterator)
			error::LogicError(SL, "Invalid iterator state").throwException();
	}

	//
	//
	//
	bool isEnd() const
	{
		return m_pIterator == nullptr || m_pIterator->isEnd();
	}

	//
	// Get value of current item
	//
	const ValueType& getValue() const
	{
		// fill cache
		if (!m_fHasCache)
		{
			checkHasIterator();
			m_CachedValue = m_pIterator->getValue();
			m_fHasCache = true;
		}

		// return cache
		return m_CachedValue;
	}

public:
	explicit ConstSequenceIterator(std::unique_ptr<IIterator>&& pInternalIterator) :
		m_pIterator(std::move(pInternalIterator))
	{}

	ConstSequenceIterator() noexcept = default;
	~ConstSequenceIterator() noexcept = default;

	// Deny coping, allow moving
	ConstSequenceIterator(const ConstSequenceIterator&) = delete;
	ConstSequenceIterator(ConstSequenceIterator&& other) noexcept = default;
	ConstSequenceIterator& operator = (ConstSequenceIterator&& other) noexcept = default;

	//
	// Get next item
	//
	ConstSequenceIterator& operator++()
	{
		checkHasIterator();
		m_fHasCache = false;
		m_pIterator->goNext();
		return *this;
	}

	//
	// Get value
	//
	const ValueType& operator*() const
	{
		return getValue();
	}

	//
	// Access to value
	//
	const ValueType* operator->() const
	{
		return &getValue();
	}

	//
	// Check the end
	//
	bool operator!=(const ConstSequenceIterator& other) const
	{
		return !isEnd() || !other.isEnd();
	}
};

////////////////////////////////////////////////////////////////////////////////
//
// Dictionary iterator 
//
class ConstDictionaryIterator
{
public:
	typedef std::pair<DictionaryKeyRefType, Variant> ValueType;

private:
	// Keeps iterator or null for end iterator
	std::unique_ptr<IIterator> m_pIterator;

	void checkHasIterator() const
	{
		if (!m_pIterator)
			error::LogicError(SL, "Invalid iterator state").throwException();
	}

	bool isEnd() const
	{
		return m_pIterator == nullptr || m_pIterator->isEnd();
	}

	// Cache pair to return reference from operator*
	mutable ValueType m_CachedValue;
	mutable bool m_fHasCache = false;

	//
	//
	//
	const ValueType& getCachedValue() const
	{
		if (!m_fHasCache)
		{
			checkHasIterator();
			ValueType newValue(m_pIterator->getKey(), m_pIterator->getValue());
			swap(m_CachedValue, newValue);
			m_fHasCache = true;
		}

		return m_CachedValue;
	}

public:
	explicit ConstDictionaryIterator(std::unique_ptr<IIterator>&& pInternalIterator) :
		m_pIterator(std::move(pInternalIterator)) {}

	ConstDictionaryIterator() noexcept = default;
	~ConstDictionaryIterator() noexcept = default;

	// Deny coping, allow moving
	ConstDictionaryIterator(const ConstDictionaryIterator&) = delete;
	ConstDictionaryIterator(ConstDictionaryIterator&& other) noexcept = default;
	ConstDictionaryIterator& operator = (ConstDictionaryIterator&& other) noexcept = default;

	//
	// Go to next item
	//
	ConstDictionaryIterator& operator++()
	{
		checkHasIterator();
		m_fHasCache = false;
		m_pIterator->goNext();
		return *this;
	}

	//
	// Return pair (key,value)
	//
	const ValueType& operator*() const
	{
		return getCachedValue();
	}

	//
	// Return pair (key,value)
	//
	const ValueType* operator->() const
	{
		return &getCachedValue();
	}

	//
	// Check if is end
	//
	bool operator!=(const ConstDictionaryIterator& other) const
	{
		return !isEnd() || !other.isEnd();
	}
};

namespace detail {

////////////////////////////////////////////////////////////////////////////////
//
// IIterator for scalars
//
class ScalarIterator : public IIterator
{
private:
	Variant m_vValue;
	bool m_fIsEnd = false;

	void checkOutOfRange() const
	{
		if (m_fIsEnd)
			error::OutOfRange("Access to finished iterator").throwException();
	}

public:
	explicit ScalarIterator(const Variant& vValue) : m_vValue(vValue) {}
	~ScalarIterator() override = default;

	// Go to next element
	void goNext() override
	{
		checkOutOfRange();
		m_fIsEnd = true;
	}

	// Checking end of iteration
	bool isEnd() const override
	{
		return m_fIsEnd;
	}

	// return key of current element (for Dictionary only)
	DictionaryKeyRefType getKey() const override
	{
		error::TypeError(SL, "Scalar type does not support string key").throwException();
	}

	// return value of current element
	const Variant getValue() const override
	{
		checkOutOfRange();
		return m_vValue;
	}
};

////////////////////////////////////////////////////////////////////////////////
//
// IIterator for scalars
//
class NullIterator : public IIterator
{
public:
	NullIterator() {}
	~NullIterator() override = default;

	// Go to next element
	void goNext() override
	{
	}

	// Checking end of iteration
	bool isEnd() const override
	{
		return true;
	}

	// return key of current element (for Dictionary only)
	DictionaryKeyRefType getKey() const override
	{
		error::TypeError(SL, "Not supported in NullIterator").throwException();
	}

	// return value of current element
	const Variant getValue() const override
	{
		error::TypeError(SL, "Not supported in NullIterator").throwException();
	}
};

} // namespace detail {


} // namespace variant
} // namespace cmd
