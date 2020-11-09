//
//  EDRAv2. Unit test
//    Variant.DictionaryKey
//
#include "pch.h"

using namespace openEdr;

TEST_CASE("Variant.DictKey")
{
	using DictKey = variant::detail::DictKey;

	SECTION("default_constructor")
	{
		DictKey dk;
		REQUIRE(dk.convertToStr() == "");
	}

	SECTION("copy")
	{
		DictKey dk1("abc");

		DictKey dk2(dk1);
		REQUIRE(dk2.convertToStr() == "abc");

		DictKey dk3;
		dk3 = dk1;
		REQUIRE(dk3.convertToStr() == "abc");
	}

	SECTION("create_from_string")
	{
		auto fnTest = [](const char* str, bool fPutToStorage, bool fTestExtract)
		{
			DictKey dkArr[] = {
				DictKey(str, fPutToStorage),
				DictKey(std::string_view(str), fPutToStorage),
				DictKey(std::string(str), fPutToStorage)
			};

			for (auto dk : dkArr)
			{
				for (auto other : dkArr)
					REQUIRE(dk == other);

				if (fTestExtract)
					REQUIRE(dk.convertToStr() == str);
			}
		};

		const char* shortstrings [] = { "", "123", "1234567"};
		for (auto str : shortstrings)
		{
			fnTest(str, true, true);
			fnTest(str, false, true);
		}

		const char* longstrings[] = { 
			"12345678", 
			"1111111111111111111111111111111111",
			"11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
		};
		for (auto str : longstrings)
		{
			fnTest(str, true, true);
			fnTest(str, false, false);
		}
	}

	SECTION("convertToStr")
	{
		// short string 
		REQUIRE(DictKey("UTSFCTS").convertToStr() == "UTSFCTS");
		// long string NOT in storage
		REQUIRE_THROWS([]() { (void)DictKey("UniqueLongTestStringFor_convertToStr").convertToStr(); }());

		// long string in storage
		DictKey("UniqueLongTestStringFor_convertToStr", true);
		REQUIRE(DictKey("UniqueLongTestStringFor_convertToStr").convertToStr() == 
			"UniqueLongTestStringFor_convertToStr");
	}

	SECTION("compare")
	{
		std::string_view strings[] = { "", "123", "1234567", "12345678", "1111111111111111111111111111" };

		for (auto str1 : strings)
			for (auto str2 : strings)
			{
				bool fEqual = str1 == str2;
				REQUIRE((DictKey(str1) == DictKey(str2)) == fEqual);
				REQUIRE((DictKey(str1) != DictKey(str2)) == !fEqual);
			}
	}
}

#if 0
struct I
{
	virtual size_t virtual_inc() = 0;
};


struct A
{
	size_t n = 0;

	size_t inc()
	{
		return ++n;
	}
};

struct B
{
	size_t n = 0;

	size_t inc()
	{
		return ++n;
	}
};

struct C
{
	size_t n = 0;

	size_t inc()
	{
		return ++n;
	}
};

struct AV : public I
{
	size_t n = 0;

	size_t inc()
	{
		return ++n;
	}

	size_t virtual_inc() override
	{
		return ++n;
	}
};

struct BV : public I
{
	size_t n = 0;

	size_t inc()
	{
		return ++n;
	}

	size_t virtual_inc() override
	{
		return ++n;
	}
};

struct CV : public I
{
	size_t n = 0;

	size_t inc()
	{
		return ++n;
	}

	size_t virtual_inc() override
	{
		return ++n;
	}
};


typedef boost::variant<A, B, C> V;



struct visitor
	: public boost::static_visitor<size_t>
{
	template <typename T>
	size_t operator()(T& val) const
	{
		return val.inc();
	}
};


TEST_CASE("BoostVariant_speed")
{
	SECTION("Virtual")
	{
		std::vector<std::unique_ptr<I> > arr;
		arr.emplace_back(new AV);
		arr.emplace_back(new BV);
		arr.emplace_back(new CV);
		arr.emplace_back(new AV);
		arr.emplace_back(new BV);
		arr.emplace_back(new CV);

		auto startTime = std::chrono::high_resolution_clock::now();

		for (size_t i = 0; i < 10000000; ++i)
			for (auto& elem : arr)
				elem->virtual_inc();

		auto us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime);

		FAIL("us " << us.count());
	}

	SECTION("BoostVariant lambda visitor")
	{
		std::vector<V> arr = { A(), B(), C(), A(), B(), C() };

		auto startTime = std::chrono::high_resolution_clock::now();

		for (size_t i = 0; i < 10000000; ++i)
			for (auto& elem : arr)
				boost::apply_visitor([](auto&& val) { return val.inc(); }, elem);

		auto us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime);

		FAIL("us " << us.count());
	}

	SECTION("BoostVariant switch")
	{
		std::vector<V> arr = { A(), B(), C(), A(), B(), C() };

		auto startTime = std::chrono::high_resolution_clock::now();

		for (size_t i = 0; i < 10000000; ++i)
			for (auto& elem : arr)
			{
				switch (elem.which())
				{
				case 0:
					boost::get<A>(&elem)->inc();
					break;
				case 1:
					boost::get<B>(&elem)->inc();
					break;
				case 2:
					boost::get<C>(&elem)->inc();
					break;
				}
				
			}

		auto us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime);

		FAIL("us " << us.count());
	}

	//SECTION("BoostVariant class visitor")
	//{
	//	std::vector<V> arr = { A(), B(), C(), A(), B(), C() };
	//
	//	auto startTime = std::chrono::high_resolution_clock::now();
	//	
	//	for (size_t i = 0; i < 10000000; ++i)
	//		for (auto& elem : arr)
	//			boost::apply_visitor(visitor(), elem);
	//
	//	auto us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime);
	//
	//	FAIL("us " << us.count());
	//}
}

#endif