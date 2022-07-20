//
// EDRAv2.libcore project
//
// Dictionary type
//
// Autor: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (10.12.2018)
//
#pragma once

namespace cmd {
namespace variant {

//
// Dictionary class
//
class Dictionary
{
public:
	using Index = DictionaryKeyRefType;
	typedef ConstDictionaryIterator const_iterator;

	struct InitizalierListItem
	{
		std::string m_sKey;
		Variant m_vValue;
	};

private:
	using BasicDictionary = detail::BasicDictionary;

	Variant m_vValue;

public:

	//
	// Constructors/assign/destructor
	//
	Dictionary()
	{
		m_vValue = createVariant(detail::BasicDictionary::create());
	}
	Dictionary(const Dictionary&) = default;
	Dictionary(Dictionary&& other) noexcept = default;
	Dictionary& operator= (const Dictionary&) = default;
	Dictionary& operator= (Dictionary&&) = default;

	~Dictionary() = default;

	//
	// Constructor from initializer_list.
	// Example: { {key1, value1}, {key2, value2}, ...}
	//   key* - string
	//   value* - value of any type which can be converted to Variant.
	//
	Dictionary(std::initializer_list<InitizalierListItem> InitList)
	{
		std::shared_ptr<BasicDictionary> pBasicDictionary = detail::BasicDictionary::create(InitList.size());
		for (auto& Item : InitList)
			pBasicDictionary->put(Item.m_sKey, Item.m_vValue);

		m_vValue = createVariant(pBasicDictionary);
	}

	//
	//
	//
	Dictionary(const Variant& v)
	{
		if(!v.isDictionaryLike())
			error::TypeError(SL, "Can't convert variant to Dictionary").throwException();
		m_vValue = v;
	}

	//
	//
	//
	operator Variant() const
	{
		return m_vValue;
	}

	//
	// Compare
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
	// Common functions
	//

	//
	//
	//
	Size getSize() const
	{
		return  m_vValue.getSize();
	}
	
	//
	// Check container emptiness.
	//
	bool isEmpty() const
	{
		return  m_vValue.isEmpty();
	}
	
	//
	// Remove all elements from container.
	//
	void clear()
	{
		 m_vValue.clear();
	}
	
	//
	// Remove element by ID. It returns number of removed elements.
	//
	Size erase(const Index& Id)
	{
		return m_vValue.erase(Id);
	}

	//
	// Check element existence in dictionary 
	//
	bool has(const Index& Id) const
	{
		return  m_vValue.has(Id);
	}

	//
	// Get element by key. It throws exception, if the element is not existed. 
	//
	Variant get(const Index& Id) const
	{
		return m_vValue.get(Id);
	}

	//
	// Get element by key. It return vDefaultValue, if the element is not existed. 
	//
	Variant get(const Index& Id, const Variant& vDefaultValue) const
	{
		return m_vValue.get(Id, vDefaultValue);
	}
	
	//
	// Put element by key
	//
	void put(const Index& Id, const Variant& vValue)
	{
		 m_vValue.put(Id, vValue);
	}

	//
	// Get element by key
	//
	const Variant operator[](const Index& Id) const
	{
		return get(Id);
	}

	//
	// Variant deep cloning
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
	// Iteration interface (readonly)
	//

	//
	//
	//
	const_iterator begin() const
	{
		return detail::createDictionaryIterator(m_vValue, false);
	}
	
	//
	//
	//
	const_iterator end() const
	{
		return const_iterator();
	}

	//
	//
	//
	const_iterator unsafeBegin() const
	{
		return detail::createDictionaryIterator(m_vValue, true);
	}
	
	//
	//
	//
	const_iterator unsafeEnd() const
	{
		return const_iterator();
	}

	//
	// "Range-based for" adapter for unsafe iteration
	//
	auto getUnsafeIterable() const
	{
		typedef decltype(*this) Container;
		struct
		{
			const Container& m_Contaner;
			auto begin() const { return m_Contaner.unsafeBegin(); }
			auto end() const { return m_Contaner.unsafeEnd(); }
		} adaptor{ *this };
		return adaptor;
	}

};

///
/// Output of humanreadable representation
///
inline std::ostream& operator<<(std::ostream& oStrStream, const Dictionary& vVal)
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
struct is_range<cmd::variant::Dictionary> {
	static const bool value = false;
};
}
#endif // CATCH_VERSION_MAJOR


