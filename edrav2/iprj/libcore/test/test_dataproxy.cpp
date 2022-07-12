//
//  EDRAv2. Unit test
//    Variant.
//
#include "pch.h"

using namespace cmd;

CMD_DECLARE_LIBRARY_CLSID(CLSID_FinalConstructCounter, 0xE52D3F50);
CMD_DECLARE_LIBRARY_CLSID(CLSID_CommandProcessorExecuteCounter, 0xE52D3F51);

namespace test_dataproxy {

//
//
//
class SimpleTestObject : public ObjectBase <>
{
};

//
//
//
class FinalConstructCounter : public ObjectBase<CLSID_FinalConstructCounter>
{
public:
	void finalConstruct(Variant vCfg)
	{
		++g_nActionCount;
	}

	static size_t g_nActionCount;

	static Variant createDataProxy(bool fCached)
	{
		return createObjProxy(Dictionary({ {"clsid", CLSID_FinalConstructCounter} }), fCached);
	}
};
size_t FinalConstructCounter::g_nActionCount = 0;

//
//
//
class CommandProcessorExecuteCounter : public ObjectBase<CLSID_CommandProcessorExecuteCounter>, public ICommandProcessor
{
public:
	virtual Variant execute(Variant vCommand, Variant vParams) override
	{
		++g_nActionCount;
		if (vCommand == "passParams")
		{
			return vParams;
		}

		error::InvalidArgument(SL).throwException();
	}

	static Variant createDataProxy(bool fCached, Variant vCommand, Variant vParam)
	{
		return createCmdProxy(Dictionary({
			{"processor", Dictionary({{"clsid", CLSID_CommandProcessorExecuteCounter}}) },
			{"command", vCommand},
			{"params", vParam},
			}), fCached);
	}

	static size_t g_nActionCount;
};
size_t CommandProcessorExecuteCounter::g_nActionCount = 0;

} // namespace test_dataproxy

using namespace test_dataproxy;

CMD_DEFINE_LIBRARY_CLASS(FinalConstructCounter)
CMD_DEFINE_LIBRARY_CLASS(CommandProcessorExecuteCounter)

using cmd::variant::createCmdProxy;
using cmd::variant::createObjProxy;
using cmd::variant::createLambdaProxy;
using cmd::variant::detail::RawValueType;
using cmd::variant::ValueType;
using cmd::variant::detail::getRawType;

//
//
//
TEST_CASE("Variant_DataProxy.createObjDataProxy")
{
	SECTION("cached")
	{
		FinalConstructCounter::g_nActionCount = 0;
		Variant vDataProxy = FinalConstructCounter::createDataProxy(true);
		Variant vDataProxy2 = vDataProxy;

		// Check not calculated Variant
		REQUIRE(getRawType(vDataProxy) == RawValueType::DataProxy);
		REQUIRE(FinalConstructCounter::g_nActionCount == 0);

		// Check calculation
		REQUIRE(vDataProxy.getType() == ValueType::Object);
		REQUIRE(getRawType(vDataProxy) == RawValueType::Object);
		auto pData = queryInterfaceSafe<FinalConstructCounter>(vDataProxy);
		REQUIRE(pData != nullptr);
		REQUIRE(FinalConstructCounter::g_nActionCount == 1);

		// Check calculation of duplicate
		REQUIRE(getRawType(vDataProxy2) == RawValueType::DataProxy);
		REQUIRE(vDataProxy2.getType() == ValueType::Object);
		REQUIRE(getRawType(vDataProxy2) == RawValueType::Object);
		auto pData2 = queryInterfaceSafe<FinalConstructCounter>(vDataProxy2);
		REQUIRE(pData2 != nullptr);
		REQUIRE(FinalConstructCounter::g_nActionCount == 1);

		REQUIRE(pData->getId() == pData2->getId());
	}

	SECTION("noncached")
	{
		FinalConstructCounter::g_nActionCount = 0;
		Variant vDataProxy = FinalConstructCounter::createDataProxy(false);
		Variant vDataProxy2 = vDataProxy;

		// Check not calculated Variant
		REQUIRE(getRawType(vDataProxy) == RawValueType::DataProxy);
		REQUIRE(FinalConstructCounter::g_nActionCount == 0);

		// Check calculation
		REQUIRE(vDataProxy.getType() == ValueType::Object);
		REQUIRE(getRawType(vDataProxy) == RawValueType::Object);
		auto pData = queryInterfaceSafe<FinalConstructCounter>(vDataProxy);
		REQUIRE(pData != nullptr);
		REQUIRE(FinalConstructCounter::g_nActionCount == 1);

		// Check calculation of duplicate
		REQUIRE(getRawType(vDataProxy2) == RawValueType::DataProxy);
		REQUIRE(vDataProxy2.getType() == ValueType::Object);
		REQUIRE(getRawType(vDataProxy2) == RawValueType::Object);
		auto pData2 = queryInterfaceSafe<FinalConstructCounter>(vDataProxy2);
		REQUIRE(pData2 != nullptr);
		REQUIRE(FinalConstructCounter::g_nActionCount == 2);

		REQUIRE(pData->getId() != pData2->getId());
	}

}

//
//
//
TEST_CASE("Variant_DataProxy.createCmdDataProxy")
{
	SECTION("cached")
	{
		CommandProcessorExecuteCounter::g_nActionCount = 0;
		Variant vDataProxy = CommandProcessorExecuteCounter::createDataProxy(true, "passParams", Dictionary({ {"a", 1} }));
		Variant vDataProxy2 = vDataProxy;

		// Check not calculated Variant
		REQUIRE(getRawType(vDataProxy) == RawValueType::DataProxy);
		REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 0);

		// Check calculation
		REQUIRE(vDataProxy.getType() == ValueType::Dictionary);
		REQUIRE(getRawType(vDataProxy) == RawValueType::Dictionary);
		Variant vData = vDataProxy;
		REQUIRE(vData == Dictionary({ {"a", 1} }));
		REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 1);

		// Check calculation of duplicate
		REQUIRE(getRawType(vDataProxy2) == RawValueType::DataProxy);
		REQUIRE(vDataProxy2.getType() == ValueType::Dictionary);
		REQUIRE(getRawType(vDataProxy2) == RawValueType::Dictionary);
		Variant vData2 = vDataProxy2;
		REQUIRE(vData2 == Dictionary({ {"a", 1} }));
		REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 1);

		vData.put("b", 2);
		REQUIRE(vData2 == Dictionary({ {"a", 1}, {"b", 2} }));
	}

	SECTION("noncached")
	{
		CommandProcessorExecuteCounter::g_nActionCount = 0;
		Variant vDataProxy = CommandProcessorExecuteCounter::createDataProxy(false, "passParams", Dictionary({ {"a", 1} }));
		Variant vDataProxy2 = vDataProxy;

		// Check not calculated Variant
		REQUIRE(getRawType(vDataProxy) == RawValueType::DataProxy);
		REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 0);

		// Check calculation
		REQUIRE(vDataProxy.getType() == ValueType::Dictionary);
		REQUIRE(getRawType(vDataProxy) == RawValueType::Dictionary);
		Variant vData = vDataProxy;
		REQUIRE(vData == Dictionary({ {"a", 1} }));
		REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 1);

		// Check calculation of duplicate
		REQUIRE(getRawType(vDataProxy2) == RawValueType::DataProxy);
		REQUIRE(vDataProxy2.getType() == ValueType::Dictionary);
		REQUIRE(getRawType(vDataProxy2) == RawValueType::Dictionary);
		Variant vData2 = vDataProxy2;
		REQUIRE(vData2 == Dictionary({ {"a", 1} }));
		REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 2);

		vData.put("b", 2);
		REQUIRE(vData2 == Dictionary({ {"a", 1} }));
	}

}

//
//
//
TEST_CASE("Variant_DataProxy.createLambdaProxy")
{
	SECTION("cached")
	{
		Size nCalculationCount = 0;

		auto fnCalculator = [&nCalculationCount]() -> Variant
		{
			++nCalculationCount;
			return 1;
		};

		Variant vDataProxy = createLambdaProxy(fnCalculator, true);
		Variant vDataProxy2 = vDataProxy;

		// Check not calculated Variant
		REQUIRE(getRawType(vDataProxy) == RawValueType::DataProxy);
		REQUIRE(nCalculationCount == 0);

		// Check calculation
		REQUIRE(vDataProxy.getType() == ValueType::Integer);
		REQUIRE(getRawType(vDataProxy) == RawValueType::Integer);
		REQUIRE(vDataProxy == 1);
		REQUIRE(nCalculationCount == 1);

		// Check calculation of duplicate
		REQUIRE(getRawType(vDataProxy2) == RawValueType::DataProxy);
		REQUIRE(vDataProxy2.getType() == ValueType::Integer);
		REQUIRE(getRawType(vDataProxy2) == RawValueType::Integer);
		REQUIRE(vDataProxy2 == 1);
		REQUIRE(nCalculationCount == 1);
	}

	SECTION("noncached")
	{
		Size nCalculationCount = 0;

		auto fnCalculator = [&nCalculationCount]() -> Variant
		{
			++nCalculationCount;
			return 1;
		};

		Variant vDataProxy = createLambdaProxy(fnCalculator, false);
		Variant vDataProxy2 = vDataProxy;

		// Check not calculated Variant
		REQUIRE(getRawType(vDataProxy) == RawValueType::DataProxy);
		REQUIRE(nCalculationCount == 0);

		// Check calculation
		REQUIRE(vDataProxy.getType() == ValueType::Integer);
		REQUIRE(getRawType(vDataProxy) == RawValueType::Integer);
		REQUIRE(vDataProxy == 1);
		REQUIRE(nCalculationCount == 1);

		// Check calculation of duplicate
		REQUIRE(getRawType(vDataProxy2) == RawValueType::DataProxy);
		REQUIRE(vDataProxy2.getType() == ValueType::Integer);
		REQUIRE(getRawType(vDataProxy2) == RawValueType::Integer);
		REQUIRE(vDataProxy2 == 1);
		REQUIRE(nCalculationCount == 2);
	}
}


//
//
//
TEST_CASE("Variant_DataProxy.clone")
{
	SECTION("cached")
	{
		CommandProcessorExecuteCounter::g_nActionCount = 0;
		Variant vOrig = CommandProcessorExecuteCounter::createDataProxy(true, "passParams", Dictionary({ {"a", 1} }));
		Variant vClone1_Copy1 = vOrig.clone();
		Variant vClone1_Copy2 = vClone1_Copy1;
		Variant vClone2_Copy1 = vClone1_Copy1.clone();
		Variant vClone2_Copy2 = vClone2_Copy1;

		REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 0);
		REQUIRE(vClone1_Copy2.getType() == ValueType::Dictionary);
		REQUIRE(vClone2_Copy2.getType() == ValueType::Dictionary);
		REQUIRE(vClone1_Copy1.getType() == ValueType::Dictionary);
		REQUIRE(vClone2_Copy1.getType() == ValueType::Dictionary);
		REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 1);

		REQUIRE(getRawType(vOrig) == RawValueType::DataProxy);
		vClone1_Copy1.put("b", 2);

		REQUIRE(vClone1_Copy1 == Dictionary({ {"a", 1}, {"b", 2} }));
		REQUIRE(vClone1_Copy2 == Dictionary({ {"a", 1}, {"b", 2} }));
		REQUIRE(vClone2_Copy1 == Dictionary({ {"a", 1} }));
		REQUIRE(vClone2_Copy2 == Dictionary({ {"a", 1} }));
	}

	SECTION("noncached")
	{
		CommandProcessorExecuteCounter::g_nActionCount = 0;
		Variant vOrig = CommandProcessorExecuteCounter::createDataProxy(false, "passParams", Dictionary({ {"a", 1} }));
		Variant vClone1_Copy1 = vOrig.clone();
		Variant vClone1_Copy2 = vClone1_Copy1;
		Variant vClone2_Copy1 = vClone1_Copy1.clone();
		Variant vClone2_Copy2 = vClone2_Copy1;

		REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 0);
		REQUIRE(vClone1_Copy2.getType() == ValueType::Dictionary);
		REQUIRE(vClone2_Copy2.getType() == ValueType::Dictionary);
		REQUIRE(vClone1_Copy1.getType() == ValueType::Dictionary);
		REQUIRE(vClone2_Copy1.getType() == ValueType::Dictionary);
		REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 4);

		REQUIRE(getRawType(vOrig) == RawValueType::DataProxy);
		vClone1_Copy1.put("b", 2);

		REQUIRE(vClone1_Copy1 == Dictionary({ {"a", 1}, {"b", 2} }));
		REQUIRE(vClone1_Copy2 == Dictionary({ {"a", 1} }));
		REQUIRE(vClone2_Copy1 == Dictionary({ {"a", 1} }));
		REQUIRE(vClone2_Copy2 == Dictionary({ {"a", 1} }));
	}
}

//
//
//
TEST_CASE("Variant_DataProxy.returns_different_type")
{
	Variant vValues[] = { Variant(nullptr), Variant(true), Variant(10), Variant("str"), 
		Variant(createObject<SimpleTestObject>()), Dictionary({{"a", 1}}), Sequence({"xxx"}) };

	for (auto vValue : vValues)
	{
		Variant vDataProxy = CommandProcessorExecuteCounter::createDataProxy(false, "passParams", vValue);
		REQUIRE(getRawType(vDataProxy) == RawValueType::DataProxy);
		REQUIRE(vDataProxy.getType() == vValue.getType());
		REQUIRE(vDataProxy == vValue);
	}

}

//
//
//
TEST_CASE("Variant_DataProxy.double_DataProxy")
{
	Variant vValue = Variant(10);
	Variant vDataProxy1 = CommandProcessorExecuteCounter::createDataProxy(true, "passParams", vValue);
	Variant vDataProxy2 = CommandProcessorExecuteCounter::createDataProxy(true, "passParams", vDataProxy1);
	REQUIRE(getRawType(vDataProxy2) == RawValueType::DataProxy);
	REQUIRE(vDataProxy2.getType() == ValueType::Integer);
	REQUIRE(vDataProxy2 == 10);
}

//
//
//
TEST_CASE("Variant_DataProxy.not_evaluating_methods")
{
	FinalConstructCounter::g_nActionCount = 0;
	Variant vDataProxy = FinalConstructCounter::createDataProxy(false);
	SECTION("coping")
	{
		Variant vCopy = vDataProxy;
		Variant vCopy2 (vDataProxy);
		Variant vCopy3(std::move(vDataProxy));
	}

	SECTION("put_into_Dictionary")
	{
		Variant vCont = Dictionary({ {"a", vDataProxy} });
		vCont.put("b", vDataProxy);
	}

	SECTION("put_into_Sequence")
	{
		Variant vCont = Sequence({ vDataProxy });
		vCont.push_back(vDataProxy);
	}

	SECTION("get_from_Dictionary")
	{
		Variant vCont = Dictionary({ {"a", vDataProxy} });
		Variant vDataProxy2 = vCont.get("a");
		REQUIRE(getRawType(vDataProxy2) == RawValueType::DataProxy);
	}

	SECTION("get_from_Sequence")
	{
		Variant vCont = Sequence({ vDataProxy });
		Variant vDataProxy2 = vCont.get(0);
		REQUIRE(getRawType(vDataProxy2) == RawValueType::DataProxy);
	}

	SECTION("end")
	{
		(void)vDataProxy.end();
	}

	SECTION("print")
	{
		(void)vDataProxy.print();
	}

	SECTION("clone")
	{
		Variant vDataProxyClone = vDataProxy.clone();
	}

	REQUIRE(FinalConstructCounter::g_nActionCount == 0);
}

//
//
//
TEST_CASE("Variant_DataProxy.evaluating_methods")
{
	CommandProcessorExecuteCounter::g_nActionCount = 0;
	Variant vDataProxy = CommandProcessorExecuteCounter::createDataProxy(false, "passParams", Dictionary({ {"a", 1} }));
	REQUIRE(getRawType(vDataProxy) == RawValueType::DataProxy);
	size_t nExpectedActionCount = 1;

	SECTION("getType")
	{
		vDataProxy.getType();
	}

	SECTION("convertion")
	{
		REQUIRE_THROWS([&vDataProxy]() { int nData = vDataProxy; }());
	}

	SECTION("compare")
	{
		CHECK_FALSE(vDataProxy == 10);
	}

	SECTION("begin")
	{
		(void)vDataProxy.begin();
	}

	SECTION("clear")
	{
		(void)vDataProxy.clear();
	}

	SECTION("createIterator")
	{
		(void)vDataProxy.createIterator(false);
	}

	SECTION("Sequence_erase")
	{
		REQUIRE_THROWS([&vDataProxy]() { vDataProxy.erase(0); }());
	}

	SECTION("Dictionary_erase")
	{
		vDataProxy.erase("a");
	}

	SECTION("getSize")
	{
		vDataProxy.getSize();
	}

	SECTION("has")
	{
		vDataProxy.has("xxx");
	}

	SECTION("Sequence_get")
	{
		REQUIRE_THROWS([&vDataProxy]() { (void)vDataProxy.get(0); }());
	}

	SECTION("Sequence_get_with_default")
	{
		(void)vDataProxy.get(0, "xxx");
	}

	SECTION("Dictionary_get")
	{
		(void)vDataProxy.get("a");
	}

	SECTION("Dictionary_get_with_default")
	{
		(void)vDataProxy.get("a", "xxx");
	}

	SECTION("insert")
	{
		REQUIRE_THROWS([&vDataProxy]() { vDataProxy.insert(0, "aaa"); }());
	}

	SECTION("isDictionaryLike")
	{
		vDataProxy.isDictionaryLike();
	}

	SECTION("isDictionaryLike")
	{
		vDataProxy.isSequenceLike();
	}

	SECTION("isEmpty")
	{
		vDataProxy.isEmpty();
	}

	SECTION("isNull")
	{
		vDataProxy.isNull();
	}

	SECTION("isObject")
	{
		vDataProxy.isObject();
	}

	SECTION("isString")
	{
		vDataProxy.isString();
	}

	REQUIRE(getRawType(vDataProxy) == RawValueType::Dictionary);
	REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == nExpectedActionCount);
}

//
//
//
TEST_CASE("Variant_DataProxy.serialization")
{
	CommandProcessorExecuteCounter::g_nActionCount = 0;
	Variant vDataProxy = CommandProcessorExecuteCounter::createDataProxy(false, "passParams", Dictionary({ {"a", 1} }));
	auto sSerializedData =  variant::serializeToJson(vDataProxy);
	REQUIRE(getRawType(vDataProxy) == RawValueType::Dictionary);
	REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 1);

	Variant vDataProxy2 = variant::deserializeFromJson(sSerializedData);
	REQUIRE(getRawType(vDataProxy2) == RawValueType::Dictionary);
	REQUIRE(vDataProxy2 == Dictionary({ {"a", 1} }));
	REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 1);
}

//
//
//
TEST_CASE("Variant_DataProxy.deserialization")
{
	SECTION("obj")
	{
		FinalConstructCounter::g_nActionCount = 0;
		const std::string_view sJson = R"json({
			"$$proxy": "obj",
			"clsid": "0xE52D3F50"
		})json";
		Variant vDataProxy = variant::deserializeFromJson(sJson);
		REQUIRE(getRawType(vDataProxy) == RawValueType::DataProxy);
		Variant vDataProxy2 = vDataProxy;

		REQUIRE(queryInterfaceSafe<FinalConstructCounter>(vDataProxy) != nullptr);
		REQUIRE(FinalConstructCounter::g_nActionCount == 1);
		REQUIRE(queryInterfaceSafe<FinalConstructCounter>(vDataProxy2) != nullptr);
		REQUIRE(FinalConstructCounter::g_nActionCount == 2);
	}

	SECTION("cachedObj")
	{
		FinalConstructCounter::g_nActionCount = 0;
		const std::string_view sJson = R"json({
			"$$proxy": "cachedObj",
			"clsid": "0xE52D3F50"
		})json";
		Variant vDataProxy = variant::deserializeFromJson(sJson);
		REQUIRE(getRawType(vDataProxy) == RawValueType::DataProxy);
		Variant vDataProxy2 = vDataProxy;

		REQUIRE(queryInterfaceSafe<FinalConstructCounter>(vDataProxy) != nullptr);
		REQUIRE(FinalConstructCounter::g_nActionCount == 1);
		REQUIRE(queryInterfaceSafe<FinalConstructCounter>(vDataProxy2) != nullptr);
		REQUIRE(FinalConstructCounter::g_nActionCount == 1);
	}

	SECTION("cmd")
	{
		CommandProcessorExecuteCounter::g_nActionCount = 0;
		const std::string_view sJson = R"json({
			"$$proxy": "cmd",
			"processor": { "clsid" : "0xE52D3F51" },
			"command": "passParams",
			"params": { "a" : 1 }
		})json";
		Variant vDataProxy = variant::deserializeFromJson(sJson);
		REQUIRE(getRawType(vDataProxy) == RawValueType::DataProxy);
		Variant vDataProxy2 = vDataProxy;

		REQUIRE(vDataProxy == Dictionary({ {"a", 1} }));
		REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 1);
		REQUIRE(vDataProxy2 == Dictionary({ {"a", 1} }));
		REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 2);
	}

	SECTION("cachedCmd")
	{
		CommandProcessorExecuteCounter::g_nActionCount = 0;
		const std::string_view sJson = R"json({
			"$$proxy": "cachedCmd",
			"processor": { "clsid" : "0xE52D3F51" },
			"command": "passParams",
			"params": { "a" : 1 }
		})json";
		Variant vDataProxy = variant::deserializeFromJson(sJson);
		REQUIRE(getRawType(vDataProxy) == RawValueType::DataProxy);
		Variant vDataProxy2 = vDataProxy;

		REQUIRE(vDataProxy == Dictionary({ {"a", 1} }));
		REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 1);
		REQUIRE(vDataProxy2 == Dictionary({ {"a", 1} }));
		REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 1);
	}
}

//
//
//
TEST_CASE("Variant_DataProxy.merge")
{
	CommandProcessorExecuteCounter::g_nActionCount = 0;
	Variant vDst = CommandProcessorExecuteCounter::createDataProxy(false, "passParams", Dictionary({ {"a", 1}, {"b", 2} }));
	Variant vSrc = CommandProcessorExecuteCounter::createDataProxy(false, "passParams", Dictionary({ {"a", 1}, {"c", 3} }));

	vDst.merge(vSrc, Variant::MergeMode::All | Variant::MergeMode::MergeNestedDict);
	REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 2);

	REQUIRE(getRawType(vDst) == RawValueType::Dictionary);
	REQUIRE(getRawType(vSrc) == RawValueType::DataProxy);
	REQUIRE(vDst == Dictionary({ {"a", 1}, {"b", 2}, {"c", 3} }));
}

