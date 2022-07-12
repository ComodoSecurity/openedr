//
// edrav2.ut_libcore project
// 
// Author: Yury Ermakov (10.01.2019)
// Reviewer:
//
// Unit tests for VariantProxy objects.
//
#include "pch.h"

using namespace cmd;

//
// TestDictionaryProxy is plain wrapper of Dictionary
//
class TestDictionaryProxy2 : public ObjectBase <>, public variant::IDictionaryProxy
{
private:
	Dictionary m_dict;

	class Iterator : public variant::IIterator
	{
	private:
		Dictionary::const_iterator m_iter;
		Dictionary::const_iterator m_end;
	public:
		Iterator(Dictionary dict, bool fUnsafe)
		{
			m_iter = fUnsafe ? dict.unsafeBegin() : dict.begin();
			m_end = fUnsafe ? dict.unsafeEnd() : dict.end();
		}

		virtual void goNext() override { ++m_iter; }
		virtual bool isEnd() const override { return !(m_iter != m_end); }
		virtual variant::DictionaryKeyRefType getKey() const override { return m_iter->first; }
		virtual const Variant getValue() const override { return m_iter->second; }
	};

public:
	void finalConstruct(Variant vCfg)
	{
		if (!vCfg.isNull())
			m_dict = Dictionary(vCfg);
	}

	Size getSize() const override { return m_dict.getSize(); }
	bool isEmpty() const override { return m_dict.isEmpty(); }
	bool has(variant::DictionaryKeyRefType sName) const override { return m_dict.has(sName); }
	Size erase(variant::DictionaryKeyRefType sName) override { return m_dict.erase(sName); }
	void clear() override { return m_dict.clear(); }
	Variant get(variant::DictionaryKeyRefType sName) const override { return m_dict.get(sName); }
	void put(variant::DictionaryKeyRefType sName, const Variant& Value) override { return m_dict.put(sName, Value); }
	std::unique_ptr<variant::IIterator> createIterator(bool fUnsafe) const override
	{
		return std::make_unique<Iterator>(m_dict, fUnsafe);
	}
	std::optional<Variant> getSafe(variant::DictionaryKeyRefType sName) const override
	{
		if (!m_dict.has(sName))
			return {};
		return m_dict.get(sName);
	}
};

//
// TestSequenceProxy is plain wrapper of Sequence
//
class TestSequenceProxy : public ObjectBase <>, public variant::ISequenceProxy
{
private:
	Sequence m_seq;

	class Iterator : public variant::IIterator
	{
	private:
		Sequence::const_iterator m_iter;
		Sequence::const_iterator m_end;
	public:
		Iterator(Sequence seq, bool fUnsafe)
		{
			m_iter = fUnsafe ? seq.unsafeBegin() : seq.begin();
			m_end = fUnsafe ? seq.unsafeEnd() : seq.end();
		}

		void goNext() override { ++m_iter; }
		bool isEnd() const override { return !(m_iter != m_end); }
		variant::DictionaryKeyRefType getKey() const override
		{
			error::InvalidUsage(SL).throwException();
			// Here is an unreachable code to suppress "error C4716:: must return a value"
			return nullptr;
		}
		const Variant getValue() const override { return *m_iter; }
	};

public:
	void finalConstruct(Variant vCfg)
	{
		if (!vCfg.isNull())
			m_seq = Sequence(vCfg);
	}

	Size getSize() const override { return m_seq.getSize(); }
	bool isEmpty() const override { return m_seq.isEmpty(); }
	void clear() override { return m_seq.clear(); }
	Size erase(Size nIndex) override { return m_seq.erase(nIndex); }
	Variant get(Size nIndex) const override { return m_seq.get(nIndex); }
	void put(Size nIndex, const Variant& Value) override { return m_seq.put(nIndex, Value); }
	void push_back(const Variant& vValue) override { return m_seq.push_back(vValue); }
	void setSize(Size nSize) override { return m_seq.setSize(nSize); }
	void insert(Size nIndex, const Variant& vValue) override { return m_seq.insert(nIndex, vValue); }
	std::unique_ptr<variant::IIterator> createIterator(bool fUnsafe) const override
	{
		return std::make_unique<Iterator>(m_seq, fUnsafe);
	}
	std::optional<Variant> getSafe(Size nIndex) const override
	{
		if (nIndex >= m_seq.getSize())
			return {};
		return m_seq.get(nIndex);
	}
};

//////////////////////////////////////////////////////////////////////////////
//
// Test cases
//
//////////////////////////////////////////////////////////////////////////////

//
// Checks variant::createDictionaryProxy() function.
//
// Testcases:
//   - success creation of DictionaryProxy
//   - throwing error::TypeError for other value types
//
TEST_CASE("DictionaryProxy.create")
{
	auto fnScenario = [&](Variant vValue)
	{
		auto vProxy = variant::createDictionaryProxy(vValue,
			nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
		REQUIRE((!vValue.isDictionaryLike() || (vValue.isDictionaryLike() && vProxy.isDictionaryLike())));
	};

	SECTION("Dictionary")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary()));
	}

	SECTION("IDictionaryProxy")
	{
		REQUIRE_NOTHROW(fnScenario(createObject<TestDictionaryProxy2>()));
	}

	SECTION("notDictionary")
	{
		REQUIRE_THROWS_AS(fnScenario(Variant(1)), error::TypeError);
	}
}

//
// Checks all basic methods of DictionaryProxy.
//
TEST_CASE("DictionaryProxy.basicMethods")
{
	auto fnScenario = [&](Variant vValue,
		variant::DictionaryPreModifyCallback fnPreModify, variant::DictionaryPostModifyCallback fnPostModify,
		variant::DictionaryPreClearCallback fnPreClear, variant::DictionaryPostClearCallback fnPostClear,
		variant::CreateProxyCallback fnCreateDictionaryProxy, variant::CreateProxyCallback fnCreateSequenceProxy)
	{
		Dictionary dict = variant::createDictionaryProxy(vValue,
			fnPreModify, fnPostModify, fnPreClear, fnPostClear, fnCreateDictionaryProxy, fnCreateSequenceProxy);
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

		auto pProxy = queryInterface<variant::IDictionaryProxy>(Variant(dict));
		REQUIRE(pProxy->getSafe("i") == 999);

		auto pClonable = queryInterface<variant::IClonable>(Variant(dict));
		Variant vClone = dict.clone();

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
// Checks variant::createSequenceProxy() function.
//
// Testcases:
//   - success creation of SequenceProxy
//   - throwing error::TypeError for other value types
//
TEST_CASE("SequenceProxy.create")
{
	auto fnScenario = [&](Variant vValue)
	{
		auto vProxy = variant::createSequenceProxy(vValue, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
		REQUIRE((!vValue.isSequenceLike() || (vValue.isSequenceLike() && vProxy.isSequenceLike())));
	};

	SECTION("Sequence")
	{
		REQUIRE_NOTHROW(fnScenario(Sequence()));
	}

	SECTION("ISequenceProxy")
	{
		REQUIRE_NOTHROW(fnScenario(createObject<TestSequenceProxy>()));
	}

	SECTION("notSequence")
	{
		REQUIRE_THROWS_AS(fnScenario(Variant(1)), error::TypeError);
	}
}

/// @fn SequenceProxy.basicMethods
/// @brief Checks all basic methods of SequenceProxy.
TEST_CASE("SequenceProxy.basicMethods")
{
	auto fnScenario = [&](Variant vValue,
		variant::SequencePreModifyCallback fnPreModify, variant::SequencePostModifyCallback fnPostModify,
		variant::SequencePreResizeCallback fnPreResize, variant::SequencePostResizeCallback fnPostResize,
		variant::CreateProxyCallback fnCreateDictionaryProxy, variant::CreateProxyCallback fnCreateSequenceProxy)
	{
		Sequence seq(variant::createSequenceProxy(vValue,
			fnPreModify, fnPostModify, fnPreResize, fnPostResize, fnCreateDictionaryProxy, fnCreateSequenceProxy));
		REQUIRE((vValue.getSize() == seq.getSize()));
		REQUIRE((vValue.isEmpty() == seq.isEmpty()));
		REQUIRE(seq[0] == 1);
		REQUIRE(seq.get(2) == 3);

		std::set<int> s;
		for (auto v : seq)
			s.insert(v);
		REQUIRE(s.size() == 3);

		seq.push_back(4);
		REQUIRE(seq[3] == 4);
		REQUIRE(seq.getSize() == 4);

		seq.insert(4, 5);
		REQUIRE(seq[4] == 5);
		REQUIRE(seq.getSize() == 5);

		seq.erase(4);
		REQUIRE(seq.getSize() == 4);

		seq.setSize(3);
		REQUIRE(seq.getSize() == 3);

		auto pProxy = queryInterface<variant::ISequenceProxy>(Variant(seq));
		REQUIRE(pProxy->getSafe(1) == 2);

		auto pClonable = queryInterface<variant::IClonable>(Variant(seq));
		Variant vClone = seq.clone();

		seq.clear();
		REQUIRE(seq.isEmpty());
		REQUIRE(seq.getSize() == 0);
		REQUIRE_THROWS_AS(seq.get(2), error::OutOfRange);
	};

	SECTION("noCallbacks")
	{
		Sequence seq({ 1, 2, 3 });
		REQUIRE_NOTHROW(fnScenario(seq, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));
	}
}
