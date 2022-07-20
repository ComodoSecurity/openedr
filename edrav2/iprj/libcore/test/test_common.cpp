//
//  EDRAv2. Unit test
//    common.hpp test
//
#include "pch.h"

using namespace cmd;

//
//
//
CMD_DEFINE_ENUM_FLAG(ExampleEnum)
{
	Elem1 = 1 << 0,
	Elem2 = 1 << 1,
	Elem4 = 1 << 2,
	Elem8 = 1 << 3,
	Elem16 = 1 << 4,
};

typedef std::uint32_t ExampleEnumUnderlyingType;

//
//
//
TEST_CASE("flags.castToUnderlying")
{
	REQUIRE(castToUnderlying(ExampleEnum::Elem2) == 2);
	
	ExampleEnum e = ExampleEnum::Elem4;
	REQUIRE(castToUnderlying(e) == 4);
}

//
//
//
TEST_CASE("flags.castToUnderlyingPtr")
{
	ExampleEnum e = ExampleEnum::Elem4;
	ExampleEnumUnderlyingType* pUT = castToUnderlyingPtr(&e);
	REQUIRE((void*) &e == (void*) pUT);
}

//
//
//
TEST_CASE("flags.CMD_DEFINE_ENUM_FLAG")
{
	// Checking of underlying type
	REQUIRE(std::is_same_v<std::underlying_type_t<ExampleEnum>, ExampleEnumUnderlyingType>);
	REQUIRE(sizeof(ExampleEnum) == sizeof(ExampleEnumUnderlyingType));

	// Checking of elements
	REQUIRE(ExampleEnum::Elem1 == ExampleEnum(1));
	REQUIRE(ExampleEnum::Elem2 == ExampleEnum(2));
	REQUIRE(ExampleEnum::Elem4 == ExampleEnum(4));
	REQUIRE(ExampleEnum::Elem8 == ExampleEnum(8));
	REQUIRE(ExampleEnum::Elem16 == ExampleEnum(16));

}

//
//
//
TEST_CASE("flags.CMD_DEFINE_ENUM_FLAG_OPERATORS")
{
	// or
	{
		ExampleEnum e = ExampleEnum::Elem2 | ExampleEnum::Elem4;
		REQUIRE(e == ExampleEnum(2 + 4));
		e |= ExampleEnum::Elem8;
		REQUIRE(e == ExampleEnum(2 + 4 + 8));
	}

	// and
	{
		ExampleEnum e =
			(ExampleEnum::Elem2 | ExampleEnum::Elem4 | ExampleEnum::Elem8) &
			(ExampleEnum::Elem4 | ExampleEnum::Elem8 | ExampleEnum::Elem16);
		REQUIRE(e == ExampleEnum(4 + 8));
		e &= ExampleEnum::Elem4;
		REQUIRE(e == ExampleEnum::Elem4);
		e &= ExampleEnum::Elem8;
		REQUIRE(e == ExampleEnum(0));
	}

	// xor
	{
		ExampleEnum e = (ExampleEnum::Elem2 | ExampleEnum::Elem4) ^ (ExampleEnum::Elem4 | ExampleEnum::Elem8);
		REQUIRE(e == ExampleEnum(2 + 8));
		e ^= ExampleEnum::Elem8;
		REQUIRE(e == ExampleEnum::Elem2);
	}

	// not
	{
		ExampleEnum e = ~ExampleEnum::Elem1;
		REQUIRE(ExampleEnumUnderlyingType(e) == ExampleEnumUnderlyingType(~1));
	}
}

//
//
//
TEST_CASE("flags.testFlag")
{
	ExampleEnum e = ExampleEnum::Elem2 | ExampleEnum::Elem4;
	REQUIRE(testFlag(e, ExampleEnum::Elem2));
	REQUIRE(testFlag(e, ExampleEnum::Elem4));

	REQUIRE_FALSE(testFlag(e, ExampleEnum::Elem1));
	REQUIRE_FALSE(testFlag(e, ExampleEnum::Elem8));
	REQUIRE_FALSE(testFlag(e, ExampleEnum::Elem16));
}

//
//
//
TEST_CASE("flags.setFlag")
{
	ExampleEnum e = ExampleEnum::Elem2;
	REQUIRE(setFlag(e, ExampleEnum::Elem4) == ExampleEnum(2 + 4));
	REQUIRE(e == ExampleEnum(2 + 4));
}

//
//
//
TEST_CASE("flags.resetFlag")
{
	ExampleEnum e = ExampleEnum::Elem2 | ExampleEnum::Elem4;
	REQUIRE(resetFlag(e, ExampleEnum::Elem4) == ExampleEnum::Elem2);
	REQUIRE(e == ExampleEnum::Elem2);

	const ExampleEnum eConst = ExampleEnum::Elem2 | ExampleEnum::Elem4;
	REQUIRE(resetFlag(eConst, ExampleEnum::Elem4) == ExampleEnum::Elem2);
	REQUIRE(eConst == ExampleEnum(2 + 4));
}

//
//
//
TEST_CASE("flags.testMask")
{
	ExampleEnum e = ExampleEnum::Elem2 | ExampleEnum::Elem4 | ExampleEnum::Elem8;
	REQUIRE(testMask(e, ExampleEnum::Elem4));
	REQUIRE(testMask(e, ExampleEnum::Elem2 | ExampleEnum::Elem4));
	REQUIRE(testMask(e, ExampleEnum::Elem2 | ExampleEnum::Elem4 | ExampleEnum::Elem8));

	REQUIRE_FALSE(testMask(e, ExampleEnum::Elem1));
	REQUIRE_FALSE(testMask(e, ExampleEnum::Elem1 | ExampleEnum::Elem2));
	REQUIRE_FALSE(testMask(e, ExampleEnum::Elem1 | ExampleEnum::Elem2 | ExampleEnum::Elem4));
	REQUIRE_FALSE(testMask(e, ExampleEnum::Elem2 | ExampleEnum::Elem4 | ExampleEnum::Elem16));
}

//
//
//
TEST_CASE("flags.testMaskAny")
{
	ExampleEnum e = ExampleEnum::Elem2 | ExampleEnum::Elem4 | ExampleEnum::Elem8;
	REQUIRE(testMaskAny(e, ExampleEnum::Elem4));
	REQUIRE(testMaskAny(e, ExampleEnum::Elem2 | ExampleEnum::Elem4));
	REQUIRE(testMaskAny(e, ExampleEnum::Elem2 | ExampleEnum::Elem4 | ExampleEnum::Elem8));
	REQUIRE(testMaskAny(e, ExampleEnum::Elem1 | ExampleEnum::Elem2));
	REQUIRE(testMaskAny(e, ExampleEnum::Elem1 | ExampleEnum::Elem2 | ExampleEnum::Elem4));
	REQUIRE(testMaskAny(e, ExampleEnum::Elem2 | ExampleEnum::Elem4 | ExampleEnum::Elem16));

	REQUIRE_FALSE(testMaskAny(e, ExampleEnum::Elem1));
	REQUIRE_FALSE(testMaskAny(e, ExampleEnum::Elem16));
}
