//
//  EDRAv2. libcore
//    Variant. Forward declarations
//
#pragma once

namespace cmd {
namespace variant {

class Variant;
class Sequence;
class Dictionary;

class ConstSequenceIterator;
class ConstDictionaryIterator;


typedef std::size_t SequenceIndex;
typedef std::string_view DictionaryIndex;

// use std::string_view as reference to std::string
// because it gives opportunity not to create std::string
// when string passed as parameter
typedef std::string_view DictionaryKeyRefType;

/// 
/// @brief Enum of supported Variant's types
///
enum class ValueType : std::uint8_t
{
	Null = 0,				///< null value
	Boolean = 1,			///< boolean value
	Integer = 2,			///< integer value up to 64-bit (signed or unsigned)
	String = 3,				///< string value
	Object = 4,				///< pointer to BaseObject
	Dictionary = 5,			///< dictionary (unordered set of name/value pairs)
	Sequence = 6,			///< sequence (ordered collection of values)
};

//
//
//
inline std::ostream& operator<<(std::ostream& oStrStream, const ValueType eVal)
{
	switch (eVal)
	{
		case ValueType::Null: oStrStream << "Null"; break;
		case ValueType::Boolean: oStrStream << "Boolean"; break;
		case ValueType::Integer: oStrStream << "Integer"; break;
		case ValueType::String: oStrStream << "String"; break;
		case ValueType::Object: oStrStream << "Object"; break;
		case ValueType::Dictionary: oStrStream << "Dictionary"; break;
		case ValueType::Sequence: oStrStream << "Sequence"; break;
		default: oStrStream << "INVALID_VALUE"; break;
	}
	return oStrStream;
}


// TODO: check doxygen generated doc for MergeMode
///
/// @brief Settings (flags) of cmd::variant::merge()
///
CMD_DEFINE_ENUM_FLAG(MergeMode)
{
	New = 1 << 0, ///< dictionary merging puts elements from vSrc into vDst which have names nonexistent in vDst
	Exist = 1 << 1, ///< dictionary merging puts elements from vSrc into vDst which have names existent in vDst
	All = New | Exist, ///< Merge all elements in dictionaries (new and exist)

	CheckType = 1 << 2, ///< Merge elements only if they have the same type

	MergeNestedDict = 1 << 3, ///< Recursive call merge() for dictionaries nested in dictionary
	MergeNestedSeq = 1 << 4, ///< Recursive call merge() for sequences nested in dictionary

	NonErasingNulls = 1 << 5, ///< Null-values in vSrc doesn't erase the same element in vDst
};


///
/// Base Interface for variant values iteration
///
class IIterator
{
public:
	virtual ~IIterator() {};

	/// Go to the next element
	virtual void goNext() = 0;
	
	/// Checking the end of iteration
	virtual bool isEnd() const = 0;
	
	/// Returns key of current element (for Dictionary only)
	virtual DictionaryKeyRefType getKey() const = 0;
	
	/// Returns the value of current element
	virtual const Variant getValue() const = 0;
};

//
// Interface provides Dictionary-like behavior
//
class IDictionaryProxy : OBJECT_INTERFACE
{
public:

	//
	// Get number of elements
	//
	virtual Size getSize() const = 0;

	//
	//
	//
	virtual bool isEmpty() const = 0;

	//
	// Check existence element with specified name
	//
	virtual bool has(variant::DictionaryKeyRefType sName) const = 0;

	//
	// Remove element with specified name
	// Return number of removed elements
	//
	virtual Size erase(variant::DictionaryKeyRefType sName) = 0;

	//
	//
	//
	virtual void clear() = 0;

	//
	// Returns element with specified name
	// If element does not exist or error occurred, 
	// the method throws appropriate error
	//
	virtual Variant get(variant::DictionaryKeyRefType sName) const = 0;

	//
	// Returns element with specified name
	// If element does not exist or error occurred, 
	// return empty element
	//
	virtual std::optional<Variant> getSafe(variant::DictionaryKeyRefType sName) const = 0;

	//
	// Add element into container
	//
	virtual void put(variant::DictionaryKeyRefType sName, const Variant& Value) = 0;

	//
	// Return dictionary iterator
	// if fUnsafe == false, IIterator must be thread safe
	//
	virtual std::unique_ptr<variant::IIterator> createIterator(bool fUnsafe) const = 0;
};

//
// Sequence like behavior.
// Variant with Object implementing ISequenceProxy provides sequence methods
//
class ISequenceProxy : OBJECT_INTERFACE
{
public:
	//
	// return number of elements
	//
	virtual Size getSize() const = 0;

	//
	//
	//
	virtual bool isEmpty() const = 0;

	//
	// Remove element with specified index
	// Return number of removed elements
	//
	virtual Size erase(Size nIndex) = 0;

	//
	//
	//
	virtual void clear() = 0;

	//
	// Returns element with specified index
	// If element does not exist or error occurred, 
	// the method throws appropriate error
	//
	virtual Variant get(Size nIndex) const = 0;

	//
	// Returns element with specified index
	// If element does not exist or error occurred, 
	// return empty element
	//
	virtual std::optional<Variant> getSafe(Size nIndex) const = 0;

	//
	// Set element with specified index
	// Index should be valid (Index < Size)
	//
	virtual void put(Size nIndex, const Variant& Value) = 0;

	//
	// Append element
	//
	virtual void push_back(const Variant& vValue) = 0;

	//
	// Resize sequence
	//
	virtual void setSize(Size nSize) = 0;

	//
	// Insert new element into sequence.
	// After successful insertion new element should have specified index
	// it support index <= size. 
	// If Index == size, the method works like push_back
	//
	virtual void insert(Size nIndex, const Variant& vValue) = 0;

	//
	// Return sequence iterator
	// if fUnsafe == false, IIterator must be thread safe
	//
	virtual std::unique_ptr<variant::IIterator> createIterator(bool fUnsafe) const = 0;
};

///
/// Interface for cloning objects in Variant
///
class IClonable : OBJECT_INTERFACE
{
public:
	///	
	/// Returns an object's clone.
	/// \return Variant Cloned value.
	///
	virtual Variant clone() = 0;
};

///
/// Function to calculate inside data proxy
///
typedef std::function<Variant()> DataProxyCalculator;


} // namespace variant
} // namespace cmd

