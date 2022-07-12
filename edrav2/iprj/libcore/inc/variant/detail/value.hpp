//
// edrav2.libcore
// Raw type for storing Variant internal data 
// 
// Autor: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (10.12.2018)
//
#pragma once

#define PRINT_HUMAN

#include "value_int.hpp"
#include "value_string.hpp"

namespace cmd {
namespace variant {
namespace detail {

// 
// @brief Enum of supported Variant's raw types
//
enum class RawValueType : std::uint8_t
{
	Null = 0,				
	Boolean = 1,			//< boolean value
	Integer = 2,			//< integer value up to 64-bit (signed or unsigned)
	String = 3,				//< string value
	Object = 4,				//< pointer to BaseObject
	Dictionary = 5,			//< dictionary (unordered set of name/value pairs)
	Sequence = 6,			//< sequence (ordered collection of values)
	DataProxy = 7,			//< Calculated value. For internal usage. It is never returned from getType().
	Max,					//< For internal usage
};

inline std::ostream& operator<<(std::ostream& oStrStream, const RawValueType eVal)
{
	switch (eVal)
	{
	case RawValueType::Null:
	case RawValueType::Boolean:
	case RawValueType::Integer:
	case RawValueType::String:
	case RawValueType::Object:
	case RawValueType::Dictionary:
	case RawValueType::Sequence:
		oStrStream << ValueType(eVal); break;
	case RawValueType::DataProxy: oStrStream << "DataProxy"; break;
	default: oStrStream << "INVALID_VALUE"; break;
	}
	return oStrStream;
}



class BasicDictionary;
class BasicSequence;
class IDataProxy;

//
// Get ValueType by Type
//
template<typename T> struct GetValueType { static_assert(true, "Unsupported type"); };
template<> struct GetValueType<nullptr_t> { static constexpr auto value = RawValueType::Null; };
template<> struct GetValueType <bool> { static constexpr auto value = RawValueType::Boolean; };
template<> struct GetValueType <IntValue> { static constexpr auto value = RawValueType::Integer; };
template<> struct GetValueType <std::shared_ptr<StringValue>> { static constexpr auto value = RawValueType::String; };
template<> struct GetValueType <ObjPtr<IObject>> { static constexpr auto value = RawValueType::Object; };
template<> struct GetValueType <std::shared_ptr<BasicDictionary>> { static constexpr auto value = RawValueType::Dictionary; };
template<> struct GetValueType <std::shared_ptr<BasicSequence>> { static constexpr auto value = RawValueType::Sequence; };
template<> struct GetValueType <std::shared_ptr<IDataProxy>> { static constexpr auto value = RawValueType::DataProxy; };

//
// Internal object to store values (boost::variant wrapper)
//
struct Value
{
public:
	using Type = RawValueType;

private:
	
	// Fictive value keeper for null-type
	struct NullValue
	{
		nullptr_t Value;
	};

	// Basic data type
	typedef boost::variant < 
		NullValue, 
		bool, 
		IntValue, 
		std::shared_ptr<StringValue>, 
		ObjPtr<IObject>, 
		std::shared_ptr<BasicDictionary>, 
		std::shared_ptr<BasicSequence>, 
		std::shared_ptr<IDataProxy>
	> VariantType;


	VariantType m_Variant;

public:

	//
	// Constructors, destructor, assignments
	//
	
	Value() = default;
	Value(const Value&) = default;
	Value& operator= (const Value&) = default;
	~Value() = default;

	//
	// Move constructor moves data and reset type as null
	//
	Value(Value&& other) noexcept
	{
		*this = std::move(other);
	}

	//
	// Move assignment moves data and reset type as null
	//
	Value& operator=(Value&& other) noexcept
	{
		m_Variant = std::move(other.m_Variant);
		other.set(nullptr);
		return *this;
	}

	//
	// Get type
	//
	RawValueType getType() const
	{
		return static_cast<RawValueType> (m_Variant.which());
	}

	//
	// Setters/Getters
	//


	//
	// Default putter. T must be one of supported types
	//
	template<typename T, std::enable_if_t< boost::mpl::contains<VariantType::types, T>::value, int> = 0 >
	void set(const T& Value)
	{
		m_Variant = Value;
	}

	//
	// null putter
	//
	template<typename T, std::enable_if_t< std::is_same_v<T, nullptr_t>, int> = 0 >
	void set(T /*Value*/)
	{
		m_Variant = NullValue();
	}

	//
	// Default safe getter. Returns pointer to internal storage
	// if variant contains other type, return nullptr
	// 
	template<typename T>
	const T* getSafe() const
	{
		static_assert(boost::mpl::contains<VariantType::types, T>::value, "Type 'T' is not supported");
		return boost::get<T>(&m_Variant);
	}

	//
	// null safe getter
	//
	template<>
	const nullptr_t* getSafe<nullptr_t>() const
	{
		const NullValue* pResult = boost::get<NullValue>(&m_Variant);
		if (pResult == nullptr)
			return nullptr;
		return &(pResult->Value);
	}

	//
	// Unsafe getter. If variant contains other type, throw TypeError
	// 
	template<typename T>
	T get() const
	{
		const T* OptValue = getSafe<T>();
		if (OptValue == nullptr)
			error::TypeError(SL, FMT("Can't get " << GetValueType<T>::value <<
				" value from Variant with " << getType())).throwException();
		return *OptValue;
	}
};


} // namespace detail
} // namespace variant
} // namespace cmd
