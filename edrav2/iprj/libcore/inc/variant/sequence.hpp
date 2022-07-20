//
// EDRAv2.libcore project
//
// Sequence type
//
// Autor: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (10.12.2018)
//
#pragma once

namespace cmd {
namespace variant {

//
// Sequence object
//
class Sequence
{
public:
	using Index = SequenceIndex;

private:
	Variant m_vValue;

	//
	// Init internal variant from std::vector<Variant> (move semantic)
	//
	void initFromVector(detail::BasicSequence::ValuesVector&& Values)
	{
		m_vValue = std::move(detail::createVariant(detail::BasicSequence::create(std::move(Values))));
	}

	void InitFromIterable(const Variant& vIterable)
	{
		detail::BasicSequence::ValuesVector Values;
		Values.reserve(vIterable.getSize());

		for (auto vVal : vIterable)
			Values.push_back(vVal);
		initFromVector(std::move(Values));
	}

public:

	//
	// Constructors/assign/destructor
	//
	~Sequence() = default;
	Sequence(const Sequence&) = default;
	Sequence(Sequence&& other) noexcept = default;
	Sequence& operator=(const Sequence&) = default;
	Sequence& operator=(Sequence&&) = default;

	//
	//
	//
	Sequence()
	{
		m_vValue = std::move(detail::createVariant(detail::BasicSequence::create()));
	}

	//
	//
	//
	explicit Sequence(const Variant& v)
	{
		switch (v.getType())
		{
			case Variant::ValueType::Null:
			{
				m_vValue = Sequence();
				break;
			}
			case Variant::ValueType::Boolean:
			case Variant::ValueType::Integer:
			case Variant::ValueType::String:
			{
				m_vValue = Sequence({ v });
				break;
			}
			case Variant::ValueType::Sequence:
			{
				m_vValue = v;
				break;
			}
			case Variant::ValueType::Dictionary:
			{
				InitFromIterable(v);
				break;
			}
			case Variant::ValueType::Object:
			{
				if (v.isSequenceLike())
					m_vValue = v;
				else if (v.isDictionaryLike())
					InitFromIterable(v);
				else
					m_vValue = Sequence({ v });
				break;
			}
		}
	}

	//
	// Constructor from std::initializer_list
	// Example: v{value1, value2, ...}
	// valueN - value of any type which can be converted to Variant.
	//
	Sequence(std::initializer_list<Variant> InitList)
	{
		detail::BasicSequence::ValuesVector Values(InitList);
		initFromVector(std::move(Values));
	}

	//
	//
	//
	operator Variant()
	{
		return m_vValue;
	}
	operator Variant() const
	{
		return m_vValue;
	}

	//
	// Return number of elements in container
	//
	Size getSize() const
	{
		return m_vValue.getSize();
	}

	//
	// Check container emptiness
	//
	bool isEmpty() const
	{
		return m_vValue.isEmpty();
	}

	//
	// Remove all elements from container
	//
	void clear()
	{
		return m_vValue.clear();
	}

	//
	// Remove element by ID. It returns number of removed elements.
	//
	Size erase(Index Id)
	{
		return m_vValue.erase(Id);
	}

	//
	// Get element by ID.
	// It throws exception, if the element is not exist. 
	//
	Variant get(Index Id) const
	{
		return m_vValue.get(Id);
	}

	//
	// Get element by ID.
	// It return vDefaultValue, if the element is not exist. 
	//
	Variant get(Index Id, const Variant& vDefaultValue) const
	{
		return m_vValue.get(Id, vDefaultValue);
	}
	
	//
	// Set element by ID (without resizing)
	//
	void put(Index Id, const Variant& vValue)
	{
		return m_vValue.put(Id, vValue);
	}
	
	//
	// Append into sequence
	//
	void push_back(const Variant& vValue)
	{
		return m_vValue.push_back(vValue);
	}
	
	//
	// Append into sequence
	//
	Sequence& operator+=(const Variant& vValue)
	{
		m_vValue += vValue;
		return *this;
	}

	//
	// Resize Sequence. If getSize grows, null elements are added.
	//
	void setSize(Size nNewSize)
	{
		return m_vValue.setSize(nNewSize);
	}

	//
	//
	//
	void insert(Index Id, const Variant& vValue)
	{
		return m_vValue.insert(Id, vValue);
	}

	//
	// Get element by ID.
	//
	const Variant operator[](Index Id) const 
	{ 
		return m_vValue[Id];
	}

	//
	// Clone
	//
	Variant clone(bool fResolveProxy = false)
	{
		return m_vValue.clone(fResolveProxy);
	}

	///
	/// Returning humanreadable representation
	///
	std::string print() const
	{
		return m_vValue.print();
	}

	//
	// Comparators
	//

	bool operator==(const Variant& other) const
	{
		return m_vValue == other;
	}

	bool operator!=(const Variant& other) const
	{
		return m_vValue != other;
	}

	//
	// Default (safe) std iterator interface
	//
	typedef ConstSequenceIterator const_iterator;

	const_iterator begin() const
	{
		return m_vValue.begin();
	}
	
	const_iterator end() const
	{
		return m_vValue.end();
	}

	//
	// Unsafe iterator (not thread safe)
	//

	const_iterator unsafeBegin() const
	{
		return m_vValue.unsafeBegin();
	}
	
	const_iterator unsafeEnd() const
	{
		return m_vValue.unsafeEnd();
	}

	//
	// "Range-based for" adapter for unsafe iteration
	//
	auto getUnsafeIterable() const
	{
		struct
		{
			const decltype(*this)& m_Contaner;
			auto begin() const { return m_Contaner.unsafeBegin(); }
			auto end() const { return m_Contaner.unsafeEnd(); }
		} adaptor{ *this };
		return adaptor;
	}
};

///
/// Output of humanreadable representation
///
inline std::ostream& operator<<(std::ostream& oStrStream, const Sequence& vVal)
{
	oStrStream << Variant(vVal);
	return oStrStream;
}


} // namespace variant
} // namespace cmd

//////////////////////////////////////////////////////////////////////////
//
// Catch extension
//

// If catch2 is included
// Avoiding catch2 recursion
#ifdef CATCH_VERSION_MAJOR
namespace Catch {
template<>
struct is_range<cmd::variant::Sequence> {
	static const bool value = false;
};
}
#endif // CATCH_VERSION_MAJOR
