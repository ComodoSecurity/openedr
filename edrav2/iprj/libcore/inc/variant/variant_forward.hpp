//
// edrav2.libcore project
//
// This separation of declaration and implementation is needed 
// because of recursive dependences.
//
// Author: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (10.12.2018)
//
///
/// @file Variant type declaration
///
/// @addtogroup basicDataObjects Basic data objects
/// TODO: Brief description place here
/// @{

#pragma once
#include "forward.hpp"
#include "detail/value.hpp"

namespace cmd {
namespace variant {
namespace detail {

//
// Internal variant creator
// It supports creation from BasicDictionary and BasicSequence
// Caller must pass types only supported by Value
//
template <typename T>
inline Variant createVariant(const T& Val);

//
// Internal variant converter
// It supports conversion to all types supported Value
// including BasicDictionary, BasicSequence, DataProxy
//
template <typename T>
inline std::optional<T> convertVariantSafe(const Variant& vThis);

//
// Get variant raw type
// Support return _DataProxy
//
inline RawValueType getRawType(const Variant& vThis);

//
// Variant self cloning
// If type is clonable, it clones and reassigns Variant value.
// else Variant is not changed (already cloned)
//
inline void cloneContainer(Variant& v, bool fResolveProxy);

//
// Resolves IDataProxy-es in Variant, replaces with their values
//
inline void resolveDataProxy(Variant& v, bool fRecursive);

//
// Create begin iterator for Dictionary
//
ConstDictionaryIterator createDictionaryIterator(const Variant& vVal, bool fUnsafe);

//
// Variant merging implementation
//
Variant merge(Variant vDst, const Variant vSrc, MergeMode eMode);

///
/// Prints Variant value into human-readable utf8 string.
///
/// @param v - value for output.
/// @param nIndent [opt] - indent level for nested values (default: 1).
/// @param fQuoted [opt] - use std::quoted for string (default: false).
/// @returns 
///
std::string print(const Variant& v, Size nIndent = 1, bool fQuoteString = false);

} // namespace detail


//
// Variant type
//
class Variant
{
public:
	using IntValue = detail::IntValue;
	using StringValuePtr = std::shared_ptr<detail::StringValue>;
	using ValueType = ValueType;
	using MergeMode = MergeMode;

	using SequenceIndex = SequenceIndex;
	using DictionaryIndex = DictionaryIndex;

private:

	// Friends
	template <typename T>
	friend Variant detail::createVariant(const T& Val);
	template <typename T>
	friend std::optional<T> detail::convertVariantSafe(const Variant& vThis);
	friend void detail::cloneContainer(Variant& v, bool fResolveProxy);
	friend void detail::resolveDataProxy(Variant& v, bool fRecursive);

	friend ConstDictionaryIterator detail::createDictionaryIterator(const Variant& vVal, bool fUnsafe);
	friend detail::RawValueType detail::getRawType(const Variant& vThis);


	using RawValueType = detail::RawValueType;
	using BasicDictionaryPtr = std::shared_ptr<detail::BasicDictionary>;
	using BasicSequencePtr = std::shared_ptr<detail::BasicSequence>;
	using Value = detail::Value;
	using DataProxyPtr = std::shared_ptr<detail::IDataProxy>;

	template<typename T, typename Result = void>
	using IsNullType = IsSameType<T, nullptr_t, Result>;
	template<typename T, typename Result = void>
	using IsBoolType = IsSameType<T, bool, Result>;
	template<typename T, typename Result = void>
	using IsIntType = std::enable_if_t<IntValue::IsSupportedType<T>::value, Result>;

	// Internal value
	Value m_Value;

	//
	// Internal converter to Dictionary (valid only if type is Dictionary)
	//
	BasicDictionaryPtr getDict() const
	{
		return m_Value.get<BasicDictionaryPtr>();
	}

	//
	// Internal converter to Sequence (valid only if type is Sequence)
	//
	BasicSequencePtr getSeq() const
	{
		return m_Value.get<BasicSequencePtr>();
	}

	//
	// Resolves DataProxy type.
	// Calculates its value and assign the result value to the current Variant('this').
	// Attention! The current Variant will change its value.
	//
	void resolveDataProxy() const;


	// Do action with dictionary like object
	// VariantPtr passes as parameter to support const and not const methods
	template<typename VariantPtr, typename Fn>
	static auto callDictionaryLike(VariantPtr pThis, Fn fn);

	// Do action with sequence like object
	// VariantPtr passes as parameter to support const and not const methods
	template<typename VariantPtr, typename Fn>
	static auto callSequenceLike(VariantPtr pThis, Fn fn);

	//
	// Getters/setters
	//

	//
	// Return variant value as type T. Caller must pass supported type
	// if variant value has different type then return empty value
	//
	// TODO: Process DataProxy
	//
	template<typename T>
	std::optional<const T> getValueSafe() const
	{
		if (m_Value.getType() == RawValueType::DataProxy)
			resolveDataProxy();

		const T* pValue = m_Value.getSafe<T>();
		if (pValue == nullptr)
			return {};
		return *pValue;
	}

	//
	// getValueSafe to extract IDataProxy.
	//
	template<>
	std::optional<const DataProxyPtr> getValueSafe<DataProxyPtr>() const
	{
		const DataProxyPtr* pValue = m_Value.getSafe<DataProxyPtr>();
		if (pValue == nullptr)
			return {};
		return *pValue;
	}

	//
	// Return variant value as type T. Caller must pass supported type
	// if variant value has different type, throw TypeError
	//
	template<typename T>
	T getValue() const
	{
		if (m_Value.getType() == RawValueType::DataProxy)
			resolveDataProxy();

		return m_Value.get<T>();
	}

	//
	// Set variant value (replace old value)
	// Caller must pass supported type
	//
	template<typename T>
	void setValue(const T& Value)
	{
		m_Value.set(Value);
	}

	//
	// Iterators
	//

	ConstSequenceIterator createSequenceIterator(bool fUnsafe = false) const;
	ConstDictionaryIterator createDictionaryIterator(bool fUnsafe = false) const;

public:
	//
	// Default constructors/assign/destructor
	//

	Variant() = default;
	Variant(const Variant&) = default;
	Variant& operator= (const Variant&) = default;
	Variant(Variant&& other) noexcept = default;
	Variant& operator= (Variant&& other) noexcept = default;
	~Variant() = default;

	//
	// Variant deep compare
	// Object-type comparison is not supported (always false)
	//
	bool operator==(const Variant& other) const;

	//
	// "Not equal" operator for all types
	//
	template <typename T>
	bool operator!=(const T& Value) const
	{
		return !(*this == Value);
	}

	//
	// Constructors/assignments/comparison/type conversions
	// For Null type
	//

	template<typename T, IsNullType<T, int> = 0 >
	Variant(const T& Value)
	{
		setValue(Value);
	}
	
	template<typename T, typename = IsNullType<T>>
	Variant& operator=(const T& Value)
	{
		setValue(Value);
		return *this;
	}

	bool isNull() const
	{
		return getType() == ValueType::Null;
	}

	bool isRawNull()
	{
		return m_Value.getType() == detail::RawValueType::Null;
	}

	bool isDataProxy()
	{
		return m_Value.getType() == detail::RawValueType::DataProxy;
	}

	template<typename T, IsNullType<T, int> = 0>
	bool operator==(const T& Value) const;

	template<typename T, IsNullType<T, int> = 0>
	operator T() const
	{
		return getValue<nullptr_t>();
	}

	//
	// Constructors/assignments/comparison/type conversions
	// For Boolean type
	//

	template<typename T, IsBoolType<T, int> = 0 >
	Variant(const T& Value)
	{
		setValue(Value);
	}
	
	template<typename T, IsBoolType<T, int> = 0>
	Variant& operator=(const T& Value)
	{
		setValue(Value);
		return *this;
	}

	template<typename T, IsBoolType<T, int> = 0>
	operator T() const
	{
		return getValue<bool>();
	}

	template<typename T, IsBoolType<T, int> = 0>
	bool operator==(const T& Value) const;

	//
	// Constructors/assignments/comparison/type conversions
	// For Integer type
	//

	template<typename T, IsIntType<T, int> = 0 >
	Variant(const T& Value)
	{
		setValue(IntValue(Value));
	}
	
	template<typename T, IsIntType<T, int> = 0>
	Variant& operator=(T Value)
	{
		setValue(IntValue(Value));
		return *this;
	}
	
	Variant& operator=(const IntValue& Value)
	{
		setValue(Value);
		return *this;
	}

	//
	// Type casts for supported types
	// throws error::TypeError if type is wrong
	// Example: uint64_t x = v;
	//
	template<typename T, IsIntType<T, int> = 0>
	operator T() const
	{
		return static_cast<T>(getValue<IntValue>());
	}

	operator IntValue() const
	{
		return getValue<IntValue>();
	}

	template<typename T, IsIntType<T, int> = 0>
	bool operator==(const T& Value) const;

	//
	// Constructors/assignments/comparison/type conversions
	// For String type

	// NOTE: We cannot leave only std::string_view-based statements,
	// and get rid of 'const char*' and 'const std::string&',
	// because only one implicit user conversion is allowed by C++

	Variant(std::string_view sValue)
	{
		setValue(detail::StringValue::create(sValue));
	}

	Variant(const char* sValue)
	{
		setValue(detail::StringValue::create(sValue));
	}

	Variant(const std::string& sValue)
	{
		setValue(detail::StringValue::create(sValue));
	}

	Variant(std::wstring_view sValue)
	{
		setValue(detail::StringValue::create(sValue));
	}

	Variant(const wchar_t* sValue)
	{
		setValue(detail::StringValue::create(sValue));
	}
	
	Variant(const std::wstring& sValue)
	{
		setValue(detail::StringValue::create(sValue));
	}

	operator StringValuePtr() const
	{
		return getValue<StringValuePtr>();
	}

	Variant& operator=(const std::string& sValue)
	{
		setValue(detail::StringValue::create(sValue));
		return *this;
	}

	Variant& operator=(std::string_view sValue)
	{
		setValue(detail::StringValue::create(sValue));
		return *this;
	}

	Variant& operator=(const char* sValue)
	{
		setValue(detail::StringValue::create(sValue));
		return *this;
	}

	Variant& operator=(const std::wstring& sValue)
	{
		setValue(detail::StringValue::create(sValue));
		return *this;
	}

	Variant& operator=(std::wstring_view sValue)
	{
		setValue(detail::StringValue::create(sValue));
		return *this;
	}

	Variant& operator=(const wchar_t* sValue)
	{
		setValue(detail::StringValue::create(sValue));
		return *this;
	}

	operator const std::string_view() const
	{
		return std::string_view(*getValue<StringValuePtr>());
	}

	operator std::string() const
	{
		return std::string(std::string_view(*getValue<StringValuePtr>()));
	}

	operator const std::wstring_view() const
	{
		return std::wstring_view(*getValue<StringValuePtr>());
	}

	operator std::wstring() const
	{
		return std::wstring(std::wstring_view(*getValue<StringValuePtr>()));
	}

	bool operator==(const char* sValue) const
	{
		return operator==(std::string_view(sValue));
	}

	bool operator==(const std::string& sValue) const
	{
		return operator==(std::string_view(sValue));
	}

	bool operator==(std::string_view sValue) const;

	bool operator==(const wchar_t* sValue) const
	{
		return operator==(std::wstring_view(sValue));
	}

	bool operator==(const std::wstring& sValue) const
	{
		return operator==(std::wstring_view(sValue));
	}

	bool operator==(std::wstring_view sValue) const;

	//
	// Constructors/assignments/comparison/type conversions
	// For Object type
	//

	template<typename I>
	Variant(const ObjPtr<I>& Value)
	{
		setValue(ObjPtr<IObject>(Value));
	}

	template<typename I>
	Variant& operator=(const ObjPtr<I>& Value)
	{
		setValue(ObjPtr<IObject>(Value));
		return *this;
    }

	operator ObjPtr<IObject>() const
	{
		return getValue<ObjPtr<IObject>>();
	}

	template<typename I>
	operator ObjPtr<I>() const
	{
		return queryInterface<I>(getValue<ObjPtr<IObject>>());
	}

	bool isObject() const
	{
		return getType() == ValueType::Object;
	}

	bool isString() const
	{
		return getType() == ValueType::String;
	}

	//
	// Common functions
	//

	//
	// Converts variant to std path
	//
	std::filesystem::path convertToPath() const
	{
		// Detect best string_view type
		using PathCharT = std::filesystem::path::string_type::value_type;
		using PathStringView = std::basic_string_view<PathCharT>;

		return std::filesystem::path(PathStringView(*getValue<StringValuePtr>()));
	}

	//
	// Returns true if Variant can behave like Dictionary.
	//  * Variant contains BasicDictionary or 
	//  * Variant contains Object implementing the `IDictionaryProxy` interface
	//
	bool isDictionaryLike() const;

	//
	// Returns true if Variant can behave like Sequence.
	//  * Variant contains BasicSequence or 
	//  * Variant contains Object implementing the `ISequenceProxy` interface
	//
	bool isSequenceLike() const;

	// Get the type of underlying variable
	ValueType getType() const
	{
		auto eType = m_Value.getType();
		if (eType != RawValueType::DataProxy)
			return (ValueType)eType;

		resolveDataProxy();
		return (ValueType)m_Value.getType();
	}

	// Return number of elements in container
	Size getSize() const;
	
	// Resize Sequence. If getSize grows, null elements is added.
	void setSize(Size nNewSize);
	
	// Check container emptiness.
	bool isEmpty() const;
	
	///
	/// Remove all elements from container.
	///
	void clear();
	
	///
	/// Deep cloning
	///
	Variant clone(bool fResolveProxy = false) const;
	
	///
	/// Print value as humanreadable utf8 string
	///
	std::string print() const
	{
		return detail::print(*this);
	}


	///
	/// Merging content of other Variant (vSrc) into this Variant (`this`).
	///
	/// It is primarily intended to merging of Dictionaries or Sequences.
	/// Recursive merging also provided.
	/// 
	/// **Merge rules**
	///
	/// The function supports different cases:
	/// * if `vSrc` is null, null merging is applied.
	/// * if `this` and `vSrc` are dictionary-like, dictionary merging is applied.
	/// * if `this` and `vSrc` are sequence-like, sequence merging is applied.
	/// * otherwise, scalar merging is applied.
	/// 
	/// **Null merging**
	///
	/// If `vSrc` is null, merge() sets `this` to null. The CheckType flag is not lead to exception in this case.
	///
	/// **Dictionary merging:**
	/// Elements of vSrc are inserted into `this`.  
	/// The MergeMode parameter tunes behavior:
	///   * MergeMode::New and MergeMode::Exist specify which element will be inserted. If both are absent, error::InvalidArgument will be thrown.
	///   * MergeMode::MergeNestedDict and MergeMode::MergeNestedSeq enable recursive merging. If both are absent all inserted elements will be replaced without merging.
	///   * The MergeMode::CheckType flag denies replacement/merging elements with different types (error::InvalidArgument will be thrown)
	/// Null-value element allow to erace data. If merge() meets Null-value element in vSrc, it remove the
	/// element with the same name from `this`. The MergeMode::NonErasingNulls flag disables this behaviour.
	/// 
	/// **Sequence merging:**
	/// Elements of vSrc are appended into `this`.
	/// 
	/// **Scalar merging:**
	/// merge() just sets `this` to vSrc.
	/// if eMode contains CheckType, exception error::InvalidArgument is thrown in following cases:
	///   * `this` and vSrc have different types; 
	///   * `this` is dictionaryLike but vSrc is not dictionaryLike; 
	///   * `this` is sequenceLike but vSrc is not sequenceLike; 
	/// 
	/// @param vSrc Data for adding.
	/// @param eMode Merging mode.
	/// @throw error::InvalidArgument (see above).
	/// @return reference to self. It is helpful for processing of clones (see "Basic usage") 
	///
	/// Basic usage:
	/// @code {.cpp}
	/// // Source data
	/// Variant vDst = Dictionary({{"a",1}, {"b", 2}});
	/// Variant vSrc = Dictionary({{"b",100}, {"c", 3}});
	///
	/// vDst.merge(vSrc, MergeMode::All);
	/// 
	/// REQUIRE(vDst == Dictionary({{"a",1}, {"b", 100}, {"c", 2}}));
	/// @endcode
	///
	/// If you don't want change vDst, do following:
	/// @code {.cpp}
	/// // merge() return the same, but does not change vDst
	/// Variant vRes = vDst.clone().merge(vSrc, MergeMode::All);
	/// @endcode
	///
	Variant& merge(Variant vSrc, MergeMode eMode);

	// Check element existence in dictionary 
	bool has(DictionaryIndex id) const;

	// Remove element by ID. It returns number of removed elements.
	Size erase(DictionaryIndex id);
	
	// Remove element by ID. It returns number of removed elements.
	Size erase(SequenceIndex id);

	// Get element by ID. It throws exception, if the element is not exist. 
	Variant get(DictionaryIndex id) const;
	
	// Get element by ID. It throws exception, if the element is not exist. 
	Variant get(SequenceIndex id) const;
	
	// Get element by ID. It returns vDefaultValue, if the element is not exist. 
	Variant get(DictionaryIndex id, const Variant& vDefaultValue) const;
	
	// Get element by ID. It returns vDefaultValue, if the element is not exist. 
	Variant get(SequenceIndex id, const Variant& vDefaultValue) const;

	// Get element by ID. It returns std::nullopt, if the element is not exist. 
	std::optional<Variant> getSafe(DictionaryIndex id) const;

	// Get element by ID. It returns std::nullopt, if the element is not exist.
	std::optional<Variant> getSafe(SequenceIndex id) const;


	//
	// Get element by ID. It throws exception, if the element is not exist.
	//
	const Variant operator[](SequenceIndex id) const
	{
		return get(id);
	
	}
	const Variant operator[](DictionaryIndex id) const
	{
		return get(id);
	}

#ifdef __INTELLISENSE__
	const Variant operator[](const char *) const {}
#endif	

	// Set new/existing element by ID
	void put(DictionaryIndex id, const Variant& vValue);
	
	// Set existing element by ID (don't change getSize)
	void put(SequenceIndex id, const Variant& vValue);
	
	// Append element into sequence
	void push_back(const Variant& vValue);
	
	// Append element into sequence
	Variant& operator+=(const Variant& vValue);
	
	// Insert element before index Id into Sequence.
	void insert(SequenceIndex Id, const Variant& vValue);

	//
	// Value iteration (safe, readonly)
	//
	ConstSequenceIterator begin() const;
	ConstSequenceIterator end() const;

	//
	// Value iteration (unsafe, readonly)
	//
	ConstSequenceIterator unsafeBegin() const;
	ConstSequenceIterator unsafeEnd() const;
	auto getUnsafeIterable() const;

	//
	// Creates iterator.
	//
	std::unique_ptr<IIterator> createIterator(bool fUnsafe) const;
};

//////////////////////////////////////////////////////////////////////////
//
// 
//

///
/// Creates Variant with DataProxy for a command call (lazily calculated Variant)
/// @param vCmd - command descriptor or ICommand object
/// @param fCached - cache a result
///
/// The command will be called with null parameter.
/// 
/// Cache behavior.
/// DataProxy Variant calculate on data access. Coping does not lead to calculation.
/// If fCached is true, the value will be calculated once on first access (even with cloning). 
/// A cached value will be returned each next access
/// If fCached is false, the value will be calculated on each access. 
/// But if Variant with DataProxy was calculated, Its content will be replaced with this value.
///
extern Variant createCmdProxy(Variant vCmd, bool fCached);

///
/// Creates Variant with DataProxy for object creation (lazily calculated Variant)
/// @param vObj - object descriptor (with the 'clsid' field and finalConstruct parameters)
/// @param fCached - cache a result. See createCmdProxy
///
/// The command will be called with null parameter.
/// 
extern Variant createObjProxy(Variant vObj, bool fCached);

///
/// Creates Variant with DataProxy from lambda or any c++ functor (lazily called lambda)
/// @param fnCalculator - functor with signature "Variant fn()")
/// @param fCached - cache a result. See createCmdProxy
///
extern Variant createLambdaProxy(DataProxyCalculator fnCalculator, bool fCached);


//////////////////////////////////////////////////////////////////////////
//
// Access to variant by path
//
// Path allows access into deep of multilevel Variant (containing nested dictionaries and sequence)
// Path example:
//   "key1.key2[10].key3[0][1]"
// The same as on C++ ["key"]["key2"][10]["key3"][0][1]
//

///
/// Returns element by path
///
/// @param vRoot - Variant to extract data. Root of data.
/// @param sPath - Path to extract data.
/// @return Extracted data.
/// @throw error::OutOfRange if element not found
/// @throw error::TypeError if path contains not dictionary/sequence element
///
extern Variant getByPath(Variant vRoot, const std::string_view sPath);


///
/// Returns element by path
///
/// If element not found or path contains not dictionary/sequence element, returns vDefault
///
/// @param vRoot - Variant to extract data. Root of data.
/// @param sPath - Path to extract data.
/// @param vDefault - default data
/// @return Extracted data or vDefault
///
extern Variant getByPath(Variant vRoot, const std::string_view sPath, Variant vDefault);

///
/// Returns element by path
///
/// If element not found or path contains not dictionary/sequence element, returns std::nullopt.
///
/// @param vRoot - Variant to extract data. Root of data.
/// @param sPath - Path to extract data.
/// @return Extracted data or nullopt
///
extern std::optional<Variant> getByPathSafe(Variant vRoot, const std::string_view& sPath);

///
/// Puts data to a data packet
///
/// If last element of path does not exist, function create if it possible.
///
/// @param vRoot a Variant value with the target data packet
/// @param sPath a path where the data will be placed to
/// @param vValue a Variant value with data to put
/// @param fCreatePaths (Optional) Data to add
/// @return Extracted data or vDefault
/// @throw error::TypeError if path (except last element) contains not dictionary/sequence element
/// @throw error::OutOfRange If path (except last element) does not exist
///
extern void putByPath(Variant vRoot, const std::string_view& sPath, Variant vValue);
extern void putByPath(Variant vRoot, const std::string_view& sPath, Variant vValue, bool fCreatePaths);

//
// Delete element by path.
// If path (except last element) does not exist, throws error::OutOfRange.
//
extern void deleteByPath(Variant vRoot, const std::string_view& sPath);

///
/// Output of humanreadable representation
///
inline std::ostream& operator<<(std::ostream& oStrStream, const Variant& vVal)
{
	oStrStream << detail::print(vVal);
	return oStrStream;
}

//////////////////////////////////////////////////////////////////////////
//
// Object serialization/deserialization
//
//////////////////////////////////////////////////////////////////////////

///
/// Conversion of object into serializable `Variant`.
/// If it is impossible, throw exception error::InvalidArgument
///
/// The function process 2 types of objects:
/// * Implementing the ISerializable interface. It return result of ISerializable::serialize.
/// * Implementing the IReadableStream interface. Create base64 representation of data and descriptor suitable stream
///
/// @sa cmd::variant::json::serializeStream
///
Variant serializeObject(Variant vSrc);

///
/// Creation of IObject during deserialization.
/// If vSrc is Dictionary and contain "$$proxy", 
/// * create DataProxy object.
/// * Support following valid values
///   * "obj" - returns DataProxy creating object without caching. vSrc should contain object descriptor with the 'clsid' field.
///   * "cmd" - returns DataProxy calling command without caching. vSrc should contain command descriptor.
///   * "cachedCmd" - Same as "cmd" but with caching.
///   * "cachedObj" - Same as "obj" but with caching.
///
/// If vSrc is dictionary and contain "$$clsid", 
/// * create object with this clsid and vSrc as parameter.
/// * return new object
/// Otherwise return vSrc
/// If fRecursively is true, parse vSrc recursively
///
Variant deserializeObject(Variant vSrc, bool fRecursively = false);

//////////////////////////////////////////////////////////////////////////
//
// Operations
//
//////////////////////////////////////////////////////////////////////////

namespace operation {

///
/// Perform a logical NOT operation (!)
///
/// @param vValue the source Variant value
/// @return Returns true if vValue is false, false otherwise.
///
bool logicalNot(Variant vValue);

///
/// Perform a logical AND comparison operation (&&)
///
/// This operation is used to perform a logical AND comparison between two value.
/// The order of parameters doesn't matter.
///
/// @param vValue1 a Variant value to compare
/// @param vValue2 a Variant value to compare
/// @return Returns true if both vValue1 and vValue2 values are true, false otherwise.
///
bool logicalAnd(Variant vValue1, Variant vValue2);

///
/// Perform a logical OR comparison operation (||)
///
/// This operation is used to perform a logical OR comparison between two value.
/// The order of parameters doesn't matter.
///
/// @param vValue1 a Variant value to compare
/// @param vValue2 a Variant value to compare
/// @return Returns true if vValue1 or vValue2 value is tru, false otherwise.
///
bool logicalOr(Variant vValue1, Variant vValue2);

///
/// Perform an equality comparison operation (==)
///
/// This relational operation is used to check if one value is equal to another one.
/// The order of parameters doesn't matter.
///
/// @param vValue1 a Variant value to compare
/// @param vValue2 a Variant value to compare
/// @return Returns true if vValue1 is equal to vValue2, false otherwise.
///
bool equal(Variant vValue1, Variant vValue2);

///
/// Perform an integer comparison operation (>)
///
/// This relational operator is used to check if one integer value is greater than another one.
///
/// @param vValue1 a Variant value to compare
/// @param vValue2 a Variant value to compare
/// @return Returns true if vValue1 is greater than vValue2, false otherwise.
///
bool greater(Variant vValue1, Variant vValue2);

///
/// Perform an integer comparison operation (<)
///
/// This relational operator is used to check if one integer value is less than another one.
///
/// @param vValue1 a Variant value to compare
/// @param vValue2 a Variant value to compare
/// @return Returns true if vValue1 is less than vValue2, false otherwise.
///
bool less(Variant vValue1, Variant vValue2);

///
/// Perform an regex-matching operation
///
/// This operation is used to check if value matches a regex-pattern.
///
/// @param vValue the source Variant value
/// @param vPattern the regex-pattern value
/// @param fCaseInsensitive use case insensitive matching
/// @return Returns true if a match exists, false otherwise.
///
bool match(Variant vValue, Variant vPattern, bool fCaseInsensitive = false);

///
/// Perform an inclusion matching operation
///
/// This operation is used to check if one value contains another one.
///
/// @param vValue1 a Variant value to compare
/// @param vValue2 a Variant value to compare
/// @return Returns true if vValue1 is contain vValue2, false otherwise.
///
bool contain(Variant vValue1, Variant vValue2);

///
/// Performs the logical addition of data in terms of variant logical operations
///
/// For more information, see
/// https://intranet.nurd.com/display/DEV/EDRAv2+Core+Library+Architecture#EDRAv2CoreLibraryArchitecture-Add
///
/// @param vValue1 a Variant value
/// @param vValue2 a Variant value
/// @return Returns a Variant value with result of addition of vValue1 and vValue2.
///
Variant add(Variant vValue1, Variant vValue2);

///
/// Extracts data from data packet by specified path
///
/// @param vData a Variant value with soutce data packet
/// @param sPath the string with path which points to an item in vValue
/// @return Returns a Variant value with extracted data.
///
Variant extract(Variant vData, std::string_view sPath);

//
//
//
enum class TransformSourceMode
{
	ChangeSource, ///< Make changes in source data
	CloneSource, ///< Clone the source
	ReplaceSource ///< Don't copy non-transformed data from source
};

///
/// Performs transformation of dictionary according to specified schema
///
/// Transformation schema (vSchema) contains one or more transformatio rules.
/// Each rule is a dictionary with the following fields:
///   * **item** - a path to the item in the source dictionary
///   * **value** - (Optional) a explicit value which will be written to the
///     result dictionary
///   * **localPath** - (Optional) a path of the item in the source dictionary
///     which will be written to the result dictionary
///   * **catalogPath** - (Optional) a path of the item in the global catalog
///     which will be written to the result dictionary
///   * **command** - (Optional) a command the result of which will be written
///     to the result dictionary
/// One of the optional value parameters (value, localPath or command) must be
/// supplied. If rule has more than one parameter the one to apply will be chosen
/// according to the following priority order: value -> localPath -> command.
/// In order to delete some item from the source dictionary the null-value shall
/// to be used.
///
/// @param vData a dictionary with source data
/// @param vSchema a dictionary (one rule) or a sequence of dictionaries (set of rules)
/// @eMode eMode - Source data transformation mode (see TransformSourceMode)
/// @return Returns the result dictionary.
///
Variant transform(Variant vData, Variant vSchema, TransformSourceMode eMode);

///
/// Replace all occurrences of substring
///
/// Replacement schema (vSchema) contains one or more replacement rules.
/// Each rule is a dictionary with the following fields:
///   * **target** - a value to be searched in vValue
///   * **value** - a value which is used to replace all found occurences
///   * **mode** - (Optional) flags from MatchMode
/// vValue can be a string or a sequence. If vValue is a sequence, its only 
/// string inner items is affected, while all non-string values stay unchanged.
///
/// @param vValue the source Variant value which the operation will be performed on
/// @param vSchema a dictionary (one rule) or a sequence of dictionaries (set of rules)
/// which contains the replacement schema or StringMatcher object
/// @return Returns the Variant value with all replacements.
/// @throw error::InvalidArgument if vValue is not a string or sequence,
/// or vSchema has an incorrect format.
///
Variant replace(Variant vValue, Variant vSchema);

///
/// Matching mode
///
CMD_DEFINE_ENUM_FLAG(MatchMode)
{
	Begin = 1 << 0, ///< search in the beginning of the string
	End = 1 << 1, ///< search in the ending of the string
	All = 1 << 2, ///< search in the entire string
};

} // namespace operation

// Base generic template for Variant convertion
template<ValueType type>
inline Variant convert(Variant vValue) { static_assert(false, "Unsupported convert type"); }

///
/// Variant logical (explicit) conversion
///
/// For more information, see
/// https://intranet.nurd.com/display/DEV/EDRAv2+Core+Library+Architecture#EDRAv2CoreLibraryArchitecture-Logicalconversions
///
/// @param vValue the source Variant value
/// @return Returns the converted Variant value.
///
template<> Variant convert<ValueType::Boolean>(Variant vValue);
template<> Variant convert<ValueType::Integer>(Variant vValue);
template<> Variant convert<ValueType::String>(Variant vValue);
template<> Variant convert<ValueType::Dictionary>(Variant vValue);
template<> Variant convert<ValueType::Sequence>(Variant vValue);

} // namespace variant


//////////////////////////////////////////////////////////////////////////
//
// SFINAE type checker
//

//
// Check is Variant
//
template<typename T, typename Result = void>
using IsVariantType = IsSameType<T, Variant, Result>;


} // namespace cmd
/// @}
