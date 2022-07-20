//
//  EDRAv2. Unit test
//    Variant.
//
#include "pch.h"

using namespace cmd;

inline constexpr size_t c_nValueTypeCount = static_cast<size_t>(variant::detail::RawValueType::Max);


//////////////////////////////////////////////////////////////////////////
//
// TestObject
//

//
//
//
class ISimpleTestObject : OBJECT_INTERFACE
{
public:
	virtual void callMethod() = 0;
};

//
//
//
class INotImplementedInterface : OBJECT_INTERFACE
{
public:
	virtual void callMethod2() = 0;
};


//
//
//
class SimpleTestObject : public ObjectBase <>, public ISimpleTestObject
{
public:
	virtual void callMethod() override {}
};

//
//
//
TEST_CASE("VariantValueKeeper. Test type()")
{
	using RawValueType = cmd::variant::detail::RawValueType;
	typedef cmd::variant::detail::Value TVariant;
	using IntKeeper = cmd::Variant::IntValue;

	TVariant vTmp;
	REQUIRE(vTmp.getType() == RawValueType::Null);

	vTmp.set(nullptr);
	REQUIRE(vTmp.getType() == RawValueType::Null);
	vTmp.set(false);
	REQUIRE(vTmp.getType() == RawValueType::Boolean);

	IntKeeper nSrcVal = 10;
	vTmp.set(nSrcVal);
	REQUIRE(vTmp.getType() == RawValueType::Integer);

	auto pStringValue = cmd::variant::detail::StringValue ::create("asdasd");
	vTmp.set(pStringValue);
	REQUIRE(vTmp.getType() == RawValueType::String);

	ObjPtr<IObject> pObj = createObject<SimpleTestObject>();
	vTmp.set(pObj);
	REQUIRE(vTmp.getType() == RawValueType::Object);
}

//////////////////////////////////////////////////////////////////////////
//
// Variant. Scalar type. 
// 

//
//
//
TEST_CASE("Variant. Copy/assign/move Variant")
{
	using cmd::Variant;

	SECTION("Copy")
	{
		Variant vTest1("qqqqqqqqqqqqqqq");
		REQUIRE(vTest1 == "qqqqqqqqqqqqqqq");

		Variant vTest2(vTest1);
		REQUIRE(vTest2 == "qqqqqqqqqqqqqqq");

		std::string_view s1 = vTest1;
		std::string_view s2 = vTest2;
		REQUIRE(s1.data() == s2.data());
	}

	SECTION("Assign")
	{
		Variant vTest1("qqqqqqqqqqqqqqq");
		REQUIRE(vTest1 == "qqqqqqqqqqqqqqq");

		Variant vTest2("aaaaaaaaaaaaaaaaa");
		REQUIRE(vTest2 == "aaaaaaaaaaaaaaaaa");

		vTest2 = vTest1;
		REQUIRE(vTest2 == "qqqqqqqqqqqqqqq");

		std::string_view s1 = vTest1;
		std::string_view s2 = vTest2;
		REQUIRE(s1.data() == s2.data());
	}

	SECTION("Move")
	{
		Variant vTest1("qqqqqqqqqqqqqqq");
		REQUIRE(vTest1 == "qqqqqqqqqqqqqqq");
		REQUIRE(vTest1.getType() == Variant::ValueType::String);

		Variant vTest2(std::move(vTest1));
		REQUIRE(vTest2 == "qqqqqqqqqqqqqqq");
		REQUIRE(vTest1.getType() == Variant::ValueType::Null);
	}
}

//
//
//
TEST_CASE("Variant. Scalar type. Conversion to and from null type")
{
	Variant vTest(nullptr);
	REQUIRE(vTest.getType() == Variant::ValueType::Null);
	REQUIRE(static_cast<nullptr_t>(vTest) == nullptr);

	vTest = 10u;
	REQUIRE(vTest.getType() == Variant::ValueType::Integer);
	vTest = nullptr;
	REQUIRE(vTest.getType() == Variant::ValueType::Null);
	REQUIRE(static_cast<nullptr_t>(vTest) == nullptr);

	nullptr_t Val = vTest;
	REQUIRE(Val == nullptr);
}

//
//
//
TEST_CASE("Variant. Scalar type. Conversion to and from bool type")
{
	Variant vTest(true);
	REQUIRE(vTest.getType() == Variant::ValueType::Boolean);
	REQUIRE((bool)vTest);

	vTest = nullptr;
	REQUIRE(vTest.getType() == Variant::ValueType::Null);

	vTest = false;
	REQUIRE(vTest.getType() == Variant::ValueType::Boolean);
	REQUIRE(!(bool)vTest);

	bool Val = vTest;
	REQUIRE(!Val);
}

//
//
//
TEST_CASE("Variant. Scalar type. Conversion to and from integer type")
{
	SECTION("Simple unsigned")
	{
		Variant vTest(10u);
		REQUIRE(vTest.getType() == Variant::ValueType::Integer);
		REQUIRE((uint64_t)vTest == 10);
		REQUIRE(vTest == 10);

		vTest = nullptr;
		REQUIRE(vTest.getType() == Variant::ValueType::Null);

		vTest = 20u;
		REQUIRE(vTest.getType() == Variant::ValueType::Integer);
		REQUIRE(vTest == 20);

		uint64_t Val = vTest;
		REQUIRE(Val == 20);
	}

	SECTION("Simple signed")
	{
		Variant vTest(-10);
		REQUIRE(vTest.getType() == Variant::ValueType::Integer);
		REQUIRE((int64_t)vTest == -10);
		REQUIRE(vTest == -10);

		vTest = nullptr;
		REQUIRE(vTest.getType() == Variant::ValueType::Null);

		vTest = -20;
		REQUIRE(vTest.getType() == Variant::ValueType::Integer);
		REQUIRE(vTest == -20);

		int64_t Val = vTest;
		REQUIRE(Val == -20);
	}

	SECTION("enum")
	{
		enum class ExampleEnum
		{
			Elem1,
			Elem2,
		};

		Variant vTest(ExampleEnum::Elem1);
		REQUIRE(vTest.getType() == Variant::ValueType::Integer);
		REQUIRE((ExampleEnum)vTest == ExampleEnum::Elem1);

		vTest = nullptr;

		vTest = ExampleEnum::Elem2;
		REQUIRE(vTest.getType() == Variant::ValueType::Integer);
		REQUIRE(vTest == ExampleEnum::Elem2);


		ExampleEnum e = vTest;
		REQUIRE(e == ExampleEnum::Elem2);
	}

	SECTION("IntKeeper")
	{
		static_assert(Variant::IntValue::IsSupportedType<const Variant::IntValue&>::value, "xxxxx");

		{
			Variant vTest1(256u);
			REQUIRE(vTest1.getType() == Variant::ValueType::Integer);
			//Variant::IntKeeper oVal = vTest1.operator Variant::IntKeeper();
			//Variant::IntKeeper oVal = (Variant::IntKeeper) vTest1;
			Variant::IntValue oVal = vTest1;
			REQUIRE(!oVal.isSigned());
			REQUIRE(oVal.asSigned() == 256u);

			Variant vTest2(oVal);
			REQUIRE(static_cast<uint16_t>(vTest1) == 256);
		}

		{
			Variant vTest1(-256);
			REQUIRE(vTest1.getType() == Variant::ValueType::Integer);
			Variant::IntValue oVal = vTest1;
			REQUIRE(oVal.isSigned());
			REQUIRE(oVal.asSigned() == -256);

			Variant vTest2 = oVal;
			REQUIRE(static_cast<int16_t>(vTest1) == -256);
		}
	}

	SECTION("numeric_cast. Signed positive")
	{
		Variant vTest1(255);
		REQUIRE(vTest1.getType() == Variant::ValueType::Integer);

		REQUIRE(static_cast<uint16_t>(vTest1) == 255);
		REQUIRE(static_cast<int16_t>(vTest1) == 255);
		REQUIRE(static_cast<uint8_t>(vTest1) == 255);

		REQUIRE_THROWS_AS([&]() {
			static_cast<int8_t>(vTest1);
		}(), error::ArithmeticError);
	}

	SECTION("numeric_cast. Signed negative")
	{
		Variant vTest1(-10);
		REQUIRE(vTest1.getType() == Variant::ValueType::Integer);

		REQUIRE(static_cast<int16_t>(vTest1) == -10);
		REQUIRE(static_cast<int8_t>(vTest1) == -10);

		REQUIRE_THROWS_AS([&]() {
			static_cast<uint8_t>(vTest1);
		}(), error::ArithmeticError);

		REQUIRE_THROWS_AS([&]() {
			static_cast<uint32_t>(vTest1);
		}(), error::ArithmeticError);
	}

	SECTION("numeric_cast. -1")
	{
		Variant vTest1(-1);
		REQUIRE(vTest1.getType() == Variant::ValueType::Integer);

		REQUIRE(static_cast<int16_t>(vTest1) == -1);
		REQUIRE(static_cast<int8_t>(vTest1) == -1);
		REQUIRE(static_cast<uint8_t>(vTest1) == 255);
		REQUIRE(static_cast<uint16_t>(vTest1) == 0xFFFF);
		REQUIRE(static_cast<uint32_t>(vTest1) == 0xFFFFFFFF);
	}

	SECTION("numeric_cast. Unsigned")
	{
		Variant vTest1(256u);
		REQUIRE(vTest1.getType() == Variant::ValueType::Integer);

		REQUIRE(static_cast<uint16_t>(vTest1) == 256);
		REQUIRE(static_cast<int16_t>(vTest1) == 256);

		REQUIRE_THROWS_AS([&]() {
			static_cast<int8_t>(vTest1);
		}(), error::ArithmeticError);

		REQUIRE_THROWS_AS([&]() {
			static_cast<uint8_t>(vTest1);
		}(), error::ArithmeticError);

		REQUIRE_THROWS_AS([&]() {
			static_cast<int8_t>(vTest1);
		}(), error::ArithmeticError);
	}
}


//
//
//
template <typename T>
void TestConversionFromString(T Val)
{
	// Constructor
	Variant vTest(Val);
	REQUIRE(vTest.getType() == Variant::ValueType::String);
	REQUIRE(vTest == Val);

	// operator = 
	vTest = nullptr;
	vTest = Val;
	REQUIRE(vTest.getType() == Variant::ValueType::String);
	REQUIRE(vTest == Val);

	// operator !=
	REQUIRE_FALSE(vTest != Val);
}

//
//
//
template <typename T>
void TestConversionToString(T Val)
{
	Variant vTest(Val);

	// operator Type
	T otherVal = vTest;
	REQUIRE(otherVal == Val);
	REQUIRE(T(vTest) == Val);
}

//
//
//
template <typename Ch>
void TestConversionToString(const Ch* Val)
{
	Variant vTest(Val);

	// operator Type
	const Ch* otherVal = vTest;
	REQUIRE(std::basic_string_view<Ch>(Val) == otherVal);
	REQUIRE(std::basic_string_view<Ch>(Val) == (const Ch*)(vTest));
}

//
//
//
TEST_CASE("Variant. Scalar type. Conversion to and from string type")
{
	SECTION("string")
	{
		TestConversionFromString(std::string("TestString"));
		TestConversionToString(std::string("TestString"));
	}

	SECTION("wstring")
	{
		TestConversionFromString(std::wstring(L"TestString"));
		TestConversionToString(std::wstring(L"TestString"));
	}

	SECTION("string_view")
	{
		TestConversionFromString(std::string_view("TestString"));
		TestConversionToString(std::string_view("TestString"));
	}

	SECTION("wstring_view")
	{
		TestConversionFromString(std::wstring_view(L"TestString"));
		TestConversionToString(std::wstring_view(L"TestString"));
	}

	SECTION("char")
	{
		TestConversionFromString((const char*)"TestString");
		//TestConversionToString((const char*)"TestString");
	}

	SECTION("wchar_t")
	{
		TestConversionFromString((const wchar_t*)L"TestString");
		//TestConversionToString((const wchar_t*)L"TestString");
	}

	SECTION("char string literal")
	{
		// Constructor
		Variant vTest("TestString");
		REQUIRE(vTest.getType() == Variant::ValueType::String);
		REQUIRE(vTest == "TestString");

		// operator = 
		vTest = nullptr;
		vTest = "TestString";
		REQUIRE(vTest.getType() == Variant::ValueType::String);
		REQUIRE(vTest == "TestString");

		// operator !=
		REQUIRE_FALSE(vTest != "TestString");
	}
}

//
//
//
TEST_CASE("Variant. Scalar type. String encoding conversion")
{
	// operator == conversion
	{
		REQUIRE(Variant("") == L"");
		REQUIRE(Variant(L"") == "");
		REQUIRE(Variant("TestString") == L"TestString");
		REQUIRE(Variant(L"TestString") == "TestString");
		REQUIRE(Variant(L"TestString") == Variant("TestString"));
		REQUIRE(Variant("TestString") == Variant(L"TestString"));
	}

	// operator Type conversion
	// Utf8 -> Utf16
	{
		Variant vUtf8("TestString");
		std::wstring_view sUtf16 = vUtf8;
		REQUIRE(sUtf16 == L"TestString");
		std::wstring_view sUtf16_ = vUtf8;
		REQUIRE(sUtf16_ == sUtf16);
	}

	// operator Type conversion
	// Utf8 <- Utf16
	{
		Variant vWchar(L"TestString");
		std::string_view sUtf8 = vWchar;
		REQUIRE(sUtf8 == "TestString");
		std::string_view sUtf8_ = vWchar;
		REQUIRE(sUtf8_ == sUtf8);
	}
}

//
//
//
template <typename ScalarType>
void testScalarTypeConversion(int nNothrowIndex, std::vector<Variant>& values)
{
	for (size_t i=0; i<values.size(); ++i)
	{
		auto fnConvertor = [&]() 
		{
			ScalarType scalar = values[i];
		};
		if (i == nNothrowIndex)
			CHECK_NOTHROW(fnConvertor());
		else
			CHECK_THROWS_AS(fnConvertor(), error::TypeError);
	}
}

//
//
//
TEST_CASE("Variant.conversion_to_invalid_scalar_type")
{
	std::vector<Variant> variantList = { Variant(nullptr), Variant(true), Variant(10), Variant("XXXX"), 
		Variant(createObject<SimpleTestObject>()), Sequence(), Dictionary() };

	SECTION("Null")
	{
		testScalarTypeConversion<nullptr_t>(0, variantList);
	}

	SECTION("Boolean")
	{
		testScalarTypeConversion<bool>(1, variantList);
	}

	SECTION("Integer")
	{
		testScalarTypeConversion<int>(2, variantList);
	}

	SECTION("String")
	{
		testScalarTypeConversion<std::string>(3, variantList);
	}

	SECTION("Object")
	{
		testScalarTypeConversion<ObjPtr<IObject>>(4, variantList);
	}
}




//
//
//
bool hasWcharString(const Variant& v)
{
	auto pStringValue = variant::detail::convertVariantSafe<std::shared_ptr<variant::detail::StringValue>>(v);
	if (!pStringValue.has_value())
		return false;
	return (*pStringValue)->hasWChar();
}

//
//
//
bool hasUtf8String(const Variant& v)
{
	auto pStringValue = variant::detail::convertVariantSafe<std::shared_ptr<variant::detail::StringValue>>(v);
	if (!pStringValue.has_value())
		return false;
	return (*pStringValue)->hasUtf8();
}

//
// Implementation specific test (check internal features)
//
TEST_CASE("Variant. Scalar type. String allocations during encoding")
{
	// operator Type returns the same buffer
	{
		Variant vStr("TestString");
		REQUIRE(std::string_view(vStr).data() == std::string_view(vStr).data());
		REQUIRE(std::wstring_view(vStr).data() == std::wstring_view(vStr).data());
	}

	// Check allocation
	{
		Variant vWchar(L"TestString");
		REQUIRE(hasWcharString(vWchar));
		REQUIRE_FALSE(hasUtf8String(vWchar));

		REQUIRE_FALSE(vWchar == L"XXX");
		REQUIRE(hasWcharString(vWchar));
		REQUIRE_FALSE(hasUtf8String(vWchar));

		REQUIRE_FALSE(vWchar == "XXX");
		REQUIRE(hasWcharString(vWchar));
		REQUIRE(hasUtf8String(vWchar));

		Variant vUtf8("TestString");
		REQUIRE_FALSE(hasWcharString(vUtf8));
		REQUIRE(hasUtf8String(vUtf8));

		REQUIRE_FALSE(vUtf8 == "XXX");
		REQUIRE_FALSE(hasWcharString(vUtf8));
		REQUIRE(hasUtf8String(vUtf8));

		REQUIRE_FALSE(vUtf8 == L"XXX");
		REQUIRE(hasWcharString(vUtf8));
		REQUIRE(hasUtf8String(vUtf8));
	}

	// Check allocation empty
	{
		Variant vUtf8("");
		REQUIRE(hasWcharString(vUtf8));
		REQUIRE(hasUtf8String(vUtf8));

		Variant vWchar(L"");
		REQUIRE(hasWcharString(vWchar));
		REQUIRE(hasUtf8String(vWchar));
	}

}


void IterateLikeSalarType(Variant vOrigValue)
{
	Variant vTest = vOrigValue;

	// size
	REQUIRE(vTest.getSize() == 1);

	// empty
	REQUIRE(!vTest.isEmpty());

	// ranged for
	{
		size_t nCount = 0;
		for (auto vValue : vTest)
		{
			++nCount;
			REQUIRE(vValue == vOrigValue);
		}
		REQUIRE(nCount == 1);
	}

	// iterator
	{
		size_t nCount = 0;
		for (auto Iter = vTest.begin(); Iter != vTest.end(); ++Iter)
		{
			++nCount;
			REQUIRE(*Iter == vOrigValue);
		}
		REQUIRE(nCount == 1);
	}

	// unsafe ranged for
	{
		size_t nCount = 0;
		for (auto vValue : vTest.getUnsafeIterable())
		{
			++nCount;
			REQUIRE(vValue == vOrigValue);
		}
		REQUIRE(nCount == 1);
	}

	// unsafe iterator
	{
		size_t nCount = 0;
		for (auto Iter = vTest.unsafeBegin(); Iter != vTest.unsafeEnd(); ++Iter)
		{
			++nCount;
			REQUIRE(*Iter == vOrigValue);
		}
		REQUIRE(nCount == 1);
	}
}

//
//
//
TEST_CASE("Variant. Scalar type. Iteration + size + empty (without null)")
{
	Variant ScalarValues[] = { Variant(true), Variant(10), Variant("XXXX")};

	for (auto vOrigValue : ScalarValues)
		IterateLikeSalarType(vOrigValue);
}

//
//
//
TEST_CASE("Variant. Scalar type. Iteration + size + empty (null-type)")
{
	Variant vTest(nullptr);

	// size
	REQUIRE(vTest.getSize() == 0);

	// empty
	REQUIRE(vTest.isEmpty());

	// ranged for
	{
		size_t nCount = 0;
		for (auto vValue : vTest)
		{
			++nCount;
		}
		REQUIRE(nCount == 0);
	}

	// iterator
	{
		size_t nCount = 0;
		for (auto Iter = vTest.begin(); Iter != vTest.end(); ++Iter)
		{
			++nCount;
		}
		REQUIRE(nCount == 0);
	}

	// unsafe ranged for
	{
		size_t nCount = 0;
		for (auto vValue : vTest.getUnsafeIterable())
		{
			++nCount;
		}
		REQUIRE(nCount == 0);
	}

	// unsafe iterator
	{
		size_t nCount = 0;
		for (auto Iter = vTest.unsafeBegin(); Iter != vTest.unsafeEnd(); ++Iter)
		{
			++nCount;
		}
		REQUIRE(nCount == 0);
	}
}

//
//
//
void testSequenceMethodsForNotsupportedType(cmd::Variant vOrigValue)
{
	REQUIRE(vOrigValue.getType() != Variant::ValueType::Sequence);

	// operator []
	REQUIRE_THROWS_AS([&]() {
		(void)vOrigValue[0];
	}(), error::TypeError);

	// get
	REQUIRE_THROWS_AS([&]() {
		(void)vOrigValue.get(0);
	}(), error::TypeError);

	// get with default
	REQUIRE_NOTHROW([&]() {
		REQUIRE(vOrigValue.get(0, 0) == 0);
	}());

	// put
	REQUIRE_THROWS_AS([&]() {
		vOrigValue.put(0, Variant(1));
	}(), error::TypeError);

	// operator +=
	REQUIRE_THROWS_AS([&]() {
		vOrigValue += Variant(1);
	}(), error::TypeError);

	// push_back
	REQUIRE_THROWS_AS([&]() {
		vOrigValue.push_back(Variant(1));
	}(), error::TypeError);

	// insert
	REQUIRE_THROWS_AS([&]() {
		vOrigValue.insert(0, Variant(1));
	}(), error::TypeError);

	// erase
	REQUIRE_THROWS_AS([&]() {
		vOrigValue.erase(0);
	}(), error::TypeError);
}

//
//
//
void testDictionaryMethodsForNotsupportedType(cmd::Variant vOrigValue)
{
	REQUIRE(vOrigValue.getType() != Variant::ValueType::Dictionary);

	// has
	REQUIRE_THROWS_AS([&]() {
		vOrigValue.has("INT");
	}(), error::TypeError);

	// put
	REQUIRE_THROWS_AS([&]() {
		vOrigValue.put("INT", 200);
	}(), error::TypeError);

	// get
	REQUIRE_THROWS_AS([&]() {
		std::string sStr = vOrigValue.get("STR");
	}(), error::TypeError);

	// get with default
	REQUIRE_NOTHROW([&]() {
		Variant vTest = vOrigValue;
		REQUIRE(vTest.get("a", 0) == 0);
	}());

	// operator []
	REQUIRE_THROWS_AS([&]() {
		uint16_t nVal = vOrigValue["INT"];
	}(), error::TypeError);
}

//
//
//
TEST_CASE("Variant. Scalar type. Sequence and Dictionary methods")
{
	Variant ScalarValues[] = {Variant(nullptr), Variant(true), Variant(10), Variant("XXXX")};

	for (auto vOrigValue : ScalarValues)
	{
		REQUIRE_NOTHROW(testSequenceMethodsForNotsupportedType(vOrigValue));
		REQUIRE_NOTHROW(testDictionaryMethodsForNotsupportedType(vOrigValue));

		// clear
		REQUIRE_THROWS_AS([&]() {
			Variant vTest = vOrigValue;
			vTest.clear();
		}(), error::TypeError);
	}
}

//////////////////////////////////////////////////////////////////////////
//
// Variant. BasicDictionary. 
// 

//
//
//
template<typename DictionaryLikeType, typename Fn>
void testDictionaryMethodsForSupportedType(Fn fnCreate)
{
	// getType
	{
		DictionaryLikeType vTest = fnCreate(cmd::Dictionary());
		REQUIRE(Variant(vTest).isDictionaryLike());
	}

	// isEmpty
	{
		REQUIRE(fnCreate(cmd::Dictionary()).isEmpty());
		REQUIRE_FALSE(fnCreate(cmd::Dictionary({ {"a", 1} })).isEmpty());
	}

	// has
	{
		DictionaryLikeType vTest = fnCreate(cmd::Dictionary({ {"a", 1} }));
		REQUIRE(vTest.has("a"));
		REQUIRE_FALSE(vTest.has("b"));
	}

	// size
	{
		REQUIRE(fnCreate(cmd::Dictionary()).getSize() == 0);
		REQUIRE(fnCreate(cmd::Dictionary({ {"a", 1}, {"b", 1} })).getSize() == 2);
	}

	// put
	{
		DictionaryLikeType vTest = fnCreate(cmd::Dictionary());
		vTest.put("a", 200);
		vTest.put("b", true);
		vTest.put("b", "str");
		REQUIRE(vTest.getSize() == 2);
		REQUIRE(vTest["a"] == 200);
		REQUIRE(vTest["b"] == "str");
	}

	// get
	{
		DictionaryLikeType vTest = fnCreate(cmd::Dictionary({ {"a", 1}, {"b", "str"} }));

		REQUIRE_NOTHROW([&]() {
			REQUIRE(vTest.get("a") == 1);
			REQUIRE(vTest.get("b") == "str");
			REQUIRE(int(vTest.get("a")) == 1);
			REQUIRE(std::string(vTest.get("b")) == "str");
		}());

		REQUIRE_THROWS_AS([&]() {
			uint32_t v = vTest.get("c");
		}(), error::OutOfRange);
	}

	// get with default
	{
		DictionaryLikeType vTest = fnCreate(cmd::Dictionary({ {"a", 1}, {"b", "str"} }));

		REQUIRE_NOTHROW([&]() {
			REQUIRE(vTest.get("a", 0) == 1);
			REQUIRE(vTest.get("b", 0) == "str");
			REQUIRE(vTest.get("c", 0) == 0);
		}());
	}

	// operator []
	{
		DictionaryLikeType vTest = fnCreate(Dictionary({ {"a", 1}, {"b", "str"} }));

		REQUIRE_NOTHROW([&]() {
			REQUIRE(vTest["a"] == 1);
			REQUIRE(vTest["b"] == "str");
			REQUIRE(int(vTest["a"]) == 1);
			REQUIRE(std::string(vTest["b"]) == "str");
		}());

		REQUIRE_THROWS_AS([&]() {
			uint32_t v = vTest["c"];
		}(), error::OutOfRange);
	}
}

//
//
//
TEST_CASE("Variant. BasicDictionary. Dictionary methods")
{
	testDictionaryMethodsForSupportedType<Variant>([](Variant vDict)
	{
		return Variant(vDict);
	});
}

//
//
//
TEST_CASE("Variant. BasicDictionary. Iteration")
{
	Variant vSrc = Dictionary({
		{"a", nullptr},
		{"b", 1},
		{"c", "str"},
		});

	std::vector<Variant> Ethalons;
	{
		REQUIRE(vSrc.getSize() == 3);
		for (auto kv : Dictionary(vSrc))
			Ethalons.push_back(kv.second);
		REQUIRE(Ethalons.size() == 3);
	}

	Variant vTest(vSrc);

	// size
	REQUIRE(vTest.getSize() == Ethalons.size());
	// empty
	REQUIRE(!vTest.isEmpty());

	// ranged for
	{
		size_t nIndex = 0;
		for (auto vValue : vTest)
		{
			REQUIRE(vValue == Ethalons.at(nIndex));
			++nIndex;
		}
		REQUIRE(nIndex == Ethalons.size());
	}

	// iterator
	{
		size_t nIndex = 0;
		for (auto Iter = vTest.begin(); Iter != vTest.end(); ++Iter)
		{
			REQUIRE(*Iter == Ethalons.at(nIndex));
			++nIndex;
		}
		REQUIRE(nIndex == Ethalons.size());
	}

	// unsafe ranged for
	{
		size_t nIndex = 0;
		for (auto vValue : vTest.getUnsafeIterable())
		{
			REQUIRE(vValue == Ethalons.at(nIndex));
			++nIndex;
		}
		REQUIRE(nIndex == Ethalons.size());
	}

	// unsafe iterator
	{
		size_t nIndex = 0;
		for (auto Iter = vTest.unsafeBegin(); Iter != vTest.unsafeEnd(); ++Iter)
		{
			REQUIRE(*Iter == Ethalons.at(nIndex));
			++nIndex;
		}
		REQUIRE(nIndex == Ethalons.size());
	}
}

//
//
//
TEST_CASE("Variant. BasicDictionary. Sequence methods")
{
	testSequenceMethodsForNotsupportedType(Dictionary({ {"a", 1}, {"b", "str"} }));
}


//
//
//
TEST_CASE("Variant_BasicDictionary_IDictStorage")
{
	char chBegin = 'A';

	// try to fill from 0 to 28 items
	for (char chEnd = 'Z'/*chBegin*/; chEnd <= 'Z'; ++chEnd)
	{
		Dictionary v;

		// Fill
		for (char ch = chBegin; ch < chEnd; ++ch)
		{
			std::string sKey = { ch };
			v.put(sKey, sKey);
		}

		// Check size
		REQUIRE(v.getSize() == chEnd - chBegin);

		// Check exist items
		for (char ch = chBegin; ch < chEnd; ++ch)
		{
			std::string sKey = { ch };
			REQUIRE(v[sKey] == sKey);
		}

		// Check non-exist items
		REQUIRE_THROWS(v[std::string{ chEnd }]);
	}
}



//////////////////////////////////////////////////////////////////////////
//
// Variant. BasicSequence.
// 

//
//
//
template<typename SequenceLikeType, typename Fn>
void testSequenceMethodsForSupportedType(Fn fnCreate)
{
	SequenceLikeType vTest = fnCreate(Sequence());
	REQUIRE(Variant(vTest).isSequenceLike());

	REQUIRE(vTest.isEmpty());
	vTest.push_back(200);
	vTest.push_back(-200);
	vTest.push_back("XXXX");
	REQUIRE(vTest.getSize() == 3);
	REQUIRE(!vTest.isEmpty());
	vTest.erase(0);
	REQUIRE(vTest.getSize() == 2);

	REQUIRE((int64_t)vTest.get(0) == -200);
	REQUIRE(vTest[1] == std::string_view("XXXX"));
	REQUIRE(vTest.get(100, 1234) == 1234);
	vTest.put(0, 321);
	vTest.put(1, 987);
	REQUIRE(vTest.get(0) == 321);
	REQUIRE(vTest.get(1) == 987);

	vTest.setSize(5);
	REQUIRE(vTest.getSize() == 5);
	REQUIRE(Variant(vTest[4]).isNull());
	vTest.insert(1, 456);
	REQUIRE(vTest.getSize() == 6);
	REQUIRE(vTest.get(0) == 321);
	REQUIRE(vTest.get(1) == 456);
	REQUIRE(vTest.get(2) == 987);

	vTest.clear();
	REQUIRE(vTest.isEmpty());

	REQUIRE_THROWS_AS([&]() {
		(void)vTest.get(100);
	}(), error::OutOfRange);

	REQUIRE_THROWS_AS([&]() {
		uint32_t v = vTest[100];
	}(), error::OutOfRange);

	REQUIRE_THROWS_AS([&]() {
		vTest.clear();
		vTest.push_back(200);
		std::string sVal = vTest.get(0);
	}(), error::TypeError);


	REQUIRE_NOTHROW([&]() {
		SequenceLikeType vTest2 = fnCreate(Sequence({ 100 }));
		REQUIRE(vTest2.get(0, 0) == 100);
		REQUIRE(vTest2.get(1, 0) == 0);
	}());
}

//
//
//
template<typename SequenceLikeType, typename Fn>
void testSequenceIteration(Fn fnCreate)
{
	std::string sJson = R"json(
		[1,-1,"str",{"a":1},[1, "a"],true,null]
	)json";

	SequenceLikeType vTest = fnCreate(cmd::variant::deserializeFromJson(sJson));
	REQUIRE(Variant(vTest).isSequenceLike());

	auto fnChecker = [](const std::vector<cmd::Variant>& Values)
	{
		REQUIRE(Values.size() == 7);

		REQUIRE(Values[0] == 1);
		REQUIRE(Values[1] == -1);
		REQUIRE(std::string_view(Values[2]) == "str");
		REQUIRE(Values[3]["a"] == 1);
		REQUIRE(Values[4][0] == 1);
		REQUIRE(Values[5] == true);
		REQUIRE(Values[6] == nullptr);
	};

	{
		std::vector<cmd::Variant> Values;
		for (auto Val : vTest)
		{
			Values.push_back(Val);
		}
		fnChecker(Values);
	}

	{
		std::vector<cmd::Variant> Values;
		for (auto Iter = vTest.begin(); Iter != vTest.end(); ++Iter)
		{
			Values.push_back(*Iter);
		}
		fnChecker(Values);
	}

	{
		std::vector<cmd::Variant> Values;
		for (auto Val : vTest.getUnsafeIterable())
		{
			Values.push_back(Val);
		}
		fnChecker(Values);
	}

	{
		std::vector<cmd::Variant> Values;
		for (auto Iter = vTest.unsafeBegin(); Iter != vTest.unsafeEnd(); ++Iter)
		{
			Values.push_back(*Iter);
		}
		fnChecker(Values);
	}
}



//
//
//
TEST_CASE("Variant. BasicSequence. Sequence methods")
{
	testSequenceMethodsForSupportedType<Variant>([](Variant vSeq)
	{
		return Variant(vSeq);
	});
}

//
//
//
TEST_CASE("Variant. BasicSequence. Iteration")
{
	testSequenceIteration<Variant>([](Variant vSeq)
	{
		return Variant(vSeq);
	});

}

//
//
//
TEST_CASE("Variant. BasicSequence. Dictionary methods")
{
	testDictionaryMethodsForNotsupportedType(Sequence({1,"str"}));
}


//////////////////////////////////////////////////////////////////////////
//
// Variant. Clone
//

//
//
//
TEST_CASE("Variant. Clone")
{
	SECTION("Scalar types + object")
	{
		Variant vScalars[] = { Variant(nullptr), Variant(true), Variant(10), Variant("XXXX"), Variant(createObject<SimpleTestObject>()) };

		for(auto& vScalar : vScalars)
		{
			Variant v1 = vScalar;
			Variant v2 = v1.clone();
			v1 = nullptr;
			REQUIRE(v2 == vScalar);
		}
	}

	SECTION("Dictionary")
	{
		Variant v1 = Dictionary({
			{"a", 1},
			{"b", "str"}
		});

		Variant v2 = v1.clone();
		v1.put("a", true);
		v1.put("c", 100);
		
		REQUIRE(v1.getSize() == 3);
		REQUIRE(v1["a"] == true);
		REQUIRE(v1["b"] == "str");
		REQUIRE(v1["c"] == 100);

		REQUIRE(v2.getSize() == 2);
		REQUIRE(v2["a"] == 1);
		REQUIRE(v2["b"] == "str");
	}

	SECTION("Sequence")
	{
		Variant v1 = Sequence({ true, "str" });

		Variant v2 = v1.clone();
		v1.insert(0, 10);

		REQUIRE(v1.getSize() == 3);
		REQUIRE(v1[0] == 10);
		REQUIRE(v1[1] == true);
		REQUIRE(v1[2] == "str");

		REQUIRE(v2.getSize() == 2);
		REQUIRE(v2[0] == true);
		REQUIRE(v2[1] == "str");
	}

	SECTION("Deep structure")
	{
		Variant v1 = Sequence({ 
			true, 
			Dictionary({
				{"a", "str"},
				{"b", Sequence({ 10 })},
				{"c", Dictionary({ 
					{ "a", 100} 
				})},
			}),
			Sequence({ false })
		});

		Variant v2 = v1.clone();
		v1.erase(0);
		Variant(v1[0]).put("a", false);
		Variant(v1[0]["b"]).clear();
		Variant(v1[0]["c"]).put("b", "str");
		Variant(v1[1]).push_back(10);

		REQUIRE(v1.getSize() == 2);
		REQUIRE(v1[0].getSize() == 3);
		REQUIRE(v1[0]["a"] == false);
		REQUIRE(v1[0]["b"].getSize() == 0);
		REQUIRE(v1[0]["c"].getSize() == 2);
		REQUIRE(v1[0]["c"]["a"] == 100);
		REQUIRE(v1[0]["c"]["b"] == "str");
		REQUIRE(v1[1].getSize() == 2);
		REQUIRE(v1[1][0] == false);
		REQUIRE(v1[1][1] == 10);

		REQUIRE(v2.getSize() == 3);
		REQUIRE(v2[0] == true);
		REQUIRE(v2[1].getSize() == 3);
		REQUIRE(v2[1]["a"] == "str");
		REQUIRE(v2[1]["b"].getSize() == 1);
		REQUIRE(v2[1]["b"][0] == 10);
		REQUIRE(v2[1]["c"].getSize() == 1);
		REQUIRE(v2[1]["c"]["a"] == 100);
		REQUIRE(v2[2].getSize() == 1);
		REQUIRE(v2[2][0] == false);

	}
}

//////////////////////////////////////////////////////////////////////////
//
// Variant. Comparison
//

//
//
//
template <size_t TupleIndex, typename T>
void CompareValues(T Tuple, const std::vector<cmd::Variant>& Values, size_t ValuesIndex)
{
	CAPTURE(TupleIndex, ValuesIndex);

	REQUIRE_NOTHROW([&]() {
		const cmd::Variant & Value = Values[ValuesIndex];
		if (TupleIndex == ValuesIndex)
		{
			CHECK(Value == std::get<TupleIndex>(Tuple));
			CHECK_FALSE(Value != std::get<TupleIndex>(Tuple));
		}
		else
		{
			CHECK(Value != std::get<TupleIndex>(Tuple));
			CHECK_FALSE(Value == std::get<TupleIndex>(Tuple));
		}
	}());
}

//
//
//
TEST_CASE("Variant.comparison")
{
	SECTION("different_type_of_Variant")
	{
		std::vector<Variant> Values = { Variant(nullptr), Variant(true), Variant(1), Variant("XXXX"), Dictionary({{"a",1}}), Sequence(1), Variant(createObject<SimpleTestObject>())};
		std::vector<Variant> Values2 = Values;

		for (size_t i = 0; i < std::size(Values); ++i)
		{
			for (size_t j = 0; j < std::size(Values2); ++j)
			{
				CAPTURE(i, j);
				if (i == j)
				{
					CHECK(Values[i] == Values2[j]);
					CHECK_FALSE(Values[i] != Values2[j]);
				}
				else
				{
					CHECK(Values[i] != Values2[j]);
					CHECK_FALSE(Values[i] == Values2[j]);
				}
			}
		}
	}

	SECTION("Variant_and_literal")
	{
		std::vector<Variant> Values = { Variant(nullptr), Variant(true), Variant(1), Variant("XXXX"), Dictionary({{"a",1}}), Sequence(1), Variant(createObject<SimpleTestObject>()) };

		auto Literals = std::make_tuple(nullptr, true, 1, "XXXX");


		for (size_t i = 0; i < std::size(Values); ++i)
		{
			CompareValues<0>(Literals, Values, i);
			CompareValues<1>(Literals, Values, i);
			CompareValues<2>(Literals, Values, i);
			CompareValues<3>(Literals, Values, i);
		}
	}

	SECTION("Sequence")
	{
		#define TEST_SEQUENCE_VALUE Sequence({nullptr, true, 1, "XXXX"})
		Sequence seq1 = TEST_SEQUENCE_VALUE;
		Sequence seq2 = TEST_SEQUENCE_VALUE;

		CHECK(seq1 == seq2);
		CHECK_FALSE(seq1 != seq2);
		CHECK(Variant(seq1) == Variant(seq2));
		CHECK_FALSE(Variant(seq1) != Variant(seq2));

		seq1.push_back(1);

		CHECK_FALSE(seq1 == seq2);
		CHECK(seq1 != seq2);
		CHECK_FALSE(Variant(seq1) == Variant(seq2));
		CHECK(Variant(seq1) != Variant(seq2));

		seq2.push_back(0);

		CHECK_FALSE(seq1 == seq2);
		CHECK(seq1 != seq2);
		CHECK_FALSE(Variant(seq1) == Variant(seq2));
		CHECK(Variant(seq1) != Variant(seq2));

		#undef TEST_SEQUENCE_VALUE
	}

	SECTION("Dictionary")
	{
		#define TEST_DICTIONARY_VALUE Dictionary({ {"a", nullptr} , {"b", true}, {"c", 1},  {"d", "XXXX"}})
		Dictionary v1 = TEST_DICTIONARY_VALUE;
		Dictionary v2 = TEST_DICTIONARY_VALUE;

		CHECK(v1 == v2);
		CHECK_FALSE(v1 != v2);
		CHECK(Variant(v1) == Variant(v2));
		CHECK_FALSE(Variant(v1) != Variant(v2));

		v1.put("e", 100);

		CHECK_FALSE(v1 == v2);
		CHECK(v1 != v2);
		CHECK_FALSE(Variant(v1) == Variant(v2));
		CHECK(Variant(v1) != Variant(v2));

		v2.put("e", 200);

		CHECK_FALSE(v1 == v2);
		CHECK(v1 != v2);
		CHECK_FALSE(Variant(v1) == Variant(v2));
		CHECK(Variant(v1) != Variant(v2));

		#undef TEST_DICTIONARY_VALUE
	}

	SECTION("deep_comparison")
	{
		Variant v1 = Sequence({
			true,
			Dictionary({
				{"a", "str"},
				{"b", Sequence({ 10 })},
				{"c", Dictionary({
					{ "a", 100}
				})},
			}),
			Sequence({ false })
			});

		Variant v2 = Sequence({
			true,
			Dictionary({
				{"a", "str"},
				{"b", Sequence({ 10 })},
				{"c", Dictionary({
					{ "a", 100}
				})},
			}),
			Sequence({ false })
			});

		CHECK(v1 == v2);
		CHECK_FALSE(v1 != v2);
		CHECK(Variant(v1) == Variant(v2));
		CHECK_FALSE(Variant(v1) != Variant(v2));

		Variant(v1[1]).put("e", 100);

		CHECK_FALSE(v1 == v2);
		CHECK(v1 != v2);
		CHECK_FALSE(Variant(v1) == Variant(v2));
		CHECK(Variant(v1) != Variant(v2));

		Variant(v2[1]).put("e", 100);

		CHECK(v1 == v2);
		CHECK_FALSE(v1 != v2);
		CHECK(Variant(v1) == Variant(v2));
		CHECK_FALSE(Variant(v1) != Variant(v2));
	}

	SECTION("object")
	{
		Variant vObj1(createObject<SimpleTestObject>());
		Variant vObj2(createObject<SimpleTestObject>());

		CHECK(vObj1 != vObj2);
		CHECK_FALSE(vObj1 == vObj2);

		Variant vObj1Copy = vObj1;
		CHECK(vObj1 == vObj1Copy);
		CHECK_FALSE(vObj1 != vObj1Copy);
	}
}

//////////////////////////////////////////////////////////////////////////
//
// Sequence
//

//
//
//
TEST_CASE("Sequence.constructor")
{
	SECTION("default_constructor")
	{
		Sequence seq;
		REQUIRE(Variant(seq).getType() == Variant::ValueType::Sequence);
		REQUIRE(seq.getSize() == 0);
	}

	SECTION("from_null")
	{
		Sequence seq(Variant(nullptr));
		REQUIRE(Variant(seq).getType() == Variant::ValueType::Sequence);
		REQUIRE(seq.getSize() == 0);
	}

	SECTION("from_scalar_and_object")
	{
		Variant Scalars[] = { Variant(true), Variant(1), Variant("XXXX"), Variant(createObject<SimpleTestObject>()) };

		for (auto& vScalar : Scalars)
		{
			Sequence seq(vScalar);
			REQUIRE(Variant(seq).getType() == Variant::ValueType::Sequence);
			REQUIRE(seq.getSize() == 1);
			REQUIRE(seq[0] == vScalar);
		}
	}

	SECTION("from_Sequence")
	{
		Variant seq1 = Sequence({ 1, "str" });
		Sequence seq2(seq1);

		REQUIRE(Variant(seq2).getType() == Variant::ValueType::Sequence);
		REQUIRE(seq2.getSize() == 2);

		// test both refers to same object
		seq1.clear();
		REQUIRE(seq2.isEmpty());
	}

	SECTION("from_Dictionary")
	{
		// Test count
		{
			Variant vDict = Dictionary({ {"a", 1},  {"b", 10 },  {"c", 100 } });
			Sequence seq(vDict);

			REQUIRE(Variant(seq).getType() == Variant::ValueType::Sequence);
			REQUIRE(seq.getSize() == 3);

			int nSum = 0;
			for (auto vVal : seq)
				nSum += int(vVal);
			REQUIRE(nSum == 111);
		}

		// Test reference
		{
			Variant vDict = Dictionary({ {"a", Sequence({1,2,3}) } });
			Sequence seq(vDict);

			REQUIRE(Variant(seq).getType() == Variant::ValueType::Sequence);
			REQUIRE(seq.getSize() == 1);

			REQUIRE(seq[0].getSize() == 3);
			Variant(vDict["a"]).clear();
			REQUIRE(seq[0].getSize() == 0);
		}
	}

	SECTION("from_initializer_list")
	{
		Variant vDict = Dictionary();
		vDict.put("aaa", 20);
		Variant vInt = 10;

		Variant vTest = Sequence({ nullptr, true, 1u, -1, "str", vInt, vDict, std::string("std::string") });
		REQUIRE(vTest.getType() == Variant::ValueType::Sequence);
		REQUIRE(vTest.getSize() == 8);
		REQUIRE(vTest[0].isNull());
		REQUIRE(vTest[1] == true);
		REQUIRE(vTest[2] == 1);
		REQUIRE(vTest[3] == -1);
		REQUIRE(vTest[4] == "str");
		REQUIRE(vTest[5] == 10);
		REQUIRE(vTest[6]["aaa"] == 20);
		REQUIRE(vTest[7] == "std::string");
	}
}

//
//
//
TEST_CASE("Sequence. Iteration")
{
	testSequenceIteration<Sequence>([](Variant vSeq)
	{
		return Sequence(vSeq);
	});
}

//
//
//
TEST_CASE("Sequence. Sequence methods")
{
	testSequenceMethodsForSupportedType<Sequence>([](Variant vSeq)
	{
		return Sequence(vSeq);
	});
}

//////////////////////////////////////////////////////////////////////////
//
// Dictionary.
//

//
//
//
TEST_CASE("Dictionary. Default constructor")
{
	Dictionary vDict;
	REQUIRE(vDict.isEmpty());

	cmd::Variant vTest = vDict;
	REQUIRE(vTest.getType() == Variant::ValueType::Dictionary);
	REQUIRE(vTest.isEmpty());
}

//
//
//
TEST_CASE("Dictionary. Constructor from Variant contained dictionary")
{
	std::string sJson = R"json({
			"a": 1,
			"b": -1,
			"c": "str",
			"d": {"a":1},
			"e": [1, "a"],
			"f": true,
			"g": null
		})json";

	Variant vTest = variant::deserializeFromJson(sJson);
	REQUIRE(vTest.getType() == Variant::ValueType::Dictionary);
	Dictionary vDict(vTest);
	REQUIRE(Variant(vDict).getType() == Variant::ValueType::Dictionary);

	REQUIRE(vDict["f"] == true);
	vDict.put("XXX", 10);
	REQUIRE(vDict["XXX"] == 10);
}

//
//
//
TEST_CASE("Dictionary. Constructor from Variant contained not dictionary")
{
	Variant NonDicts[] = { Variant(nullptr), Variant(true), Variant(10), Variant("XXXX"), Sequence(), Variant(createObject<SimpleTestObject>()) };
	for (Variant& vNonDict : NonDicts)
	{
		REQUIRE(vNonDict.getType() != Variant::ValueType::Dictionary);
		REQUIRE_THROWS_AS([&]() {
			Dictionary vDict(vNonDict);
		}(), error::TypeError);
	}
}

//
//
//
TEST_CASE("Dictionary. Constructor from initializer list")
{
	Variant vDict = Dictionary({ {"aaa", 20} });

	Variant vTest = Dictionary({
		{"a", nullptr},
		{"b", true},
		{"c", 1u},
		{"d", -1},
		{"e", "str"},
		{"f", Variant(10)},
		{"g", vDict},
		{"h", Dictionary({ {"bbb", 20} })},
		{"j", Sequence({1, 2})},
		{"k", std::string("std::string")},
		{std::string("l"), 1},
		{std::string("m"), 1},
		{std::string("") + "n", 1}
		});

	REQUIRE(vTest.getType() == Variant::ValueType::Dictionary);
	REQUIRE(vTest.getSize() == 13);
	REQUIRE(vTest["a"].isNull());
	REQUIRE(vTest["b"] == true);
	REQUIRE(vTest["c"] == 1);
	REQUIRE(vTest["d"] == -1);
	REQUIRE(vTest["e"] == "str");
	REQUIRE(vTest["f"] == 10);
	REQUIRE(vTest["g"]["aaa"] == 20);
	REQUIRE(vTest["h"]["bbb"] == 20);
	REQUIRE(vTest["j"][0] == 1);
	REQUIRE(vTest["j"][1] == 2);
	REQUIRE(vTest["k"] == "std::string");
	REQUIRE(vTest["l"] == 1);
	REQUIRE(vTest["m"] == 1);
	REQUIRE(vTest["n"] == 1);
}

//
//
//
TEST_CASE("Dictionary. Dictionary methods")
{
	testDictionaryMethodsForSupportedType<Dictionary>([](Variant vDict)
	{
		return Dictionary(vDict);
	});
}

//
//
//
TEST_CASE("Dictionary. Iteration")
{
	SECTION("Enum values")
	{
		std::string sJson = R"json({
			"a": 1,
			"b": -1,
			"c": "str",
			"d": {"a":1},
			"e": [1, "a"],
			"f": true,
			"g": null
		})json";


		Dictionary vTest(cmd::variant::deserializeFromJson(sJson));
		REQUIRE(Variant(vTest).getType() == Variant::ValueType::Dictionary);

		auto fnChecker = [](const std::vector<size_t>& Counters)
		{
			REQUIRE(Counters[static_cast<size_t>(Variant::ValueType::Integer)] == 2);
			REQUIRE(Counters[static_cast<size_t>(Variant::ValueType::String)] == 1);
			REQUIRE(Counters[static_cast<size_t>(Variant::ValueType::Sequence)] == 1);
			REQUIRE(Counters[static_cast<size_t>(Variant::ValueType::Dictionary)] == 1);
			REQUIRE(Counters[static_cast<size_t>(Variant::ValueType::Boolean)] == 1);
			REQUIRE(Counters[static_cast<size_t>(Variant::ValueType::Null)] == 1);
		};

		{
			std::vector<size_t> Counters(c_nValueTypeCount);
			for (auto Item : vTest)
			{
				Counters[static_cast<size_t>(Variant(Item.second).getType())]++;
			}
			fnChecker(Counters);
		}

		{
			std::vector<size_t> Counters(c_nValueTypeCount);
			for (auto Iter = vTest.begin(); Iter != vTest.end(); ++Iter)
			{
				Counters[static_cast<size_t>(Variant(Iter->second).getType())]++;
			}
			fnChecker(Counters);
		}

		{
			std::vector<size_t> Counters(c_nValueTypeCount);
			for (auto Item : vTest.getUnsafeIterable())
			{
				Counters[static_cast<size_t>(Variant(Item.second).getType())]++;
			}
			fnChecker(Counters);
		}
	}

	SECTION("Enum keys")
	{
		std::string sJson = R"json({
			"a": 1,
			"b": -1,
			"c": "str",
			"d": {"a":1},
			"e": [1, "a"],
			"f": true,
			"g": null
		})json";

		Dictionary vTest(cmd::variant::deserializeFromJson(sJson));
		REQUIRE(Variant(vTest).getType() == Variant::ValueType::Dictionary);
		std::vector<std::string> ExpectedResult = { "a", "b", "c", "d", "e", "f", "g" };

		{
			std::vector<std::string> TestKeyList;
			for (auto Iter = vTest.begin(); Iter != vTest.end(); ++Iter)
			{
				TestKeyList.push_back(std::string(Iter->first));
			}

			std::sort(TestKeyList.begin(), TestKeyList.end());
			REQUIRE(TestKeyList == ExpectedResult);
		}

		{
			std::vector<std::string> TestKeyList;
			for (auto Item : vTest)
			{
				TestKeyList.push_back(std::string(Item.first));
			}

			std::sort(TestKeyList.begin(), TestKeyList.end());
			REQUIRE(TestKeyList == ExpectedResult);
		}
	}
}

//
//
//
TEST_CASE("Variant. queryInterface")
{
	// object type
	{
		ObjPtr<SimpleTestObject> pClass = createObject<SimpleTestObject>();
		Variant vTest(pClass);

		REQUIRE_NOTHROW([&]() {
			auto pObj = queryInterface<IObject>(vTest);
			REQUIRE(pObj->getId() == pClass->getId());
		}());

		REQUIRE_NOTHROW([&]() {
			auto pObj = queryInterface<ISimpleTestObject>(vTest);
			REQUIRE(pObj->getId() == pClass->getId());
		}());

		REQUIRE_NOTHROW([&]() {
			auto pObj = queryInterface<SimpleTestObject>(vTest);
			REQUIRE(pObj->getId() == pClass->getId());
		}());

		REQUIRE_THROWS_AS([&]() {
			auto pObj = queryInterface<INotImplementedInterface>(vTest);
		}(), std::runtime_error);
	}

	// other types
	{
		Variant NonObjects[] = { Variant(nullptr), Variant(true), Variant(10), Variant("XXXX"), Sequence(), Dictionary() };

		for (auto& vTest : NonObjects)
		{
			REQUIRE_THROWS_AS([&]() {
				auto pObj = queryInterface<IObject>(vTest);
			}(), error::TypeError);
		}
	}
}

//
//
//
TEST_CASE("Variant. queryInterfaceSafe")
{
	// object type
	{
		ObjPtr<SimpleTestObject> pClass = createObject<SimpleTestObject>();
		Variant vTest(pClass);

		REQUIRE_NOTHROW([&]() {
			auto pObj = queryInterfaceSafe<IObject>(vTest);
			REQUIRE_FALSE(pObj == nullptr);
			REQUIRE(pObj->getId() == pClass->getId());
		}());

		REQUIRE_NOTHROW([&]() {
			auto pObj = queryInterfaceSafe<ISimpleTestObject>(vTest);
			REQUIRE_FALSE(pObj == nullptr);
			REQUIRE(pObj->getId() == pClass->getId());
		}());

		REQUIRE_NOTHROW([&]() {
			auto pObj = queryInterfaceSafe<SimpleTestObject>(vTest);
			REQUIRE_FALSE(pObj == nullptr);
			REQUIRE(pObj->getId() == pClass->getId());
		}());

		REQUIRE_NOTHROW([&]() {
			auto pObj = queryInterfaceSafe<INotImplementedInterface>(vTest);
			REQUIRE(pObj == nullptr);
		}());
	}

	// other types
	{
		Variant NonObjects[] = { Variant(nullptr), Variant(true), Variant(10), Variant("XXXX"), Sequence(), Dictionary() };

		for (auto& vTest : NonObjects)
		{
			REQUIRE_NOTHROW([&]() {
				auto pObj = queryInterfaceSafe<INotImplementedInterface>(vTest);
				REQUIRE(pObj == nullptr);
			}());
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Variant. Object
//

//
//
//
TEST_CASE("Variant. Object. Conversion")
{
	// Constructor
	{
		ObjPtr<SimpleTestObject> pClass = createObject<SimpleTestObject>();
		Variant vTest(pClass);
		REQUIRE(vTest.getType() == Variant::ValueType::Object);

		ObjPtr<IObject> pObj = vTest;
		REQUIRE(pObj->getId() == pClass->getId());
	}

	// operator =
	{
		ObjPtr<SimpleTestObject> pClass = createObject<SimpleTestObject>();
		Variant vTest;
		vTest = pClass;
		REQUIRE(vTest.getType() == Variant::ValueType::Object);

		ObjPtr<IObject> pObj = vTest;
		REQUIRE(pObj->getId() == pClass->getId());
	}

	// operator Type
	{
		ObjPtr<SimpleTestObject> pClass = createObject<SimpleTestObject>();
		Variant vTest(pClass);
		REQUIRE(vTest.getType() == Variant::ValueType::Object);

		REQUIRE_NOTHROW([&]() {
			ObjPtr<IObject> pObj = vTest;
			REQUIRE(pObj->getId() == pClass->getId());
		}());

		REQUIRE_NOTHROW([&]() {
			ObjPtr<ISimpleTestObject> pObj = vTest;
			REQUIRE(pObj->getId() == pClass->getId());
		}());

		REQUIRE_NOTHROW([&]() {
			ObjPtr<SimpleTestObject> pObj = vTest;
			REQUIRE(pObj->getId() == pClass->getId());
		}());

		REQUIRE_THROWS_AS([&]() {
			ObjPtr<INotImplementedInterface> pObj = vTest;
		}(), std::runtime_error);
	}
}

//
//
//
TEST_CASE("Variant. Object. Iteration + size + empty")
{
	IterateLikeSalarType(Variant(createObject<SimpleTestObject>()));
}

//
//
//
TEST_CASE("Variant. Object. Sequence and Dictionary methods")
{
	REQUIRE_NOTHROW(testSequenceMethodsForNotsupportedType(Variant(createObject<SimpleTestObject>())));
	REQUIRE_NOTHROW(testDictionaryMethodsForNotsupportedType(Variant(createObject<SimpleTestObject>())));

	// clear
	REQUIRE_THROWS_AS([&]() {
		Variant vTest = Variant(createObject<SimpleTestObject>());
		vTest.clear();
	}(), error::TypeError);
}

//
//
//
TEST_CASE("Variant. Object. Clone")
{
	Variant vObj(createObject<SimpleTestObject>());
	Variant vObjClone = vObj.clone();

	CHECK(vObj == vObjClone);
}

//////////////////////////////////////////////////////////////////////////
//
// Dictionary Proxy
//

//
// TestDictionaryProxy is plain wrapper of Dictionary
//
class TestDictionaryProxy : public ObjectBase <>, public variant::IDictionaryProxy
{
	Dictionary m_Dict;

	class Iterator : public variant::IIterator
	{
		Dictionary::const_iterator m_Iter;
		Dictionary::const_iterator m_End;
	public:
		Iterator(Dictionary Dict, bool fUnsafe)
		{
			m_Iter = fUnsafe ? Dict.unsafeBegin() : Dict.begin();
			m_End = fUnsafe ? Dict.unsafeEnd() : Dict.end();
		}

		void goNext() override { ++m_Iter; }
		bool isEnd() const override { return !(m_Iter != m_End); }
		variant::DictionaryKeyRefType getKey() const override { return m_Iter->first; }
		const Variant getValue() const override { return m_Iter->second; }
	};

public:
	void finalConstruct(Variant vCfg)
	{
		m_Dict = Dictionary(vCfg);
	}

	Size getSize() const override { return m_Dict.getSize(); }
	bool isEmpty() const override { return m_Dict.isEmpty(); }
	bool has(variant::DictionaryKeyRefType sName) const override { return m_Dict.has(sName); }
	Size erase(variant::DictionaryKeyRefType sName) override { return m_Dict.erase(sName); }
	void clear() override { return m_Dict.clear(); }
	Variant get(variant::DictionaryKeyRefType sName) const override { return m_Dict.get(sName); }
	void put(variant::DictionaryKeyRefType sName, const Variant& Value) override { return m_Dict.put(sName, Value); }

	std::unique_ptr<variant::IIterator> createIterator(bool fUnsafe) const override
	{ 
		return std::make_unique<Iterator>(m_Dict, fUnsafe);
	}

	std::optional<Variant> getSafe(variant::DictionaryKeyRefType sName) const override
	{
		if (!m_Dict.has(sName))
			return {};
		return m_Dict.get(sName);
	}

	Variant getInternaValue()
	{
		return m_Dict;
	}
};

//
//
//
TEST_CASE("Variant. IDictionaryProxy. Dictionary methods")
{
	testDictionaryMethodsForSupportedType<Variant>([](Variant vDict)
	{
		return Variant(createObject<TestDictionaryProxy>(vDict));
	});
}

//
//
//
TEST_CASE("Variant.IDictionaryProxy_sequence_methods_support")
{
	testSequenceMethodsForNotsupportedType(Variant(createObject<TestDictionaryProxy>(Dictionary())));
}

//
//
//
TEST_CASE("Variant.IDictionaryProxy_comparison")
{
	#define TEST_DICTIONARY_VALUE Dictionary({ {"a", nullptr} , {"b", true}, {"c", Dictionary ({{"a", 1}}) }})

	Variant v1 = createObject<TestDictionaryProxy>(TEST_DICTIONARY_VALUE);
	Variant v2 = TEST_DICTIONARY_VALUE;

	CHECK(v1 == v2);
	CHECK(v2 == v1);
	CHECK_FALSE(v2 != v1);
	CHECK_FALSE(v1 != v2);

	v1.put("e", 100);

	CHECK_FALSE(v1 == v2);
	CHECK_FALSE(v2 == v1);
	CHECK(v2 != v1);
	CHECK(v1 != v2);

	v2.put("e", 200);

	CHECK_FALSE(v1 == v2);
	CHECK_FALSE(v2 == v1);
	CHECK(v2 != v1);
	CHECK(v1 != v2);

	#undef TEST_DICTIONARY_VALUE
}


//
//
//
TEST_CASE("Variant.IDictionaryProxy_iteration_support")
{
	Variant vSrc = Dictionary({
		{"a", nullptr},
		{"b", 1},
		{"c", "str"},
		});

	std::vector<Variant> Ethalons;
	{
		REQUIRE(vSrc.getSize() == 3);
		for (auto kv : Dictionary(vSrc))
			Ethalons.push_back(kv.second);
		REQUIRE(Ethalons.size() == 3);
	}

	Variant vTest(createObject<TestDictionaryProxy>(Dictionary(vSrc)));

	// size
	REQUIRE(vTest.getSize() == Ethalons.size());
	// empty
	REQUIRE(!vTest.isEmpty());

	// ranged for
	{
		size_t nIndex = 0;
		for (auto vValue : vTest)
		{
			REQUIRE(vValue == Ethalons.at(nIndex));
			++nIndex;
		}
		REQUIRE(nIndex == Ethalons.size());
	}

	// iterator
	{
		size_t nIndex = 0;
		for (auto Iter = vTest.begin(); Iter != vTest.end(); ++Iter)
		{
			REQUIRE(*Iter == Ethalons.at(nIndex));
			++nIndex;
		}
		REQUIRE(nIndex == Ethalons.size());
	}

	// unsafe ranged for
	{
		size_t nIndex = 0;
		for (auto vValue : vTest.getUnsafeIterable())
		{
			REQUIRE(vValue == Ethalons.at(nIndex));
			++nIndex;
		}
		REQUIRE(nIndex == Ethalons.size());
	}

	// unsafe iterator
	{
		size_t nIndex = 0;
		for (auto Iter = vTest.unsafeBegin(); Iter != vTest.unsafeEnd(); ++Iter)
		{
			REQUIRE(*Iter == Ethalons.at(nIndex));
			++nIndex;
		}
		REQUIRE(nIndex == Ethalons.size());
	}
}

//
//
//
//ctorVariantWithIDictionaryProxy
//constructor_from_Variant_with_IDictionaryProxy
TEST_CASE("Dictionary. Constructor from Variant contained IDictionaryProxy")
{
	std::string sJson = R"json({
			"a": 1,
			"b": -1,
			"c": "str",
			"d": {"a":1},
			"e": [1, "a"],
			"f": true,
			"g": null
		})json";

	Variant vTest = createObject<TestDictionaryProxy>(variant::deserializeFromJson(sJson));
	REQUIRE(vTest.getType() == Variant::ValueType::Object);
	REQUIRE(vTest.isDictionaryLike());

	Dictionary vDict(vTest);
	REQUIRE(Variant(vDict).getType() == Variant::ValueType::Object);
	REQUIRE(Variant(vDict).isDictionaryLike());

	REQUIRE(vDict["f"] == true);
	vDict.put("XXX", 10);
	REQUIRE(vDict["XXX"] == 10);
}

//
//
//
TEST_CASE("Sequence. Constructor from IDictionaryProxy")
{
	Variant vDictionaryProxy = createObject<TestDictionaryProxy>(Dictionary({ {"a", 1},  {"b", 10 },  {"c", 100 } }));

	Sequence seq(vDictionaryProxy);

	REQUIRE(Variant(seq).getType() == Variant::ValueType::Sequence);
	REQUIRE(seq.getSize() == 3);

	int nSum = 0;
	for (auto vVal : seq)
		nSum += int(vVal);
	REQUIRE(nSum == 111);
}

//////////////////////////////////////////////////////////////////////////
//
// Sequence Proxy
//

//
// TestSequenceProxy is plain wrapper of Dictionary
//
class TestSequenceProxy : public ObjectBase <>, public variant::ISequenceProxy
{
	Sequence m_Seq;

	class Iterator : public variant::IIterator
	{
		Sequence::const_iterator m_Iter;
		Sequence::const_iterator m_End;
	public:
		Iterator(Sequence Cont, bool fUnsafe)
		{
			m_Iter = fUnsafe ? Cont.unsafeBegin() : Cont.begin();
			m_End = fUnsafe ? Cont.unsafeEnd() : Cont.end();
		}

		virtual void goNext() override { ++m_Iter; }
		virtual bool isEnd() const override { return !(m_Iter != m_End); }
		virtual variant::DictionaryKeyRefType getKey() const override { error::TypeError(SL).throwException(); }
		virtual const Variant getValue() const override { return *m_Iter; }
	};

public:
	void finalConstruct(Variant vCfg)
	{
		m_Seq = Sequence(vCfg);
	}

	virtual Size getSize() const { return m_Seq.getSize(); }
	virtual bool isEmpty() const { return m_Seq.isEmpty(); }
	virtual void clear() { return m_Seq.clear(); }
	virtual Size erase(Size nIndex) override { return m_Seq.erase(nIndex); }
	virtual Variant get(Size nIndex) const override { return m_Seq.get(nIndex); }
	virtual void put(Size nIndex, const Variant& Value) override { return m_Seq.put(nIndex, Value); }
	virtual void push_back(const Variant& vValue) override { return m_Seq.push_back(vValue); }
	virtual void setSize(Size nSize) override { return m_Seq.setSize(nSize); }
	virtual void insert(Size nIndex, const Variant& vValue) override { return m_Seq.insert(nIndex, vValue); }

	virtual std::unique_ptr<variant::IIterator> createIterator(bool fUnsafe) const override
	{ 
		return std::make_unique<Iterator>(m_Seq, fUnsafe);
	}

	virtual std::optional<Variant> getSafe(Size nIndex) const override
	{
		if(nIndex >= m_Seq.getSize())
			return {};
		return m_Seq.get(nIndex);
	}
};

//
//
//
TEST_CASE("Variant. ISequenceProxy. Dictionary methods")
{
	testDictionaryMethodsForNotsupportedType(createObject<TestSequenceProxy>(Sequence({ 1,"str" })));
}

//
//
//
TEST_CASE("Variant. ISequenceProxy. Sequence methods")
{
	testSequenceMethodsForSupportedType<Variant>([](Variant vSeq)
	{
		return Variant(createObject<TestSequenceProxy>(vSeq));
	});
}

//
//
//
TEST_CASE("Variant. ISequenceProxy. Iteration")
{
	testSequenceIteration<Variant>([](Variant vSeq)
	{
		return Variant(createObject<TestSequenceProxy>(vSeq));
	});
}

//
//
//
TEST_CASE("Sequence. Constructor from ISequenceProxy")
{
	Variant vSeqProxy(createObject<TestSequenceProxy>(Sequence({ 1, "str" })));
	Sequence seq(vSeqProxy);

	REQUIRE(Variant(seq).getType() == Variant::ValueType::Object);
	REQUIRE(vSeqProxy.getSize() == 2);
	REQUIRE(Variant(seq) == vSeqProxy);

	// test both refers to same object
	seq.clear();
	REQUIRE(vSeqProxy.isEmpty());
}

//
//
//
TEST_CASE("Variant.ISequenceProxy_comparison")
{
	#define TEST_SEQUENCE_VALUE Sequence({nullptr, true, 1, "XXXX", Dictionary ({{"a", 1}})})

	Variant v1 = createObject<TestSequenceProxy>(TEST_SEQUENCE_VALUE);
	Variant v2 = TEST_SEQUENCE_VALUE;

	CHECK(v1 == v2);
	CHECK(v2 == v1);
	CHECK_FALSE(v2 != v1);
	CHECK_FALSE(v1 != v2);

	v1.push_back(1);

	CHECK_FALSE(v1 == v2);
	CHECK_FALSE(v2 == v1);
	CHECK(v2 != v1);
	CHECK(v1 != v2);

	v2.push_back(0);

	CHECK_FALSE(v1 == v2);
	CHECK_FALSE(v2 == v1);
	CHECK(v2 != v1);
	CHECK(v1 != v2);

	#undef TEST_SEQUENCE_VALUE
}



//////////////////////////////////////////////////////////////////////////
//
//
//
TEST_CASE("Variant.getByPath")
{
	SECTION("EmptyString")
	{
		Variant v = Dictionary({ {"a", 1} });
		REQUIRE(variant::getByPath(v, "") == v);
		REQUIRE(*variant::getByPathSafe(v, "") == v);
	}

	SECTION("SimpleDictKey")
	{
		Variant v = Dictionary({ {"abc", 1} });
		REQUIRE(variant::getByPath(v, "abc") == 1);
		REQUIRE(variant::getByPath(v, "abc", Variant()) == 1);
		REQUIRE(*variant::getByPathSafe(v, "abc") == 1);
	}

	SECTION("SimpleSeqKey")
	{
		Variant v = Sequence({"abc", 1});
		REQUIRE(variant::getByPath(v, "[1]") == 1);
		REQUIRE(variant::getByPath(v, "[1]", Variant()) == 1);
		REQUIRE(*variant::getByPathSafe(v, "[1]") == 1);
	}

	SECTION("ComplexKey")
	{
		Variant v = Dictionary({ 
			{"a", Sequence({
				1, 
				Dictionary({
					{"a", 1},
				})
			})},
		});

		REQUIRE(variant::getByPath(v, "a[1].a") == 1);
		REQUIRE(variant::getByPath(v, "a[1].a", Variant()) == 1);
		REQUIRE(*variant::getByPathSafe(v, "a[1].a") == 1);
	}

	SECTION("InvalidType")
	{
		Variant v(10);

		REQUIRE_THROWS_AS([&]() {
			(void)variant::getByPath(v, "a");
		}(), error::TypeError);

		REQUIRE(variant::getByPath(v, "a", Variant()).isNull());
		REQUIRE_FALSE(variant::getByPathSafe(v, "a").has_value());

		REQUIRE_THROWS_AS([&]() {
			(void)variant::getByPath(v, "[1]");
		}(), error::TypeError);

		REQUIRE(variant::getByPath(v, "[1]", Variant()).isNull());
		REQUIRE_FALSE(variant::getByPathSafe(v, "[1]").has_value());
	}

	SECTION("OutOfRange_seq")
	{
		Variant v = Sequence({1});

		REQUIRE_THROWS_AS([&]() {
			(void)variant::getByPath(v, "[1]");
		}(), error::OutOfRange);
		REQUIRE(variant::getByPath(v, "[1]", Variant()).isNull());
		REQUIRE_FALSE(variant::getByPathSafe(v, "[1]").has_value());
	}

	SECTION("OutOfRange_dict")
	{
		Variant v = Dictionary({ {"a", 1} });

		REQUIRE_THROWS_AS([&]() {
			(void)variant::getByPath(v, "b");
		}(), error::OutOfRange);
		REQUIRE(variant::getByPath(v, "b", Variant()).isNull());
		REQUIRE_FALSE(variant::getByPathSafe(v, "b").has_value());
	}

	SECTION("Invalid path")
	{
		Variant v = Sequence({ Dictionary({ {"a", 1} }) });

		std::string invalidPaths[] = { "[a]", ".", "[0]a", "[0]..a","[1000000000000000000000]" };


		for (auto sInvalidPath : invalidPaths)
		{
			CAPTURE(sInvalidPath);

			REQUIRE_THROWS_AS([&]() {
				(void)variant::getByPath(v, sInvalidPath);
			}(), error::InvalidArgument);
			REQUIRE_THROWS_AS([&]() {
				(void)variant::getByPath(v, sInvalidPath, Variant());
			}(), error::InvalidArgument);
			REQUIRE_THROWS_AS([&]() {
				(void)variant::getByPathSafe(v, sInvalidPath);
			}(), error::InvalidArgument);
		}

	}
}

//
//
//
TEST_CASE("Variant.putByPath")
{
	SECTION("SimpleDictKey")
	{
		Variant v = Dictionary({ {"aaa", 1} });
		variant::putByPath(v, "bbb", 2);
		REQUIRE(v == Dictionary({ {"aaa", 1}, {"bbb", 2} }));
	}

	SECTION("SimpleSeqKey")
	{
		Variant v = Sequence({0, 1, 2});
		variant::putByPath(v, "[1]", 100);
		REQUIRE(v == Sequence({ 0, 100, 2 }));
	}

	SECTION("ComplexKey")
	{
		Variant v = Dictionary({
			{"a", Sequence({
				1,
				Dictionary({
					{"a", 1},
				})
			})},
			});

		variant::putByPath(v, "a[1].b", 100);
		variant::putByPath(v, "a[0]", 200);

		Variant vEthalon = Dictionary({
			{"a", Sequence({
				200,
				Dictionary({
					{"a", 1},
					{"b", 100},
				})
			})},
			});

		REQUIRE(v == vEthalon);
	}

	SECTION("InvalidType")
	{
		Variant v(10);

		REQUIRE_THROWS_AS([&]() {
			variant::putByPath(v, "a", 100);
		}(), error::TypeError);

		REQUIRE_THROWS_AS([&]() {
			variant::putByPath(v, "[1]", 100);
		}(), error::TypeError);
	}

	SECTION("OutOfRange_seq")
	{
		Variant v = Sequence({ 1 });

		REQUIRE_THROWS_AS([&]() {
			variant::putByPath(v, "[1]", 100);
		}(), error::OutOfRange);
	}

	SECTION("Invalid path")
	{
		Variant v = Sequence();

		std::string invalidPaths[] = { "[a]", ".", "[0]a", "[1000000000000000000000]" };

		for (auto sInvalidPath : invalidPaths)
		{
			CAPTURE(sInvalidPath);
			REQUIRE_THROWS_AS([&]() {
				variant::putByPath(v, sInvalidPath, 100);
			}(), error::InvalidArgument);
		}

	}
}

//
//
//
TEST_CASE("Variant.putByPath_null")
{
	auto fnScenario = [&](Variant vData, std::string_view sPath, Variant vEthalon)
	{
		Variant vResult = vData.clone();
		variant::putByPath(vResult, sPath, nullptr);
		REQUIRE(vResult == vEthalon);
	};

	const std::map<std::string, std::tuple<Variant, std::string, Variant>> mapData{
		// sName: vData, sPath, vEthalon
		{ "dictionary", {
			Dictionary({ { "aaa", 1 }, { "bbb", 2 } }),
			"aaa",
			Dictionary({ { "aaa", nullptr }, { "bbb", 2 } })
			} },
		{ "sequence_exist", {
			Sequence({ 0, 1, 2 }),
			"[1]",
			Sequence({ 0, nullptr, 2 })
			} },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vData, sPath, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vData, sPath, vEthalon));
		}
	}
}

//
//
//
TEST_CASE("Variant.putByPath_create_paths")
{
	auto fnScenario = [&](Variant vData, std::string_view sPath, Variant vExpected)
	{
		Variant vResult = vData.clone();
		variant::putByPath(vResult, sPath, "x", /* fCreatePaths */ true);
		REQUIRE(vResult == vExpected);
	};

	const std::map<std::string, std::tuple<Variant, std::string, Variant>> mapData{
		// sName: vData, sPath, vExpected
		{ "dict_1", { Dictionary(), "a.b", Dictionary({ { "a", Dictionary({ { "b", "x" } }) } }) } },
		{ "dict_2", { Dictionary(), "a.b.c",
			Dictionary({ { "a", Dictionary({ { "b", Dictionary({ { "c", "x" } }) } }) } }) } },
		{ "mixed_1", { Dictionary({ { "a", Dictionary({ { "b", Sequence({ Dictionary() }) } }) } }),
			"a.b[0].c",
			Dictionary({ { "a", Dictionary({ { "b", Sequence({ Dictionary({ { "c", "x" } }) }) } }) } }) } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vData, sPath, vExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vData, sPath, vExpected));
		}
	}

	const std::map<std::string, std::tuple<std::string, Variant, std::string>> mapBad{
		// sName: sMsg, vData, sPath
		{ "dict_nonexistent", { "Invalid sequence index: 0. Size: 0",
			Dictionary(), "a[0]" } },
		{ "seq_nonexistent_1", { "put() is not applicable for current type <Sequence>. Key: <a>",
			Sequence(), "a[0]" } },
		{ "seq_nonexistent_2", { "Invalid sequence index <0>",
			Sequence(), "[0].a" } },
		{ "seq_nonexistent_3", { "Invalid sequence index <5>",
			Sequence(), "[5].a" } },
		{ "mixed_nonexistent_seq", { "Invalid sequence index: 0. Size: 0",
			Dictionary(), "a.b.c[0]" } },
			// Dictionary({ { "a", Dictionary({ { "b", Dictionary({ { "c", Sequence({ "x" }) } }) } }) } }) } },
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [sMsg, vData, sPath] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(vData, sPath, nullptr), sMsg);
		}
	}

}

//
//
//
TEST_CASE("Variant.deleteByPath")
{
	auto fnScenario = [&](Variant vData, std::string_view sPath, Variant vEthalon)
	{
		Variant vResult = vData.clone();
		variant::deleteByPath(vResult, sPath);
		REQUIRE(vResult == vEthalon);
	};

	const std::map<std::string, std::tuple<Variant, std::string, Variant>> mapData{
		// sName: vData, sPath, vExpected
		{ "dict_exist", {
			Dictionary({ { "aaa", 1 }, { "bbb", 2 } }),
			"bbb",
			Dictionary({ { "aaa", 1 } })
			} },
		{ "dict_not_exist_1", {
			Dictionary({ { "aaa", 1 }, { "bbb", 2 } }),
			"ccc",
			Dictionary({ { "aaa", 1 }, { "bbb", 2 } }),
			} },
		{ "dict_not_exist_2", {
			Dictionary({ { "a", 1 }, { "b", 2 } }),
			"a.a1.a11",
			Dictionary({ { "a", 1 }, { "b", 2 } }),
			} },
		{ "seq_exist", {
			Sequence({ 0, 1, 2 }),
			"[1]",
			Sequence({ 0, 2 })
			} },
		{ "seq_not_exist", {
			Sequence({ 0, 1, 2 }),
			"[3]",
			Sequence({ 0, 1, 2 }),
			} },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vData, sPath, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vData, sPath, vEthalon));
		}
	}
}

//
// Testing of dictionary merging for DictionaryLike types.
// Src and Dst can be different types
//
// @param fnCreateDst Function to create Dst from Dictionary
// @param fnCreateSrc Function to create Src from Dictionary
//
template<typename FnCreateDst, typename FnCreateSrc>
void testDictionaryMerging(FnCreateDst fnCreateDst, FnCreateSrc fnCreateSrc)
{
	// MergeMode::New
	{
		Variant vDst = fnCreateDst(Dictionary({ {"a", 1}, {"b", 1} }));
		Variant vSrc = fnCreateSrc(Dictionary({ {"b", 2}, {"c", 2} }));
		vDst.merge(vSrc, Variant::MergeMode::New);
		REQUIRE(vDst == Dictionary({ {"a", 1}, {"b", 1} , {"c", 2} }));
	}

	// MergeMode::Exist
	{
		Variant vDst = fnCreateDst(Dictionary({ {"a", 1}, {"b", 1} }));
		Variant vSrc = fnCreateSrc(Dictionary({ {"b", 2}, {"c", 2} }));
		vDst.merge( vSrc, Variant::MergeMode::Exist);
		REQUIRE(vDst == Dictionary({ {"a", 1}, {"b", 2} }));
	}

	// MergeMode::All
	{
		Variant vDst = fnCreateDst(Dictionary({ {"a", 1}, {"b", 1} }));
		Variant vSrc = fnCreateSrc(Dictionary({ {"b", 2}, {"c", 2} }));
		vDst.merge( vSrc, Variant::MergeMode::All);
		REQUIRE(vDst == Dictionary({ {"a", 1}, {"b", 2} , {"c", 2} }));
	}

	// MergeMode::CheckType
	{
		Variant vDst = fnCreateDst(Dictionary({ {"a", 1}, {"b", 1} }));
		Variant vSrc = fnCreateSrc(Dictionary({ {"b", 2}, {"c", 2} }));
		vDst.merge( vSrc, Variant::MergeMode::All | Variant::MergeMode::CheckType);
		REQUIRE(vDst == Dictionary({ {"a", 1}, {"b", 2} , {"c", 2} }));
	}

	// MergeMode::CheckType
	{
		Variant vDst = fnCreateDst(Dictionary({ {"a", 1}, {"b", 1} }));
		Variant vSrc = fnCreateSrc(Dictionary({ {"b", 2}, {"c", true} }));
		vDst.merge( vSrc, Variant::MergeMode::All | Variant::MergeMode::CheckType);
		REQUIRE(vDst == Dictionary({ {"a", 1}, {"b", 2} , {"c", true} }));
	}

	// MergeMode::CheckType
	{
		Variant vDst = fnCreateDst(Dictionary({ {"a", 1}, {"b", 1} }));
		Variant vSrc = fnCreateSrc(Dictionary({ {"b", true}, {"c", true} }));
		REQUIRE_THROWS_AS([&]() {
			vDst.merge( vSrc, Variant::MergeMode::All | Variant::MergeMode::CheckType);
		}(), error::InvalidArgument);
	}

	// MergeMode::MergeNestedDict
	{
		Variant vDst = fnCreateDst(Dictionary({ {"a", Dictionary({ {"a", 1}, {"b", 1}}) } }));
		Variant vSrc = fnCreateSrc(Dictionary({ {"a", Dictionary({ {"a", 2}, {"c", 2}}) } }));
		vDst.merge( vSrc, Variant::MergeMode::All);
		REQUIRE(vDst == Dictionary({ {"a", Dictionary({ {"a", 2}, {"c", 2}}) } }));
	}

	// MergeMode::MergeNestedDict
	{
		Variant vDst = fnCreateDst(Dictionary({ {"a", Dictionary({ {"a", 1}, {"b", 1}}) } }));
		Variant vSrc = fnCreateSrc(Dictionary({ {"a", Dictionary({ {"a", 2}, {"c", 2}}) } }));
		vDst.merge( vSrc, Variant::MergeMode::All | Variant::MergeMode::MergeNestedDict);
		REQUIRE(vDst == Dictionary({ {"a", Dictionary({ {"a", 2}, {"b", 1}, {"c", 2}}) } }));
	}

	// MergeMode::MergeNestedSeq
	{
		Variant vDst = fnCreateDst(Dictionary({ {"a", Sequence({1,2}) } }));
		Variant vSrc = fnCreateSrc(Dictionary({ {"a", Sequence({3,4}) } }));
		vDst.merge( vSrc, Variant::MergeMode::All);
		REQUIRE(vDst == Dictionary({ {"a", Sequence({3,4}) } }));
	}

	// delete with null
	{
		Variant vDst = fnCreateDst(Dictionary({ {"a", Dictionary({ {"a", 1}, {"b", 1}}) }, {"b", 1}, {"c", 1} }));
		Variant vSrc = fnCreateSrc(Dictionary({ {"a", Dictionary({ {"b", nullptr}}) }, {"b", nullptr}, {"d", nullptr} }));
		vDst.merge( vSrc, Variant::MergeMode::All | Variant::MergeMode::MergeNestedDict);
		REQUIRE(vDst == Dictionary({ {"a", Dictionary({ {"a", 1}}) }, {"c", 1} }));
	}

	// MergeMode::NonErasingNulls
	{
		Variant vDst = fnCreateDst(Dictionary({ {"a", Dictionary({ {"a", 1}, {"b", 1}}) }, {"b", 1}, {"c", 1} }));
		Variant vSrc = fnCreateSrc(Dictionary({ {"a", Dictionary({ {"b", nullptr}}) }, {"b", nullptr}, {"d", nullptr} }));
		vDst.merge( vSrc, Variant::MergeMode::All | Variant::MergeMode::MergeNestedDict |
			Variant::MergeMode::NonErasingNulls);
		REQUIRE(vDst == Dictionary({
			{"a", Dictionary({ {"a", 1}, {"b", nullptr} })},
			{"b", nullptr},
			{"c", 1},
			{"d", nullptr} 
			}));
	}
}


//
//
//
TEST_CASE("Variant.merge")
{
	SECTION("merging_with_null")
	{
		Variant DstList[] = {
			// Scalar
			Variant(), Variant(true), Variant(1), Variant("XXXX"),
			// Dictionary and Sequence
			Dictionary({{"a", 1}}), Sequence({"a", 1}),
			// Simple object
			Variant(createObject<SimpleTestObject>()),
			// Dictionary proxy
			createObject<TestDictionaryProxy>(Dictionary({ {"a", 1} })),
			// Sequence proxy
			createObject<TestSequenceProxy>(Sequence({ 1, "str" })),
		};

		for (auto vDst : DstList)
		{
			INFO("v type = " << (int)vDst.getType());
			REQUIRE_NOTHROW([&]() {
				CHECK(vDst.clone().merge(Variant(nullptr), Variant::MergeMode::CheckType).isNull());
			}());
		}
	}

	SECTION("scalar_merging")
	{
		Variant DstList[] = {
			// Scalar
			Variant(), Variant(true), Variant(1), Variant("XXXX"),
			// Dictionary and Sequence
			Dictionary({{"a", 1}}), Sequence({"a", 1}),
			// Simple object
			Variant(createObject<SimpleTestObject>()),
			// Dictionary proxy
			createObject<TestDictionaryProxy>(Dictionary({ {"a", 1} })),
			// Sequence proxy
			createObject<TestSequenceProxy>(Sequence({ 1, "str" })),
		};

		Variant SrcList[] = { Variant(true), Variant(1), Variant("XXXX"), Variant(createObject<SimpleTestObject>()) };

		for (auto vDst : DstList)
		{
			for (auto vSrc : SrcList)
			{
				INFO("vDst type = " << (int) vDst.getType());
				INFO("vSrc type = " << (int) vSrc.getType());

				CHECK(vDst.clone().merge(vSrc, Variant::MergeMode::All) == vSrc);

				if (vSrc.getType() == vDst.getType())
				{
					REQUIRE_NOTHROW([&]() {
						CHECK(vDst.clone().merge(vSrc, Variant::MergeMode::CheckType) == vSrc);
					}());
				}
				else
				{
					REQUIRE_THROWS_AS([&]() {
						vDst.clone().merge(vSrc, Variant::MergeMode::CheckType);
					}(), error::InvalidArgument);
				}
			}
		}
	}

	SECTION("sequence_merging_into_sequence")
	{
		Variant SrcList[] = { Sequence({1,2}), createObject<TestSequenceProxy>(Sequence({ 1, 2 })) };

		Variant vDst = Sequence({ 1,2 });

		for (auto vSrc : SrcList)
		{
			vDst.merge(vSrc, Variant::MergeMode::All);
		}
		REQUIRE(vDst == Sequence({ 1,2,1,2,1,2 }));
	}

	SECTION("sequence_merging_into_sequence_proxy")
	{
		Variant vInternalSeq = Sequence({ 1, 2 });
		Variant vDst = createObject<TestSequenceProxy>(vInternalSeq);

		Variant SrcList[] = { Sequence({1,2}), createObject<TestSequenceProxy>(Sequence({ 1, 2 })) };

		for (auto vSrc : SrcList)
		{
			vDst.merge(vSrc, Variant::MergeMode::All);
		}

		REQUIRE(vInternalSeq == Sequence({ 1,2,1,2,1,2 }));
	}

	SECTION("dictionary_merging_into_dictionary")
	{
		auto fnBasicDictionaryCreator = [](Dictionary dict) { return dict.clone(); };
		auto fnDictionaryProxyCreator = [](Dictionary dict) { return Variant(createObject<TestDictionaryProxy>(dict)); };

		std::function<Variant(Dictionary) > fnCreators[] = { fnBasicDictionaryCreator, fnDictionaryProxyCreator };

		for (auto fnCreator1 : fnCreators)
			for (auto fnCreator2 : fnCreators)
				testDictionaryMerging(fnCreator1, fnCreator2);
	}
}


//
//
//
class SimpleTestObjectWithClsid12345678 : public ObjectBase <ClassId(0x12345678)>
{
};

//
//
//
class ExamplePrintableObject : public ObjectBase<>, public IPrintable
{
public:
	
	virtual std::string print() const override
	{
		return "ExamplePrintableObject_representation";
	}

};


//
//
//
void testVariantPrint(Variant v, std::string sEthalon)
{
	INFO("type of v: " << v.getType());
	std::string sPrintResult = v.print();
	CHECK(sPrintResult == sEthalon);
}

//
//
//
TEST_CASE("Variant.print")
{
	SECTION("null")
	{
		testVariantPrint(Variant(nullptr), "null");
	}

	SECTION("boolean")
	{
		testVariantPrint(Variant(true), "true");
		testVariantPrint(Variant(false), "false");
	}

	SECTION("int")
	{
		testVariantPrint(Variant(1), "1");
		testVariantPrint(Variant(-1), "-1");
	}

	SECTION("string")
	{
		testVariantPrint(Variant("abc123"), "abc123");
		testVariantPrint(Variant(L"abc\"123"), "abc\"123");
	}

	SECTION("simple_object")
	{
		ObjPtr<IObject> pObj = createObject<SimpleTestObjectWithClsid12345678>();
		std::string sEthalon = FMT("Object<" << SimpleTestObjectWithClsid12345678::c_nClassId << ":" << hex(pObj->getId()) << ">");
		testVariantPrint(Variant(pObj), sEthalon);
	}

	SECTION("IDictionaryProxy")
	{
		ObjPtr<IObject> pObj = createObject<TestDictionaryProxy>(Dictionary());
		std::string sEthalon = FMT("Object<0xFFFFFFFF:"<< hex(pObj->getId()) << ">(IDictionaryProxy)");
		testVariantPrint(Variant(pObj), sEthalon);
		
	}

	SECTION("IPrintable")
	{
		ObjPtr<IObject> pObj = createObject<ExamplePrintableObject>();
		testVariantPrint(Variant(pObj), "ExamplePrintableObject_representation");
	}

	SECTION("sequence")
	{
		testVariantPrint(Sequence(), "[]");
		testVariantPrint(Sequence({ nullptr, 1, true, "str" }), "[\n    null,\n    1,\n    true,\n    \"str\",\n]");
	}

	SECTION("dictionary")
	{
		testVariantPrint(Dictionary(), "{}");
		testVariantPrint(Dictionary({ {"a",nullptr} , {"b", 1}, {"c", "str"} }), 
			"{\n    \"a\": null,\n    \"b\": 1,\n    \"c\": \"str\",\n}");
	}

	SECTION("indentation")
	{
		testVariantPrint(Dictionary({ {"a",Sequence({1,2})} , {"b", Dictionary()}, {"c", Dictionary({{"a", 1}})} }),
			"{\n    \"a\": [\n        1,\n        2,\n    ],\n    \"b\": {},\n    \"c\": {\n        \"a\": 1,\n    },\n}");
	}
}

//
//
//
TEST_CASE("Variant.operator_output")
{
	Variant testList[] = { nullptr, true, 1, "str", createObject<SimpleTestObjectWithClsid12345678>(), 
		Dictionary({{"a", 1}}), Sequence({1,2}) };

	for (auto v : testList)
	{
		std::string sPrinted = v.print();

		std::ostringstream oStrStream;
		oStrStream << v;
		CHECK(oStrStream.str() == sPrinted);
	}
}