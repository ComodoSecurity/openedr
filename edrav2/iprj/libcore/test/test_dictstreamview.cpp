//
// edrav2.ut_libcore project
//
// Author: Yury Ermakov (29.12.2018)
// Reviewer:
//
// Unit tests for DictionaryStreamView object.
///

#include "pch.h"

using namespace cmd;
using namespace cmd::io;

//////////////////////////////////////////////////////////////////////////////
//
// Support facilities
//
//////////////////////////////////////////////////////////////////////////////

//
// Returns name of new temporary file.
//
std::wstring getTempFileName()
{
	wchar_t sFileName[L_tmpnam_s];
#ifdef _MSC_VER
	if (_wtmpnam_s(sFileName, L_tmpnam_s))
#else
	if (tmpnam_s(sFileName, L_tmpnam_s))
#endif
		error::CrtError(SL).throwException();
	return sFileName;
}

//////////////////////////////////////////////////////////////////////////////
//
// Test cases
//
//////////////////////////////////////////////////////////////////////////////

///
// Checks the case of invalid configuration of DictionaryStreamView.
///
// In case of invalid configuration supplied into createObject function
// the error::InvalidArgument exception is thrown.
//
TEST_CASE("DictionaryStreamView.createWithInvalidConfig")
{
	auto fn = [&]()
	{
		auto pObj = createObject(CLSID_DictionaryStreamView, Dictionary());
	};
	REQUIRE_THROWS_AS(fn(), error::InvalidArgument);
}

//
// Checks modify operations in DictionaryStreamView working in read-only mode.
//
// During read-only mode any modify operation throws the error::InvalidUsage exception.
//
TEST_CASE("DictionaryStreamView.readOnly")
{
	auto pMemStream = createObject(CLSID_MemoryStream);
	auto pObj = createObject(CLSID_DictionaryStreamView,
		Dictionary({ {"stream", pMemStream}, {"readOnly", true} }));
	auto pDictionaryProxy = queryInterface<variant::IDictionaryProxy>(pObj);

	SECTION("put")
	{
		auto fn = [&]()
		{
			pDictionaryProxy->put("a", 1);
		};
		REQUIRE_THROWS_AS(fn(), error::InvalidUsage);
	}

	SECTION("erase")
	{
		auto fn = [&]()
		{
			pDictionaryProxy->erase("a");
		};
		REQUIRE_THROWS_AS(fn(), error::InvalidUsage);
	}

	SECTION("clear")
	{
		auto fn = [&]()
		{
			pDictionaryProxy->clear();
		};
		REQUIRE_THROWS_AS(fn(), error::InvalidUsage);
	}
}

//
// Checks all basic methods of IDictionaryProxy.
//
TEST_CASE("DictionaryStreamView.IDictionaryProxy")
{
	auto fnScenario = [&](Variant vValue,
		variant::DictionaryPreModifyCallback fnPreModify, variant::DictionaryPostModifyCallback fnPostModify,
		variant::DictionaryPreClearCallback fnPreClear, variant::DictionaryPostClearCallback fnPostClear,
		variant::CreateProxyCallback fnCreateDictionaryProxy, variant::CreateProxyCallback fnCreateSequenceProxy)
	{
		auto pMemStream = createObject(CLSID_MemoryStream);
		io::write(queryInterface<io::IWritableStream>(pMemStream), variant::serializeToJson(vValue));

		auto pObj = createObject(CLSID_DictionaryStreamView,
			Dictionary({ {"stream", pMemStream}, {"readOnly", false} }));
		auto pDictionaryProxy = queryInterface<variant::IDictionaryProxy>(pObj);
		Dictionary dict(pObj);

		REQUIRE((vValue.getSize() == dict.getSize()));
		REQUIRE((vValue.isEmpty() == dict.isEmpty()));
		REQUIRE(dict.has("d"));
		REQUIRE(dict.get("i") == 999);

		std::set<std::string> s;
		for (auto v : dict)
			s.insert(static_cast<std::string>(v.first));
		REQUIRE(s.size() == 5);

		dict.put("n", 123);
		REQUIRE(dict["n"] == 123);
		dict.erase("n");
		REQUIRE_FALSE(dict.has("n"));

		REQUIRE(pDictionaryProxy->getSafe("i") == 999);

		dict.clear();
		REQUIRE(dict.isEmpty());
		REQUIRE(dict.getSize() == 0);
		REQUIRE_FALSE(dict.has("d"));
	};

	SECTION("noCallbacks")
	{
		Dictionary dict({
			{"b", true},
			{"i", 999},
			{"s", "string"},
			{"d", Dictionary({ {"a", 1} })},
			{"q", Sequence({ 1, 2, 3})}
			});
		REQUIRE_NOTHROW(fnScenario(dict, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));
	}
}

//
// Checks modify operations in DictionaryStreamView working in read-only mode.
//
// During read-only mode any modify operation throws the error::InvalidUsage exception.
//
TEST_CASE("DictionaryStreamView.reload")
{
	Dictionary dict({
		{"b", true},
		{"i", 999},
		{"s", "string"},
		{"d", Dictionary({ {"a", 1} })},
		{"q", Sequence({ 1, 2, 3})}
		});

	auto fnScenario = [&](Variant vValue)
	{
		auto pMemStream = createObject(CLSID_MemoryStream);
		io::write(queryInterface<io::IWritableStream>(pMemStream), variant::serializeToJson(vValue));

		auto pObj = createObject(CLSID_DictionaryStreamView,
			Dictionary({ {"stream", pMemStream}, {"readOnly", false} }));
		auto pDictionaryProxy = queryInterface<variant::IDictionaryProxy>(pObj);
		Dictionary dict(pObj);

		dict.put("i", 666);

		auto pView = queryInterface<IDictionaryStreamView>(pObj);
		pView->reload();

		Dictionary newDict(pObj);
		REQUIRE(newDict.get("i") == 666);
	};

	REQUIRE_NOTHROW(fnScenario(dict));
}

//
// TODO: test thread-safety
//
