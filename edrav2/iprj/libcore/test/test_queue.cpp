//
// edrav2.libcore.unittest
//
// Queue object test 
//
// Autor: Denis Bogdanov (18.01.2019)
// Reviewer: ???
//
#include "pch.h"
#include "common.h"

using namespace cmd;

namespace {

//
//
//
struct NotifyAcceptor : public ObjectBase<>,
	public IQueueNotificationAcceptor
{
	Variant m_vLastTag;
	Size m_nCount = 0;
	Size m_nWarnCount = 0;
	
	virtual void notifyAddQueueData(Variant vTag) override 
	{
		++m_nCount; 
		m_vLastTag = vTag;
	}

	virtual void notifyQueueOverflowWarning(Variant vTag) override
	{ 
		++m_nWarnCount; 
		m_vLastTag = vTag;
	}
};

struct TestQueueFilter : public ObjectBase<>, public IQueueFilter
{
	void reset() override {};

	bool filterElem(Variant vElem) override
	{
		return vElem.get("filterItem", false);
	}
};

} // namespace  (anonimous)

//
//
//
TEST_CASE("Queue.creation")
{
	ObjPtr<IQueue> pCurrQueue;

	auto fnScenario = [&](const Variant& vConfig)
	{
		auto pObj = createObject(CLSID_Queue, vConfig);
		pCurrQueue = queryInterface<IQueue>(pObj);
		auto pCommandProcessor = queryInterface<ICommandProcessor>(pObj);
		Variant vParams = Dictionary({ {"data", generateData(1)} });
		Variant vResult = pCommandProcessor->execute("put", vParams);
	};

	SECTION("simple_config")
	{
		auto pNotifyAcceptor = createObject<NotifyAcceptor>();

		REQUIRE_NOTHROW(fnScenario(Dictionary({
			{"maxSize", 100},
			{"warnSize", 50},
			{"warnMask", 0x0f},
			{"tag", "test_tag"},
			{"notifyAcceptor", pNotifyAcceptor},
		})));
		REQUIRE(pCurrQueue->getSize() == 1);
		auto vInfo = pCurrQueue->getInfo();
		REQUIRE(vInfo["maxSize"] == 100);
		REQUIRE(vInfo["warnSize"] == 50);
		REQUIRE(vInfo["warnMask"] == 0x0f);
		REQUIRE(vInfo["size"] == 1);
		REQUIRE(vInfo["putTryCount"] == 1);
		REQUIRE(vInfo["putCount"] == 1);
		REQUIRE(vInfo["getTryCount"] == 0);
		REQUIRE(vInfo["getCount"] == 0);
	}

	SECTION("invalid_config_param")
	{
		REQUIRE_THROWS_AS(fnScenario(Dictionary({
			{"maxSize", 100},
			{"warnSize", 50},
			{"tag", "test_tag"},
			{"notifyAcceptor", 43},
		})), error::TypeError);
	}

	SECTION("invalid_config_format")
	{
		REQUIRE_THROWS_AS(fnScenario(Variant(42)), error::InvalidArgument);
	}
}

//
//
//
TEST_CASE("Queue.loadData")
{
	ObjPtr<IQueue> pCurrQueue;

	auto fnScenario = [&](const Variant& vConfig)
	{
		auto pObj = createObject(CLSID_Queue, vConfig);
		pCurrQueue = queryInterface<IQueue>(pObj);
		auto pCommandProcessor = queryInterface<ICommandProcessor>(pObj);
		Variant vParams = Dictionary({ {"data", generateData(0)} });
		Variant vResult = pCommandProcessor->execute("put", vParams);
	};

	SECTION("persistent")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary({
			{"maxSize", 100},
			{"warnSize", 50},
			{"warnMask", 0x0f},
			{"persistent", true},
			{"tag", "test_tag"}
		})));
		REQUIRE_NOTHROW(pCurrQueue->loadData(Dictionary({
			{"items", Sequence({
				generateData(1),
				generateData(2),
				generateData(3)
			})}
		})));
		REQUIRE(pCurrQueue->getSize() == 4);
		auto vInfo = pCurrQueue->getInfo();
		REQUIRE(vInfo["maxSize"] == 100);
		REQUIRE(vInfo["warnSize"] == 50);
		REQUIRE(vInfo["warnMask"] == 0x0f);
		REQUIRE(vInfo["size"] == 4);
		REQUIRE(vInfo["putTryCount"] == 4);
		REQUIRE(vInfo["putCount"] == 4);
		REQUIRE(vInfo["getTryCount"] == 0);
		REQUIRE(vInfo["getCount"] == 0);
		auto pGetter = queryInterface<IDataProvider>(pCurrQueue);
		auto optItem = pGetter->get();
		REQUIRE(optItem.has_value());
		REQUIRE(optItem.value().get("id", -1) == 0);
		optItem = pGetter->get();
		REQUIRE(optItem.has_value());
		REQUIRE(optItem.value().get("id", -1) == 1);
	}

	SECTION("nonpersistent")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary({
			{"maxSize", 100},
			{"warnSize", 50},
			{"warnMask", 0x0f},
			{"persistent", false},
			{"tag", "test_tag"}
		})));
		REQUIRE_NOTHROW(pCurrQueue->loadData(Dictionary({
			{"items", Sequence({
				generateData(1),
				generateData(2),
				generateData(3)
			})}
		})));
		REQUIRE(pCurrQueue->getSize() == 1);
		auto vInfo = pCurrQueue->getInfo();
		REQUIRE(vInfo["maxSize"] == 100);
		REQUIRE(vInfo["warnSize"] == 50);
		REQUIRE(vInfo["warnMask"] == 0x0f);
		REQUIRE(vInfo["size"] == 1);
		REQUIRE(vInfo["putTryCount"] == 1);
		REQUIRE(vInfo["putCount"] == 1);
		REQUIRE(vInfo["getTryCount"] == 0);
		REQUIRE(vInfo["getCount"] == 0);
	}

}

//
//
//
TEST_CASE("Queue.saveData")
{
	ObjPtr<IQueue> pCurrQueue;

	SECTION("unlimited")
	{
		REQUIRE_NOTHROW(pCurrQueue = queryInterface<IQueue>(createObject(CLSID_Queue, Dictionary({
			{"maxSize", 0x100},
			{"warnSize", 0x40},
			{"warnMask", 0x0F},
			{"persistent", true},
			{"circular", true},
			{"tag", "test_tag"}
			}))));
		auto pPutter = queryInterface<IDataReceiver>(pCurrQueue);
		for (Size n = 0; n < 10; ++n)
			REQUIRE_NOTHROW(pPutter->put(generateData(n)));
		Variant vData;
		REQUIRE_NOTHROW(vData = pCurrQueue->saveData());
		REQUIRE(vData.get("items", Variant()).getSize() == 10);
	}

	SECTION("limited")
	{
		REQUIRE_NOTHROW(pCurrQueue = queryInterface<IQueue>(createObject(CLSID_Queue, Dictionary({
			{"maxSize", 0x100},
			{"saveSize", 50},
			{"warnSize", 0x80},
			{"warnMask", 0x0F},
			{"persistent", true},
			{"circular", true},
			{"tag", "test_tag"}
			}))));
		auto pPutter = queryInterface<IDataReceiver>(pCurrQueue);
		for (Size n = 0; n < 100; ++n)
			REQUIRE_NOTHROW(pPutter->put(generateData(n)));
		Variant vData;
		REQUIRE_NOTHROW(vData = pCurrQueue->saveData());
		REQUIRE(vData.get("items", Variant()).getSize() == 50);
	}

}

//
//
//
TEST_CASE("Queue.serialize_batch")
{
	ObjPtr<IQueue> pCurrQueue;

	REQUIRE_NOTHROW(pCurrQueue = queryInterface<IQueue>(createObject(CLSID_Queue, Dictionary({
		{"persistent", true},
		{"batchSize", 42},
		{"batchTimeout", 999},
		{"tag", "test_tag"}
	}))));
	auto pPutter = queryInterface<IDataReceiver>(pCurrQueue);
	for (Size n = 0; n < 100; ++n)
		REQUIRE_NOTHROW(pPutter->put(generateData(n)));
	Variant vData;
	REQUIRE_NOTHROW(vData = pCurrQueue->saveData());
	REQUIRE(vData.get("items", Variant()).getSize() == 3);
}

//
//
//
TEST_CASE("Queue.put")
{
	ObjPtr<IQueue> pCurrQueue;
	
	auto fnScenario = [&](const Variant& vConfig, Size nCount, Size nResultCount)
	{
		Size nMaxSize = vConfig.get("maxSize", 100000);
		auto pObj = queryInterface<IDataReceiver>(createObject(CLSID_Queue, vConfig));
		pCurrQueue = queryInterface<IQueue>(pObj);
		for (Size nCounter = 0; nCounter < nCount; ++nCounter)
		{
			REQUIRE(pCurrQueue->getSize() == std::min<Size>(nCounter, nMaxSize));
			pObj->put(generateData(nCounter));
		}
		REQUIRE(nResultCount == pCurrQueue->getSize());
	};

	SECTION("regular")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary(), 10, 10));
		REQUIRE(pCurrQueue->getSize() == 10);
		if (pCurrQueue)
		{
			auto vInfo = pCurrQueue->getInfo();
			REQUIRE(vInfo["size"] == 10);
			REQUIRE(vInfo["putTryCount"] == 10);
			REQUIRE(vInfo["putCount"] == 10);
			REQUIRE(vInfo["dropCount"] == 0);
			REQUIRE(vInfo["getTryCount"] == 0);
			REQUIRE(vInfo["getCount"] == 0);
		}
	}

	SECTION("disabled")
	{
		REQUIRE_THROWS_AS(fnScenario(Dictionary({
			{"maxSize", 100},
			{"mode", QueueMode::Off},
			{"warnSize", 70},
			{"tag", "test_tag"}
		}), 10, 10), error::OperationDeclined);
		REQUIRE(pCurrQueue->getSize() == 0);
		if (pCurrQueue)
		{
			auto vInfo = pCurrQueue->getInfo();
			REQUIRE(vInfo["size"] == 0);
			REQUIRE(vInfo["putTryCount"] == 0);
			REQUIRE(vInfo["putCount"] == 0);
			REQUIRE(vInfo["dropCount"] == 0);
			REQUIRE(vInfo["getTryCount"] == 0);
			REQUIRE(vInfo["getCount"] == 0);
		}
	}

	SECTION("overflow")
	{
		REQUIRE_THROWS_AS(fnScenario(Dictionary({
			{"maxSize", 100},
			{"warnSize", 70},
			{"tag", "test_tag"}
		}), 200, 100), error::LimitExceeded);
		REQUIRE(pCurrQueue != nullptr);
		if (pCurrQueue)
		{
			REQUIRE(pCurrQueue->getSize() == 100);
			auto vInfo = pCurrQueue->getInfo();
			REQUIRE(vInfo["size"] == 100);
			REQUIRE(vInfo["putTryCount"] == 101);
			REQUIRE(vInfo["putCount"] == 100);
			REQUIRE(vInfo["dropCount"] == 0);
			REQUIRE(vInfo["getTryCount"] == 0);
			REQUIRE(vInfo["getCount"] == 0);
		}
	}

	SECTION("overflow_with_acceptor")
	{
		auto pNotifyAcceptor = createObject<NotifyAcceptor>();

		REQUIRE_THROWS_AS(fnScenario(Dictionary({
			{"maxSize", 0x80},
			{"warnSize", 0x50},
			{"warnMask", 0x0F},
			{"tag", "test_tag"},
			{"notifyAcceptor", pNotifyAcceptor}
		}), 0xA0, 0x80), error::LimitExceeded);
		REQUIRE(pCurrQueue != nullptr);
		if (pCurrQueue)
		{
			REQUIRE(pCurrQueue->getSize() == 0x80);
			auto vInfo = pCurrQueue->getInfo();
			REQUIRE(vInfo["size"] == 0x80);
			REQUIRE(vInfo["putTryCount"] == 0x81);
			REQUIRE(vInfo["putCount"] == 0x80);
			REQUIRE(vInfo["dropCount"] == 0);
			REQUIRE(vInfo["getTryCount"] == 0);
			REQUIRE(vInfo["getCount"] == 0);
			REQUIRE(pNotifyAcceptor->m_nCount == 0x80);
			REQUIRE(pNotifyAcceptor->m_nWarnCount == 4);
			REQUIRE(pNotifyAcceptor->m_vLastTag == "test_tag");
		}
	}

	SECTION("overflow_with_circular")
	{
		auto pNotifyAcceptor = createObject<NotifyAcceptor>();

		REQUIRE_NOTHROW(fnScenario(Dictionary({
			{"circular", true},
			{"maxSize", 0x80},
			{"warnSize", 0x1000}, // Don't check warning
			{"tag", "test_tag"},
			{"notifyAcceptor", pNotifyAcceptor}
			}), 0xA0, 0x80));
		REQUIRE(pCurrQueue != nullptr);
		REQUIRE(pCurrQueue->getSize() == 0x80);
		auto vInfo = pCurrQueue->getInfo();
		REQUIRE(vInfo["size"] == 0x80);
		REQUIRE(vInfo["putTryCount"] == 0xA0);
		REQUIRE(vInfo["putCount"] == 0xA0);
		REQUIRE(vInfo["dropCount"] == 0x20);
		REQUIRE(vInfo["getTryCount"] == 0);
		REQUIRE(vInfo["getCount"] == 0);
		REQUIRE(pNotifyAcceptor->m_nCount == 0x80);
		REQUIRE(pNotifyAcceptor->m_vLastTag == "test_tag");
	}
}

//
//
//
TEST_CASE("Queue.rollback")
{
	ObjPtr<IQueue> pCurrQueue;
	
	auto fnScenario = [&](const Variant& vConfig, Size nPutCount, Size nRollbackCount)
	{
		auto pObj = queryInterface<IDataReceiver>(createObject(CLSID_Queue, vConfig));
		pCurrQueue = queryInterface<IQueue>(pObj);

		// put data
		for (Size nCounter = 0; nCounter < nPutCount; ++nCounter)
		{
			REQUIRE(pCurrQueue->getSize() == nCounter);
			pObj->put(generateData(nCounter));
		}
		REQUIRE(nPutCount == pCurrQueue->getSize());

		// roll data back
		for (Size nCounter = 0; nCounter < nRollbackCount; ++nCounter)
		{
			REQUIRE(pCurrQueue->getSize() == nCounter + nPutCount);
			pCurrQueue->rollback(generateData(nCounter + nPutCount));
		}
		REQUIRE(nRollbackCount + nPutCount == pCurrQueue->getSize());

		// get all data and check it
		auto pGetter = queryInterface<IDataProvider>(pObj);
		for (Size nCounter = 0; nCounter < nRollbackCount + nPutCount; ++nCounter)
		{
			auto v = pGetter->get();
			REQUIRE(v.has_value());

			Size nExpectedId = nCounter < nRollbackCount ?
				nPutCount + nRollbackCount - 1 - nCounter : nCounter - nRollbackCount;

			REQUIRE(v.value()["id"] == nExpectedId);
			REQUIRE(v.value()["size"] == v.value().getSize());
		}
	};

	SECTION("regular")
	{
		auto pNotifyAcceptor = createObject<NotifyAcceptor>();
		REQUIRE_NOTHROW(fnScenario(Dictionary({
			{"maxSize", 0x80},
			{"notifyAcceptor", pNotifyAcceptor},
			{"tag", "test_tag"}
			}), 0x10, 0x10));

		REQUIRE(pCurrQueue != nullptr);

		auto vInfo = pCurrQueue->getInfo();
		REQUIRE(vInfo["size"] == 0);
		REQUIRE(vInfo["putTryCount"] == 0x10);
		REQUIRE(vInfo["putCount"] == 0x10);
		REQUIRE(vInfo["rollbackCount"] == 0x10);
		REQUIRE(vInfo["dropCount"] == 0);
		REQUIRE(vInfo["getTryCount"] == 0x20);
		REQUIRE(vInfo["getCount"] == 0x20);

		REQUIRE(pNotifyAcceptor->m_nCount == 0x20);
		REQUIRE(pNotifyAcceptor->m_nWarnCount == 0);
		REQUIRE(pNotifyAcceptor->m_vLastTag == "test_tag");
	}
	 
	SECTION("overflow")
	{  
		auto pNotifyAcceptor = createObject<NotifyAcceptor>();
		REQUIRE_NOTHROW(fnScenario(Dictionary({
			{"maxSize", 0x80},
			{"notifyAcceptor", pNotifyAcceptor},
			{"tag", "test_tag"}
			}), 0x80, 0x10));

		REQUIRE(pCurrQueue != nullptr);
		 
		auto vInfo = pCurrQueue->getInfo();
		REQUIRE(vInfo["size"] == 0);
		REQUIRE(vInfo["putTryCount"] == 0x80);
		REQUIRE(vInfo["putCount"] == 0x80);
		REQUIRE(vInfo["rollbackCount"] == 0x10);
		REQUIRE(vInfo["dropCount"] == 0);
		REQUIRE(vInfo["getTryCount"] == 0x90);
		REQUIRE(vInfo["getCount"] == 0x90);

		REQUIRE(pNotifyAcceptor->m_nCount == 0x90);
		REQUIRE(pNotifyAcceptor->m_nWarnCount == 0);
		REQUIRE(pNotifyAcceptor->m_vLastTag == "test_tag");
	}
}

//
// 
//
TEST_CASE("Queue.get")
{
	ObjPtr<IQueue> pCurrQueue;

	auto fnScenario = [&](const Variant& vConfig, Size nPutCount, Size nGetCount) -> Size
	{
		Size nMaxSize = vConfig.get("maxSize", 100000);
		auto pObj = queryInterface<IDataReceiver>(createObject(CLSID_Queue, vConfig));
		pCurrQueue = queryInterface<IQueue>(pObj);
		for (Size nCounter = 0; nCounter < nPutCount; ++nCounter)
		{
			REQUIRE(pCurrQueue->getSize() == std::min<Size>(nCounter, nMaxSize));
			pObj->put(generateData(nCounter));
		}

		Size nSizeAfterPut = std::min<Size>(nPutCount, nMaxSize);
		Size nFirstGetId = nPutCount - nSizeAfterPut;
		REQUIRE(pCurrQueue->getSize() == nSizeAfterPut);

		Size nRealGets = 0;
		auto pGetter = queryInterface<IDataProvider>(pObj);
		for (Size nCounter = 0; nCounter < nGetCount; ++nCounter)
		{
			if (nSizeAfterPut >= nCounter)
				REQUIRE(nSizeAfterPut - nCounter == pCurrQueue->getSize());
			else
				REQUIRE(0 == pCurrQueue->getSize());

			auto v = pGetter->get();
			if (v.has_value()) ++nRealGets;
			if (v.has_value())
			{
				REQUIRE(v.value()["id"] == nCounter + nFirstGetId);
				REQUIRE(v.value()["size"] == v.value().getSize());
			}
		}

		return nRealGets;
	};

	SECTION("regular")
	{
		Size nRealGets = 0;
		REQUIRE_NOTHROW(nRealGets = fnScenario(Dictionary(), 10, 10));
		REQUIRE(pCurrQueue->getSize() == 0);
		REQUIRE(nRealGets == 10);
		if (pCurrQueue)
		{
			auto vInfo = pCurrQueue->getInfo();
			REQUIRE(vInfo["size"] == 0);
			REQUIRE(vInfo["putTryCount"] == 10);
			REQUIRE(vInfo["putCount"] == 10);
			REQUIRE(vInfo["dropCount"] == 0x0);
			REQUIRE(vInfo["getTryCount"] == 10);
			REQUIRE(vInfo["getCount"] == 10);
		}
	}

	SECTION("disabled")
	{		
		REQUIRE_NOTHROW(pCurrQueue = queryInterface<IQueue>(createObject(CLSID_Queue, Dictionary({
			{"maxSize", 100},
			{"mode", QueueMode::Put},
			{"warnSize", 70},
			{"tag", "test_tag"}
		}))));

		auto pReceiver = queryInterface<IDataReceiver>(pCurrQueue);
		auto pProvider = queryInterface<IDataProvider>(pCurrQueue);

		REQUIRE_NOTHROW(putDataRange(pReceiver, 0, 100));
		std::atomic<bool> fStop(true);
		REQUIRE_NOTHROW(getData(pProvider, fStop));

		REQUIRE(pCurrQueue->getSize() == 100);
		if (pCurrQueue)
		{
			auto vInfo = pCurrQueue->getInfo();
			REQUIRE(vInfo["size"] == 100);
			REQUIRE(vInfo["putTryCount"] == 100);
			REQUIRE(vInfo["putCount"] == 100);
			REQUIRE(vInfo["dropCount"] == 0x0);
			REQUIRE(vInfo["getTryCount"] == 1);
			REQUIRE(vInfo["getCount"] == 0);
		}
	}

	SECTION("underflow")
	{
		auto pNotifyAcceptor = createObject<NotifyAcceptor>();
		Size nRealGets = 0;
		REQUIRE_NOTHROW(nRealGets = fnScenario(Dictionary({
			{"maxSize", 0x80},
			{"warnSize", 0x50},
			{"warnMask", 0x0F},
			{"notifyAcceptor", pNotifyAcceptor},
			{"tag", "test_tag"}
		}), 0x80, 0xA0));
		REQUIRE(pCurrQueue != nullptr);
		REQUIRE(nRealGets == 0x80);
		if (pCurrQueue)
		{
			REQUIRE(pCurrQueue->getSize() == 0);
			auto vInfo = pCurrQueue->getInfo();
			REQUIRE(vInfo["size"] == 0);
			REQUIRE(vInfo["putTryCount"] == 0x80);
			REQUIRE(vInfo["putCount"] == 0x80);
			REQUIRE(vInfo["dropCount"] == 0x0);
			REQUIRE(vInfo["getTryCount"] == 0xA0);
			REQUIRE(vInfo["getCount"] == 0x80);
		}
		REQUIRE(pNotifyAcceptor->m_nCount == 0x80);
		REQUIRE(pNotifyAcceptor->m_nWarnCount == 4);
		REQUIRE(pNotifyAcceptor->m_vLastTag == "test_tag");
	}

	SECTION("overflow_with_circular")
	{
		auto pNotifyAcceptor = createObject<NotifyAcceptor>();
		Size nRealGets = 0;
		REQUIRE_NOTHROW(nRealGets = fnScenario(Dictionary({
			{"circular", true},
			{"maxSize", 0x80},
			{"warnSize", 0x1000},
			{"notifyAcceptor", pNotifyAcceptor},
			{"tag", "test_tag"}
			}), 0x100, 0xA0));
		REQUIRE(pCurrQueue != nullptr);
		REQUIRE(nRealGets == 0x80);
		if (pCurrQueue)
		{
			REQUIRE(pCurrQueue->getSize() == 0);
			auto vInfo = pCurrQueue->getInfo();
			REQUIRE(vInfo["size"] == 0);
			REQUIRE(vInfo["putTryCount"] == 0x100);
			REQUIRE(vInfo["putCount"] == 0x100);
			REQUIRE(vInfo["dropCount"] == 0x80);
			REQUIRE(vInfo["getTryCount"] == 0xA0);
			REQUIRE(vInfo["getCount"] == 0x80);
		}
		REQUIRE(pNotifyAcceptor->m_nCount == 0x80);
		REQUIRE(pNotifyAcceptor->m_vLastTag == "test_tag");
	}
}

//
//
//
TEST_CASE("Queue.pause_resume")
{
	auto pReceiver = queryInterface<IDataReceiver>(createObject(CLSID_Queue));
	auto pProvider = queryInterface<IDataProvider>(pReceiver);
	auto pQueue = queryInterface<IQueue>(pReceiver);

#ifdef _DEBUG
	Size c_nDataCount = 10000;
#else
	Size c_nDataCount = 100000;
#endif // DEBUG

	uint64_t c_finalRes = 0;
	for (Size n = 0; n < c_nDataCount; ++n) c_finalRes += n;

	std::atomic<bool> fStop(false); 

	// Start put/get process
	REQUIRE(pQueue->getSize() == 0);
	pQueue->setMode(QueueMode::All);
	auto fPRes0(std::async(std::launch::async, putDataRange, pReceiver, 0, c_nDataCount));
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	REQUIRE(pQueue->getSize() != 0);
	auto fGRes1(std::async(std::launch::async, getData, pProvider, std::ref(fStop)));
	std::this_thread::sleep_for(std::chrono::milliseconds(30));
	AutoSetFlag autoSet(fStop); // This flag controller should be placed here!!!

	// Stop put
	pQueue->setMode(QueueMode::Get);
	REQUIRE_THROWS_AS(fPRes0.get(), error::OperationDeclined);
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	REQUIRE(pQueue->getSize() == 0);
	
	// Resume only put	
	pQueue->setMode(QueueMode::Put);
	Dictionary vInfo;
	REQUIRE_NOTHROW(vInfo = pQueue->getInfo());
	Size nPutSize = vInfo.get("putCount", 0);
	REQUIRE(nPutSize != 0);
	auto fPRes1(std::async(std::launch::async, putDataRange, pReceiver, nPutSize, c_nDataCount));
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	
	// Resume all operations
	pQueue->setMode(QueueMode::All);

	uint64_t nPRes = -1;
	REQUIRE_NOTHROW(nPRes = fPRes1.get());
	REQUIRE(nPRes != -1);

	uint64_t nGRes = 0;
	fStop.store(true);
	REQUIRE_NOTHROW(nGRes = fGRes1.get());

	REQUIRE(pQueue->getSize() == 0);

	Dictionary vFinalInfo;
	REQUIRE_NOTHROW(vFinalInfo = pQueue->getInfo());
	REQUIRE(vFinalInfo.get("putCount", 0) == c_nDataCount);
	REQUIRE(vFinalInfo.get("getCount", 0) == c_nDataCount);
	REQUIRE(Size(vFinalInfo.get("putTryCount", 0)) >= c_nDataCount);
	REQUIRE(Size(vFinalInfo.get("getTryCount", 0)) >= c_nDataCount);

	REQUIRE(nPRes <= c_finalRes);	// We don't know how much packets 
									//are sent on the first step
	REQUIRE(c_finalRes == nGRes);
}

//
//
//
TEST_CASE("Queue.multithread_access")
{
	auto pReceiver = queryInterface<IDataReceiver>(createObject(CLSID_Queue));
	auto pProvider = queryInterface<IDataProvider>(pReceiver);
	auto pQueue = queryInterface<IQueue>(pReceiver);

#ifdef _DEBUG
	static constexpr Size c_nDataCount = 8000;
#else
	static constexpr Size c_nDataCount = 80000;
#endif // DEBUG

	Size c_finalRes = 0;
	for (Size n = 0; n < c_nDataCount; ++n) c_finalRes += n;


	static constexpr Size c_nOnePercent = c_nDataCount / 100;

	auto fPRes1(std::async(std::launch::async, putDataRange, pReceiver, 000 * c_nOnePercent, 005 * c_nOnePercent));
	auto fPRes2(std::async(std::launch::async, putDataRange, pReceiver, 005 * c_nOnePercent, 015 * c_nOnePercent));
	auto fPRes3(std::async(std::launch::async, putDataRange, pReceiver, 015 * c_nOnePercent, 035 * c_nOnePercent));
	auto fPRes4(std::async(std::launch::async, putDataRange, pReceiver, 035 * c_nOnePercent, 075 * c_nOnePercent));
	auto fPRes5(std::async(std::launch::async, putDataRange, pReceiver, 075 * c_nOnePercent, 100 * c_nOnePercent));
	
	std::atomic<bool> fStop(false);
	auto fGRes1(std::async(std::launch::async, getData, pProvider, std::ref(fStop)));
	auto fGRes2(std::async(std::launch::async, getData, pProvider, std::ref(fStop)));
	auto fGRes3(std::async(std::launch::async, getData, pProvider, std::ref(fStop)));
	auto fGRes4(std::async(std::launch::async, getData, pProvider, std::ref(fStop)));
	auto fGRes5(std::async(std::launch::async, getData, pProvider, std::ref(fStop)));
	AutoSetFlag autoSet(fStop); // This flag controller should be placed here!!!

	uint64_t nPRes1 = -1;
	REQUIRE_NOTHROW(nPRes1 = fPRes1.get());
	REQUIRE(nPRes1 != -1);
	uint64_t nPRes2 = -1;
	REQUIRE_NOTHROW(nPRes2 = fPRes2.get());
	REQUIRE(nPRes2 != -1);
	uint64_t nPRes3 = -1;
	REQUIRE_NOTHROW(nPRes3 = fPRes3.get());
	REQUIRE(nPRes3 != -1);
	uint64_t nPRes4 = -1;
	REQUIRE_NOTHROW(nPRes4 = fPRes4.get());
	REQUIRE(nPRes4 != -1);
	uint64_t nPRes5 = -1;
	REQUIRE_NOTHROW(nPRes5 = fPRes5.get());
	REQUIRE(nPRes5 != -1);
	uint64_t nPRes = nPRes1 + nPRes2 + nPRes3 + nPRes4 + nPRes5;

	fStop = true;
	uint64_t nGRes = 0;
	REQUIRE_NOTHROW(nGRes = fGRes1.get() + fGRes2.get() + fGRes3.get()
		+ fGRes4.get() + fGRes5.get());

	Dictionary vFinalInfo;
	REQUIRE_NOTHROW(vFinalInfo = pQueue->getInfo());
	REQUIRE(vFinalInfo.get("putCount", 0) == c_nDataCount);
	REQUIRE(vFinalInfo.get("getCount", 0) == c_nDataCount);
	REQUIRE(vFinalInfo.get("putTryCount", 0) == c_nDataCount);
	
	REQUIRE(nPRes == c_finalRes);
	REQUIRE(nPRes == nGRes);
	REQUIRE(pQueue->getSize() == 0);
}

//
//
//
TEST_CASE("Queue.put_batch")
{
	ObjPtr<IQueue> pCurrQueue;

	auto fnScenario = [&](const Variant& vConfig, Size nCount, Size nExpectedCount)
	{
		Size nMaxSize = vConfig.get("maxSize", 100000);
		Size nBatchSize = vConfig.get("batchSize", 0);
		auto pObj = queryInterface<IDataReceiver>(createObject(CLSID_Queue, vConfig));
		pCurrQueue = queryInterface<IQueue>(pObj);
		for (Size i = 0; i < nCount; ++i)
		{
			if (nBatchSize != 0)
			{
				Size nCurSize = pCurrQueue->getSize();
				REQUIRE((nCurSize * nBatchSize) + (i % nBatchSize) == std::min<Size>(i, nMaxSize));
			}
			pObj->put(generateData(i));
		}
		REQUIRE(nExpectedCount == pCurrQueue->getSize());
	};

	auto fnCheckInfo = [&](Variant vExpectedInfo)
	{
		auto vInfo = pCurrQueue->getInfo();
		for (const auto&[sKey, vValue] : Dictionary(vExpectedInfo))
			CHECK(vInfo[sKey] == vValue);
	};

	const std::map<const std::string, std::tuple<Variant, Size, Size, Variant>> mapData{
		// sName: vConfig, nCount, nExpectedCount, vExpectedInfo
		{ "one_full", {
			Dictionary({ {"batchSize", 10} }),
			11,
			1,
			Dictionary({
				{"size", 1},
				{"putTryCount", 1},
				{"putCount", 1},
				{"dropCount", 0},
				{"getTryCount", 0},
				{"getCount", 0}
				}) } },
		{ "one_partial", {
			Dictionary({ {"batchSize", 10} }),
			5,
			0,
			Dictionary({
				{"size", 0},
				{"putTryCount", 0},
				{"putCount", 0},
				{"dropCount", 0},
				{"getTryCount", 0},
				{"getCount", 0}
				}) } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, nCount, nExpectedCount, vExpectedInfo ] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, nCount, nExpectedCount));
			REQUIRE_NOTHROW(fnCheckInfo(vExpectedInfo));
		}
	}

	const std::map<const std::string, std::tuple<std::string, Variant, Size, Size, Variant>> mapBad{
		// sName: sMsg, vConfig, nCount, nExpectedCount, vExpectedInfo
		{ "disabled", {
			"Operation is declined",
			Dictionary({
				{"batchSize", 100},
				{"mode", QueueMode::Off}
				}),
			101,
			0,
			Dictionary({
				{"size", 0},
				{"putTryCount", 0},
				{"putCount", 0},
				{"dropCount", 0},
				{"getTryCount", 0},
				{"getCount", 0}
				}) } }
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [sMsg, vConfig, nCount, nExpectedCount, vExpectedInfo] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(vConfig, nCount, nExpectedCount), sMsg);
			REQUIRE_NOTHROW(fnCheckInfo(vExpectedInfo));
		}
	}
}

//
//
//
TEST_CASE("Queue.put_batch_timeout")
{
	ObjPtr<IQueue> pCurrQueue;

	auto fnScenario = [&](const Variant& vConfig, Size nCount, Variant vExpectedInfo1, Variant vExpectedInfo2)
	{
		auto pObj = queryInterface<IDataReceiver>(createObject(CLSID_Queue, vConfig));
		pCurrQueue = queryInterface<IQueue>(pObj);
		Size nBatchTimeout = vConfig.get("batchTimeout", 0);
		for (Size i = 0; i < nCount; ++i)
			pObj->put(generateData(i));

		auto vInfo = pCurrQueue->getInfo();
		for (const auto&[sKey, vValue] : Dictionary(vExpectedInfo1))
			CHECK(vInfo[sKey] == vValue);

		std::this_thread::sleep_for(std::chrono::milliseconds(2 * nBatchTimeout / 3));

		vInfo = pCurrQueue->getInfo();
		for (const auto&[sKey, vValue] : Dictionary(vExpectedInfo1))
			CHECK(vInfo[sKey] == vValue);

		std::this_thread::sleep_for(std::chrono::milliseconds(3 * nBatchTimeout / 2));

		vInfo = pCurrQueue->getInfo();
		for (const auto&[sKey, vValue] : Dictionary(vExpectedInfo2))
			CHECK(vInfo[sKey] == vValue);
	};

	const std::map<const std::string, std::tuple<Variant, Size, Variant, Variant>> mapData{
		// sName: vConfig, nCount, nExpectedCount, vExpectedInfo
		{ "generic", {
			Dictionary({ {"batchSize", 10}, {"batchTimeout", 2000} }),
			15,
			Dictionary({
				{"size", 1},
				{"putTryCount", 1},
				{"putCount", 1},
				{"dropCount", 0},
				{"getTryCount", 0},
				{"getCount", 0}
			}),
			Dictionary({
				{"size", 2},
				{"putTryCount", 2},
				{"putCount", 2},
				{"dropCount", 0},
				{"getTryCount", 0},
				{"getCount", 0}
			}) 
		}}
	};

	for (const auto&[sName, params] : mapData)
	{
		const auto&[vConfig, nCount, nExpectedCount, vExpectedInfo] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, nCount, nExpectedCount, vExpectedInfo));
		}
	}
}

//
//
//
TEST_CASE("Queue.fitration")
{
	// Create queue
	auto pNotifyAcceptor = createObject<NotifyAcceptor>();
	auto pFilter = createObject<TestQueueFilter>();
	
	auto pQueue = queryInterface<IQueue>(createObject(CLSID_Queue, Dictionary({
		{"maxSize", 100},
		{"warnSize", 50},
		{"warnMask", 0x0f},
		{"tag", "test_tag"},
		{"notifyAcceptor", pNotifyAcceptor},
		{"filter", pFilter},
	})));
	auto pDataReceiver = queryInterface<IDataReceiver>(pQueue);

	// Put events
	size_t nSendEvents = 10;
	size_t nFilteredEvents = 0;

	for(size_t i=0; i<nSendEvents; ++i)
	{
		auto vEvent = generateData(i);
		if (i % 2 == 0)
		{
			++nFilteredEvents;
			vEvent.put("filterItem", true);
		}
		pDataReceiver->put(vEvent);
	}

	size_t nPassedEvents = nSendEvents - nFilteredEvents;

	REQUIRE(pQueue->getSize() == nPassedEvents);
	auto vInfo = pQueue->getInfo();
	REQUIRE(vInfo["size"] == nPassedEvents);
	REQUIRE(vInfo["putTryCount"] == nPassedEvents);
	REQUIRE(vInfo["filteredCount"] == nFilteredEvents);
	REQUIRE(vInfo["putCount"] == nPassedEvents);
	REQUIRE(vInfo["getTryCount"] == 0);
	REQUIRE(vInfo["getCount"] == 0);

	REQUIRE(pNotifyAcceptor->m_nCount == nPassedEvents);

	// Get events
	auto pDataProvider = queryInterface<IDataProvider>(pQueue);
	size_t nGetCount = 0;
	while (true)
	{
		auto vData = pDataProvider->get();
		if(!vData.has_value())
			break;
		++nGetCount;
		Size nId = vData->get("id");
		CHECK_FALSE(nId % 2 == 0);
	}
	REQUIRE(nGetCount == nPassedEvents);
}


