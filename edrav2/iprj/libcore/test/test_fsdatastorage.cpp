//
// edrav2.libcore.unittest
//
// Directory data provider objects test 
//
// Autor: Denis Bogdanov (08.03.2019)
// Reviewer: ???
//
#include "pch.h"
#include "common.h"

using namespace openEdr;

//
//
//
TEST_CASE("FsDataStorage.create")
{
	auto pathToData = initTest();
	ObjPtr<IInformationProvider> pInfo;

	const Size c_nInitSize = 5;
	auto sStgPath = getTestTempPath() / L"default";
	std::filesystem::copy(pathToData / "default", sStgPath);

	SECTION("default")
	{
		REQUIRE_NOTHROW(pInfo = createTestObject<IInformationProvider>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()} })));
		REQUIRE(pInfo->getInfo().get("size", 0) == c_nInitSize);
	}

	SECTION("truncate")
	{
		REQUIRE_NOTHROW(pInfo = createTestObject<IInformationProvider>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()} , {"mode", "truncate"} })));
		REQUIRE(pInfo->getInfo().get("size", 0) == 0);
	}

	SECTION("empty")
	{
		auto sStgPath = pathToData / L"empty";
		REQUIRE_NOTHROW(pInfo = createTestObject<IInformationProvider>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()} })));
		REQUIRE(pInfo->getInfo().get("size", 0) == 0);
	}

	SECTION("dataPath_not_existed")
	{
		REQUIRE_THROWS_AS(pInfo = createTestObject<IInformationProvider>(CLSID_FsDataStorage,
			Dictionary({ {"path", "invalid_data_path"} })), error::NotFound);
	}

	SECTION("path_bad_type")
	{
		REQUIRE_THROWS_AS(pInfo = createTestObject<IInformationProvider>(CLSID_FsDataStorage,
			Dictionary({ {"bad", 1} })), error::InvalidArgument);
	}

	SECTION("bad_path")
	{
		REQUIRE_THROWS_AS(pInfo = createTestObject<IInformationProvider>(CLSID_FsDataStorage,
			Dictionary({ {"path", 1} })), error::InvalidArgument);
	}

	SECTION("invalid_config_format")
	{
		REQUIRE_THROWS_AS(pInfo = createTestObject<IInformationProvider>(CLSID_FsDataStorage,
			Variant(42)), error::InvalidArgument);
	}
}

//
//
//
TEST_CASE("FsDataStorage.get")
{
	auto pathToData = initTest();
	auto sTempPath = getTestTempPath();
	ObjPtr<IDataProvider> pProvider;

	SECTION("default")
	{
		const Size c_nInitSize = 5;
		auto sStgPath = sTempPath / L"default";
		std::filesystem::copy(pathToData / "default", sStgPath);

		REQUIRE_NOTHROW(pProvider = createTestObject<IDataProvider>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()} })));
		auto pInfo = queryInterface<IInformationProvider>(pProvider);

		for (Size n = 0; n < c_nInitSize; n++)
		{
			auto v = pProvider->get();
			REQUIRE(v.has_value());
			REQUIRE(v.value().get("f_int", 100) == n);
			REQUIRE(v.value().get("f_seq", {}).getSize() == n);
		}
		auto v = pProvider->get();
		REQUIRE(!v.has_value());		
		REQUIRE(pInfo->getInfo().get("size", 0) == 0);

		// Recreate on existing data
		REQUIRE_NOTHROW(pProvider = createTestObject<IDataProvider>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str() }, {"deleteOnGet", true} })));
		pInfo = queryInterface<IInformationProvider>(pProvider);
		REQUIRE(pInfo->getInfo().get("size", 0) == c_nInitSize);
		for (Size n = 0; n < c_nInitSize; n++)
		{
			auto v = pProvider->get();
			REQUIRE(v.has_value());
			REQUIRE(v.value().get("f_int", 100) == n);
			REQUIRE(v.value().get("f_seq", {}).getSize() == n);
		}
		v = pProvider->get();
		REQUIRE(!v.has_value());
		REQUIRE(pInfo->getInfo().get("size", 0) == 0);

		REQUIRE_NOTHROW(pProvider = createTestObject<IDataProvider>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str() } })));
		pInfo = queryInterface<IInformationProvider>(pProvider);
		REQUIRE(pInfo->getInfo().get("size", 0) == 0);
	}

	SECTION("empty")
	{
		auto sStgPath = sTempPath / L"empty";
		std::filesystem::copy(pathToData / "empty", sStgPath);
		REQUIRE_NOTHROW(pProvider = createTestObject<IDataProvider>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str() } })));
		auto v = pProvider->get();
		REQUIRE(!v.has_value());
	}
}

//
//
//
TEST_CASE("FsDataStorage.put")
{
	auto pathToData = initTest();
	ObjPtr<IDataReceiver> pReceiver;
	auto sTempPath = getTestTempPath();

	SECTION("empty")
	{
		auto sStgPath = sTempPath / "empty";
		std::filesystem::copy(pathToData / "empty", sStgPath);

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()} })));
		auto pInfo = queryInterface<IInformationProvider>(pReceiver);

		for (Size n = 0; n < 3; n++)
		{
			REQUIRE_NOTHROW(pReceiver->put(Dictionary({
				{"f_int", n},
				{"f_str", "item"}
			})));
			REQUIRE(pInfo->getInfo().get("size", 0) == n + 1);
		}

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()} })));
		pInfo = queryInterface<IInformationProvider>(pReceiver);
		REQUIRE(pInfo->getInfo().get("size", 0) == 3);
	}

	SECTION("existing")
	{
		const Size c_nInitSize = 5;
		auto sStgPath = sTempPath / "default";
		std::filesystem::copy(pathToData / "default", sStgPath);

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()} })));
		auto pInfo = queryInterface<IInformationProvider>(pReceiver);

		for (Size n = 0; n < 3; n++)
		{
			REQUIRE_NOTHROW(pReceiver->put(Dictionary({
				{"f_int", c_nInitSize + n},
				{"f_str", "item"}
			})));
			REQUIRE(pInfo->getInfo().get("size", 0) == c_nInitSize + n + 1);
		}

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()} })));
		pInfo = queryInterface<IInformationProvider>(pReceiver);
		REQUIRE(pInfo->getInfo().get("size", 0) == c_nInitSize + 3);
	}

	SECTION("empty_with_size")
	{
		auto sStgPath = sTempPath / "empty";
		std::filesystem::copy(pathToData / "empty", sStgPath);

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()}, {"maxSize", 5} })));
		auto pInfo = queryInterface<IInformationProvider>(pReceiver);

		for (Size n = 0; n < 5; n++)
		{
			REQUIRE_NOTHROW(pReceiver->put(Dictionary({
				{"f_int", n},
				{"f_str", "item"}
				})));
			REQUIRE(pInfo->getInfo().get("size", 0) == n + 1);
		}

		REQUIRE_THROWS_AS(pReceiver->put(Dictionary({
			{"f_int", 6},
			{"f_str", "item"}
			})), error::LimitExceeded);
		REQUIRE(pInfo->getInfo().get("size", 0) == 5);

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()} })));
		pInfo = queryInterface<IInformationProvider>(pReceiver);
		REQUIRE(pInfo->getInfo().get("size", 0) == 5);
	}

	SECTION("existing_with_size")
	{
		const Size c_nInitSize = 5;
		auto sStgPath = sTempPath / "default";
		std::filesystem::copy(pathToData / "default", sStgPath);

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()}, {"maxSize", c_nInitSize + 1} })));
		auto pInfo = queryInterface<IInformationProvider>(pReceiver);

		REQUIRE_NOTHROW(pReceiver->put(Dictionary({
			{"f_int", c_nInitSize},
			{"f_str", "item"}
		})));
		REQUIRE(pInfo->getInfo().get("size", 0) == c_nInitSize + 1);
	
		REQUIRE_THROWS_AS(pReceiver->put(Dictionary({
			{"f_int", c_nInitSize + 1},
			{"f_str", "item"}
			})), error::LimitExceeded);
		REQUIRE(pInfo->getInfo().get("size", 0) == c_nInitSize + 1);

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()} })));
		pInfo = queryInterface<IInformationProvider>(pReceiver);
		REQUIRE(pInfo->getInfo().get("size", 0) == c_nInitSize + 1);
	}

	SECTION("existing_with_overflow")
	{
		const Size c_nInitSize = 5;
		auto sStgPath = sTempPath / "default";
		std::filesystem::copy(pathToData / "default", sStgPath);

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()}, {"maxSize", c_nInitSize - 2} })));
		auto pInfo = queryInterface<IInformationProvider>(pReceiver);

		REQUIRE_THROWS_AS(pReceiver->put(Dictionary({
			{"f_int", c_nInitSize + 1},
			{"f_str", "item"}
			})), error::LimitExceeded);
		REQUIRE(pInfo->getInfo().get("size", 0) == c_nInitSize);

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()} })));
		pInfo = queryInterface<IInformationProvider>(pReceiver);
		REQUIRE(pInfo->getInfo().get("size", 0) == c_nInitSize);
	}

	std::filesystem::remove_all(sTempPath);
}

//
//
//
TEST_CASE("FsDataStorage.put_get")
{
	auto pathToData = initTest();
	ObjPtr<IDataReceiver> pReceiver;
	auto sTempPath = getTestTempPath();

	SECTION("empty")
	{
		auto sStgPath = sTempPath / "empty";
		std::filesystem::copy(pathToData / "empty", sStgPath);

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()}, {"deleteOnGet", true} })));
		auto pInfo = queryInterface<IInformationProvider>(pReceiver);

		for (Size n = 0; n < 3; n++)
		{
			REQUIRE_NOTHROW(pReceiver->put(Dictionary({
				{"f_int", n},
				{"f_str", "item"}
			})));
			REQUIRE(pInfo->getInfo().get("size", 0) == n + 1);
		}

		auto pProvider = queryInterface<IDataProvider>(pReceiver);
		for (Size n = 0; n < 3; n++)
		{
			auto v = pProvider->get();
			REQUIRE(v.has_value());
			REQUIRE(v.value().get("f_int", 100) == n);
		}

		auto v = pProvider->get();
		REQUIRE(!v.has_value());
		REQUIRE(pInfo->getInfo().get("size", 0) == 0);
	}

	SECTION("existing")
	{
		const Size c_nInitSize = 5;
		auto sStgPath = sTempPath / "default";
		std::filesystem::copy(pathToData / "default", sStgPath);

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()}, {"deleteOnGet", true} })));
		auto pInfo = queryInterface<IInformationProvider>(pReceiver);

		for (Size n = 0; n < 3; n++)
		{
			REQUIRE_NOTHROW(pReceiver->put(Dictionary({
				{"f_int", c_nInitSize + n},
				{"f_str", "item"}
				})));
			REQUIRE(pInfo->getInfo().get("size", 0) == c_nInitSize + n + 1);
		}

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()}, {"deleteOnGet", true} })));
		pInfo = queryInterface<IInformationProvider>(pReceiver);
		REQUIRE(pInfo->getInfo().get("size", 0) == c_nInitSize + 3);

		auto pProvider = queryInterface<IDataProvider>(pReceiver);
		for (Size n = 0; n < 3; n++)
		{
			auto v = pProvider->get();
			REQUIRE(v.has_value());
			REQUIRE(v.value().get("f_int", 100) == n);
		}
		REQUIRE(pInfo->getInfo().get("size", 0) == c_nInitSize);

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()}, {"deleteOnGet", true} })));
		pInfo = queryInterface<IInformationProvider>(pReceiver);
		REQUIRE(pInfo->getInfo().get("size", 0) == c_nInitSize);
		
		pProvider = queryInterface<IDataProvider>(pReceiver);
		for (Size n = 0; n < c_nInitSize; n++)
		{
			auto v = pProvider->get();
			REQUIRE(v.has_value());
			REQUIRE(v.value().get("f_int", 100) == 3 + n);
		}
		REQUIRE(pInfo->getInfo().get("size", 0) == 0);

		REQUIRE_NOTHROW(pReceiver = createTestObject<IDataReceiver>(CLSID_FsDataStorage,
			Dictionary({ {"path", sStgPath.c_str()}, {"deleteOnGet", true} })));
		pInfo = queryInterface<IInformationProvider>(pReceiver);
		auto v = pProvider->get();
		REQUIRE(!v.has_value());
		REQUIRE(pInfo->getInfo().get("size", 0) == 0);		
	}

	std::filesystem::remove_all(sTempPath);
}
