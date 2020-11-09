# Variant Library

Variant is universal data storage for exchange information between c++ modules and classes.
For example, storing/accessing configuration data, C++ representation of data which is recieved from (sent to) a server.

Briefly, Variant is like JSON object which also can contain objects which implement the "Object" interface.
Also Variant has special support of objects with interfaces: VariantProxy, DictionaryProxy, SequenceProxy.
They provide behevour like corresponding type.

Variant can contain:

  * scalar types: integer, string, etc. (full list see below);
  * structural types: dictionary (`map<string, Variant>`), sequence (`vector<Variant>`);
  * pointer to Object interface: any class which implements Object interface.

So Variant supports to build complex hierarchy of Variants as it supports structural types.

Variant is developed for serialization(deserialization) into JSON and other formats. The serialization is implemented as an external feature.

The Variant library is inspired by the "JSON for Modern C++" library (https://github.com/nlohmann/json).

**cmd::Variant** - the main data container. It provides almost all external API.
First of all it is recommended to look at its functionality.

Simple examples:
```cpp
// Integer
Variant vInt = 10;
int i = vInt;
// String
Variant vStr = "Some utf8/ASCII string";    // Write UTF8
std::string_view sStr = vStr;               // Read UTF8
std::wstring_view sWStr = vStr;             // Read UTF16
// Sequence
Variant vSeq = Sequence();
vSeq.push_back("asd");
vSeq.push_back(100);
vSeq.push_back(nullptr);
vSeq.push_back(vInt);
printf("%d", (int) vSeq.getSize());
for(auto& Item : vSeq.seqIterable())
{
    printf("type: %d\n", (int) Item.getType());
}
// Dictionary
Variant vDict = Dictionary();
vDict.put("aaa", 10);
vDict.put("xxx", true);
vDict.put("yyy", vInt);
vDict.put("zzz", vSeq);
printf("%d", (int) vDict.getSize());
for(auto& Pair : Dictionary(vDict))
{
    printf("name: %s type: %d\n", Pair.first.c_str(), (int) Pair.second.getType());
}
assert(vDict["zzz"][1] == 100);
Variant vDict2 = Dictionary({ {"aaa", 10},  {"bbb", true} });
Variant vSeq2 = Dictionary({ 10, "bbb", true });
```

# Containers
## Container hierarchy
The library also has additional containers:

* External classes (external API):
    * **cmd::Variant** - main container;
    * **cmd::Sequence** - add-on object over cmd::Variant for convenient access to Sequence container;
    * **cmd::Dictionary** - add-on object over cmd::Variant for convenient access to Dictionary container.
* Internal classes (not use directly):
    * cmd::variant::detail::BasicDictionary - the basic implementation of dictionary;
    * cmd::variant::detail::BasicSequence - the basic implementation of sequence;
    * cmd::variant::detail::IntKeeper - the integer types keeper;
    * cmd::variant::detail::StrKeeper - the string types keeper;
    * cmd::variant::detail::VariantValueKeeper - internal keeper of all supported types.
    * cmd::variant::detail::, , , Object (DictionaryProxy, SequenceProxy, VariantProxy)

## cmd::Variant
cmd::Variant provides method/operator for:

  * assignment
  * keeping
  * accessing
  * iteration

of all data types which is supported by library.

### Constructors
Variant support copying and moving constructors and operator =.
Default constructor create null-type variant.

### Thread safity
Variant by itself is not thread safe.
But dictionary and sequence which are kept in Variant are thread safe.

You must not change Variant state from several threads without external synchrnization. 
But you can do multithreadly any actions which do not change Variant state:

* Read its data
* Change state of sequence or dictionary which is kept by Variant
* Change state of object which is kept by Variant if this object support multithread access

### Fast copyable
Variant is fast copyable. 
All complex data are stored by reference. And it copying is just copy of reference.

### Internal data types
Type of kept data is represented by enum cmd::Variant::ValueTypes

* **null** - null value (nullptr_t in c++)
* **boolean** - bool value
* **integer** - integer value up to 64-bit (signed or/and unsigned).
* **string** - string value in UTF8 or UTF16 encoding.
* **object** - any object implemented Object interface.
* **dictionary** - threadsafe `map<string, Variant>`.
* **sequence** - threadsafe `vector<Variant>`.

#### Type checking
Variant methods:

* **getType()** - return current type (cmd::Variant::ValueTypes).
* **is*()** - set of fast type checker. For example isNull, isInteger, isString.

!!! Not implemented

* **isDictionaryLike()** return true if
  * Variant is **dictionary**
  * Variant is object implemented interface **DictionaryProxy** (see "Object type").
* **isSequenceLike()** - set of fast type checker. For example isNull.
  * Variant is **sequence** 
  * Variant is object implemented interface **SequenceProxy** (see "Object type").

### Deep cloning
Variant method **clone()** returns a deep clone.
It recursively clones new sequences, dictionaries. 

!!! Not implemented
If Variant contains object which is implemented interface **Clonable**, this object will be cloned too.

### Comparison
Variant provides **operator ==** with Scalar types or other Variant (Variant convertible type).
**operator ==** performs:

  * comparison of Scalar types
  * recursively comparison of structural types.

If types of compared Variants are different, **operator ==** returns false. 
If types of compared Variants are object, **operator ==** returns false.

### Iteration
Variant implements iteration feature in STL style.
It provides methods `begin()` and `end()` and can be used in range-based for.
Also it provided unsafe iteration: `unsafeBegin()`, `unsafeEnd()`, `getUnsafeIterable()`. See "Unsafe iteration methods"

Variant iteration methods iterate values only and return ConstSequenceIterator.
**Each type of Variant is iterable**:

* null-type behaves like empty sequence.
* scalar-type behaves like sequence with one element.
* dictionary iterates its values in undetermined order.
* sequence iterates its values
* object behavior depends on implemented interfaces
  * if the object implements SequenceProxy, it behaves like sequence
  * if the object implements DictionaryProxy, it behaves like dictionary
  * otherwise the object behaves like scalar-type

**Example:**
```cpp
Variant v = Sequence({1,2,3});

// Old style Iteration
for (auto Iter=v.begin(); Iter!=v.end(); ++Iter)
   Variant vValue = *Iter;

// Iteration in range-based for 
for (auto vVal : v.seqIterable())
   Variant vValue = vVal;
```
### Scalar types
Scalar types are null, boolean, integer, string.

For reading and writing scalar types Variant provides:

* `Constructor` - create from scalar value
* `operator =` - write scalar value
* `operator Type` - read scalar value

When you write data to variant (operator =) , it does not change old data and just forget about them.
If you try to read unappropriate type, the TypeError exception will be thrown.

**Examples:**
```cpp
// Constructor
Variant vBool (true); // create variant with type boolean

// Assignment
Variant vStr;
vStr = "Data"; // writing string data in variant

// operator Type
std::wstring sString = vStr; // Reading string data from variant

// Exception
Variant vInteger(10);  // Write integer
std::wstring sString = vInteger; // Read string
```
##### null features
Default constructor create null-type variant:
```cpp
Variant vNull;
nullptr_t Null = vNull; // reading of Null
```
It is better to use the isNull() method to check that Variant is null.

##### Integer features
You can write and read any integer types into Variant (up to 64-bit).
When you write integer, Variant saves value of this type and its sign property.

When you read, Variant check ability to save its value into requested type.
If type is wrong, the boost::numeric::bad_numeric_cast exception will be thrown.
One exception from this rule: -1 can be assigned into any unsigned type.
```cpp
Variant vInt(-20);
uint64_t nVal = vInt; // exception

// You cat get raw IntKeeper if you need.
Variant::IntKeeper nKeeper = vInt;
```
##### String features
String is **stored by reference**.
Copying of Variant with string is fast as it is just copying of reference.
String is constant. You can't change internal string, 
but you can write new string to Variant (Variant forgets old string).

Support readding string into `std::string`( `std::wstring` ) and `std::string_view` ( `std::wstring_view` ).
```cpp
std::wstring sString = vStr; // Reading string data from variant (with data coping)
std::wstring_view sStringView = vStr; // Reading string data from variant (without data coping)
```
If you will not change or storing data, It's recomended to read string as `std::string_view`

**UTF8 - UTF16 conversion**
Variant can keep UTF8 and UTF16 string.
It save string internally without change of encoding.
If you read string of stored encoding, Variant just return value.
If you read string of another encoding, Variant automatically encodes string, save new string internally and return it.
After conversion Variant will be contain strings of both encodings: UTF8 and UTF16.

### Structural types
Structural types: dictionary, sequence.
Variant keep structural types by reference so copying of Variant is to copy reference.

Variant it **provides**: 

* proxy methods to access data of structural types.

but it does **NOT provide**:

* Creation of new structural type (use cmd::Dictionary and cmd::Sequence constructors)

Structural type methods work with appropriate type only, unless otherwise specified.
Otherwise the TypeError exception is thrown.

#### Common methods
* size_t getSize() const;
* bool isEmpty() const;
* void clear();

Methods **getSize()** and **isEmpty()** works with scalar according to iteration logic (see point "Iteration").

#### Dictionary access methods
* size_t erase(DictionaryIndex Id);
* bool has(DictionaryIndex Id) const;
* const Variant operator [] (DictionaryIndex Id) const { return get(Id); }
* Variant get(DictionaryIndex Id) const;
* Variant get(DictionaryIndex Id, const Variant& vDefaultValue) const;
* void put(DictionaryIndex Id, const Variant& vValue);
* Dictionary iteration methods. They returns ConstDictionaryIterator  (see the "Iterators" point).
  * recommended methods
    * dictBegin();
    * dictEnd();
    * dictIterable(); // use in range-based for
  * unsafe iteration methods
    * unsafeDictBegin();
    * unsafeDictEnd();
    * unsafeDictIterable(); // use in range-based for

Dictionary access methods works only if Variant is **isDictionaryLike**.
Otherwise the TypeError exception is thrown. 

#### Sequence access methods
* size_t erase(SequenceIndex Id);
* const Variant operator [] (SequenceIndex Id) const { return get(Id); }
* Variant get(SequenceIndex Id, const Variant& vDefaultValue) const;
* Variant get(SequenceIndex Id) const;
* void put(SequenceIndex Id, const Variant& vValue);
* void push_back(const Variant& vValue);
* Variant& operator += (const Variant& vValue);
* void resize(size_t nNewSize);
* void insert(SequenceIndex Id, const Variant& vValue);
* Sequence iteration methods. They returns ConstSequenceIterator  (see the "Iterators" point).
  * recommended methods
    * seqBegin();
    * seqEnd();
    * seqIterable(); // use in range-based for
  * unsafe iteration methods (see Unsafe iteration methods)
    * unsafeSeqBegin();
    * unsafeSeqEnd();
    * unsafeSeqIterable(); // use in range-based for

Sequence access methods works only if Variant is **isSequenceLike**.
Otherwise the TypeError exception is thrown. 

#### Iteration examples
```cpp
// Iteration in range-based for 
Variant v = Dictionary();
for (auto Pair : v.dictIterable())
{
   std::string sKey = Pair.first;
   Variant vValue = Pair.second;
}

// Old style Iteration
Variant v = Dictionary();
for (auto Iter=v.dictBegin(); Iter!=v.dictEnd(); ++Iter)
{
   std::string sKey = Iter->first;
   Variant vValue = Iter->second;
   sKey = Iter.key();
   vValue = Iter.value();
}

// Iteration in range-based for 
Variant v = Sequence();
for (auto vVal : v.seqIterable())
{
   Variant vValue = vVal;
}

// Old style Iteration
Variant v = Sequence();
for (auto Iter=v.seqBegin(); Iter!=v.seqEnd(); ++Iter)
{
   Variant vValue = * Iter;
}
```

### Object type
`TODO`

#### Special object interfaces
!!! Not implemented
DictionaryProxy, SequenceProxy, VariantProxy


## cmd::Dictionary
cmd::Dictionary is add-on object over cmd::Variant for convenient access to DictionaryLike container.
Don't confuse with cmd::variant::detail::BasicDictionary (dictionary object implementation).

Sequence is wrapper of **isDictionaryLike** Variant.

cmd::Dictionary provide:

* constructors to create new dictionary
  * Default constructor creates empty Dictionary
  * Constructor from std::initializer_list: Dictionary({ {"aaa", 10},  {"bbb", true} })
* From/to Variant conversions
  * explicit constructor from Variant (see below)
  * implicit operator Variant()
* Dictionary access methods (same as Variant)
* Dictionary iteration methods (see below)

### Constructor from Variant
It supports Variant which is **isDictionaryLike**.
Overwise it throw TypeError.

This constructor just creates wrapper around passed Variant.
This constructor is explicit for symmetry to the same Sequence constructor.

### Iteration methods
Iteration of Dictionary is like iteration of std::map. It has begin() and end() methods 
which return ConstDictionaryIterator (see the "Iterators" point).

Dictionary provide:

  * recommended methods
    * begin();
    * end();
  * unsafe iteration methods (see Unsafe iteration methods)
    * unsafeBegin();
    * unsafeEnd();
    * getUnsafeIterable(); // for range based for

**Examples:**
```cpp
// Iteration in range-based for 
Dictionary Dict();
for (auto Pair : Dict)
{
   std::string sKey = Pair.first;
   Variant vValue = Pair.second;
}

// Old style Iteration
Dictionary Dict();
for (auto Iter=Dict.begin(); Iter!=Dict.end(); ++Iter)
{
   std::string sKey = Iter->first;
   Variant vValue = Iter->second;
   sKey = Iter.key();
   vValue = Iter.value();
}
```

## cmd::Sequence
cmd::Sequence is add-on object over cmd::Variant for convenient access to SequenceLike container.
Don't confuse with cmd::variant::detail::BasicSequence (sequence object implementation).

Sequence is wrapper of **isSequenceLike** Variant.

cmd::Sequence provide:

* Constructors to create new sequence
  * Default constructor creates empty Sequence
  * Constructor from std::initializer_list: Sequence({ "aaa", 10, true })
* From/to Variant conversions
  * explicit constructor from Variant (see below)
  * implicit operator Variant()
* Sequence access methods (same as Variant)
  * One exception: Sequence around Scalar type and DictionaryLike is not readonly. It becomes writable.
* Iteration methods (see below)

Constructor from Variant supports  Variant which is **isDictionaryLike**.
Overwise it throw TypeError.

### Constructor from Variant
It supports a Variant of any type because all types are iterable.
If a passed Variant is not **isSequenceLike**, 
the constructor create new Sequence and fill it with appropriate values.

So Variant(Sequence(vOrigVariant)) can be not equal to vOrigVariant.
This constructor is explicit in order to a developer explicitly can see this conversion.

### Iteration methods
Iteration method return ConstSequenceIterator (see the "Iterators" point).
Sequence provide:

  * recommended methods
    * begin();
    * end();
  * unsafe iteration methods (see Unsafe iteration methods)
    * unsafeBegin();
    * unsafeEnd();
    * getUnsafeIterable(); // for range based for

**Examples:**
```cpp
// Iteration in range-based for 
Sequence Seq();
for (auto vVal : Seq)
{
   Variant vValue = vVal;
}

// Old style Iteration
Sequence Seq();
for (auto Iter=v.begin(); Iter!=v.end(); ++Iter)
{
   Variant vValue = * Iter;
}
```

# Iterators

## Iterator types
Variant library provide 2 types of iterator:

 * ConstDictionaryIterator
 * ConstSequenceIterator

Both iterators:

 * work like forward iterator;
 * don't support coping, moving only;
 * provide read only access.

**ConstDictionaryIterator** provides:

 * operator ++ (prefix only)
 * operator != (end comparance)
 * operator * return `std::pair<const DictionaryIndex&, const Variant&>` for current element
 * operator -> return `std::pair<const DictionaryIndex&, const Variant&>` for current element

**ConstSequenceIterator** provides:

 - operator ++ (prefix only)
 - operator *  return const Variant& (value of the current element)
 - operator -> return const Variant& (value of the current element)
 - operator != Operator is use for "end" comparance

## Iterator hierarchy
All External Iterator classes created by External Container classes methods.

* External classes:
  * **cmd::variant::ConstSequenceIterator** - readonly iterator of a Sequence like object; 
  * **cmd::variant::ConstDictionaryIterator** - readonly iterator of a Dictionary like object;
  * cmd::variant::IteratorInterface - main interface of all iterator implementations (used by ConstSequenceIterator and ConstDictionaryIterator);
* Internal classes (implementations of IteratorInterface):
  * cmd::variant::detail::ScalarIterator - iterator of scalar type (like sequence with one element)
  * cmd::variant::detail::BasicSequenceIterator - iterator of BasicSequence (safe and unsafe)
  * cmd::variant::detail::BasicDictionaryUnsafeIterator - unsafe iterator of BasicDictionary
  * cmd::variant::detail::BasicDictionaryIterator - unsafe iterator of BasicDictionary

### Unsafe iteration methods
Recommended methods provide thread safe iteration.
This iteration works correct even if container is changed during iteration.
Performance of safe methods is enough in the most cases.

Unsafe methods are not thread safe. And work like STL iterator.
They behavior is undefined if container is changed during iteration.
But they are faster than safe methods but not so much. 
Use unsafe methods in exception cases only.


# Serialization
Variant can be serizalized to JSON and deserialized from it.

* `variant::serializeToJson()`
* `variant::deserializeFromJson`


null, scalars, dictionary and sequence converted to appropriate JSON types.
object type has special serialization rules.

## Object type serialization
dictionary with field "$$clsid" converted to object.
Object created from this clsid and this dictionary as parameter.

"$$clsid" can be integer or string contained integer.

Examples:
```cpp
{ "$$clsid" : "0x12345678", "finaConstructArgument" : "Value" }
{ "$$clsid" : 1234567, "finaConstructArgument" : "Value" }
```

Object is not support serizalization (temporarily).
If Variant containt not serializable object, the exception is thrown.





