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

//
//
//
ObjPtr<IQueueManager> createQueueManager(Variant vConfig)
{
	auto pObj = queryInterface<IQueueManager>(createObject(CLSID_QueueManager, vConfig));
	auto pCommandProcessor = queryInterface<ICommandProcessor>(pObj);
	Variant vInfo = pCommandProcessor->execute("getInfo");
	(void) vInfo["size"];
	return pObj;
}

//
//
//
TEST_CASE("QueueManager.creation")
{
	SECTION("good_config")
	{
		ObjPtr<IQueueManager> pCurrQueueMgr;
		auto pNotifyAcceptor1 = createObject<NotifyAcceptor>();
		auto pNotifyAcceptor2 = createObject<NotifyAcceptor>();

		REQUIRE_NOTHROW(pCurrQueueMgr = createQueueManager(Dictionary({
			{"queues", Dictionary({
				{"queue1", Dictionary({
					{"maxSize", 100},
					{"warnSize", 50},
					{"tag", "test_tag1"},
					{"notifyAcceptor", pNotifyAcceptor1},
				})},
				{"queue2", Dictionary({
					{"maxSize", 200},
					{"warnSize", 100},
					{"tag", "test_tag2"},
				})},
			})},
			{"notifyAcceptor", pNotifyAcceptor2},
		})));
		auto vInfo = Dictionary();
		REQUIRE_NOTHROW(vInfo = pCurrQueueMgr->getInfo());
		REQUIRE(vInfo.get("size", 0) == 2);
		ObjPtr<IQueue> pQueue1;
		REQUIRE_NOTHROW(pQueue1 = pCurrQueueMgr->getQueue("queue1"));
		REQUIRE(pQueue1->getNotificationAcceptor() == pNotifyAcceptor1);
		ObjPtr<IQueue> pQueue2;
		REQUIRE_NOTHROW(pQueue2 = pCurrQueueMgr->getQueue("queue2"));
		REQUIRE(pQueue2->getNotificationAcceptor() == pNotifyAcceptor2);
	}

	SECTION("invalid_config_param")
	{
		auto pNotifyAcceptor = createObject<NotifyAcceptor>();
		ObjPtr<IQueueManager> pCurrQueueMgr;

		REQUIRE_THROWS_AS(pCurrQueueMgr = createQueueManager(Dictionary({
			{"queues", 100},
			{"notifyAcceptor", pNotifyAcceptor}
		})), error::TypeError);
	}

	SECTION("invalid_config_format")
	{
		ObjPtr<IQueueManager> pCurrQueueMgr;
		REQUIRE_THROWS_AS(pCurrQueueMgr = createQueueManager(Variant(42)), error::InvalidArgument);
	}
}

//
//
//
TEST_CASE("QueueManager.start()")
{
	SECTION("default")
	{
		ObjPtr<IQueueManager> pCurrQueueMgr;
		REQUIRE_NOTHROW(pCurrQueueMgr = createQueueManager(Dictionary({
			{"queues", Dictionary({
				{"queue1", Dictionary()},
				{"queue2", Dictionary()},
			})},
		})));

		ObjPtr<IQueue> pQueue;
		REQUIRE_NOTHROW(pQueue = pCurrQueueMgr->getQueue("queue2"));
		auto pPutter = queryInterface<IDataReceiver>(pQueue);
		REQUIRE_THROWS_AS(pPutter->put(generateData(1)), error::OperationDeclined);
		auto pService = queryInterface<IService>(pCurrQueueMgr);
		REQUIRE_NOTHROW(pService->start());
		REQUIRE_NOTHROW(pPutter->put(generateData(1)));
	}

	SECTION("autostart")
	{
		ObjPtr<IQueueManager> pCurrQueueMgr;
		REQUIRE_NOTHROW(pCurrQueueMgr = createQueueManager(Dictionary({
			{"autoStart", true}, 
			{"queues", Dictionary({
				{"queue1", Dictionary()},
				{"queue2", Dictionary()},
			})},
		})));

		ObjPtr<IQueue> pQueue;
		REQUIRE_NOTHROW(pQueue = pCurrQueueMgr->getQueue("queue2"));
		auto pPutter = queryInterface<IDataReceiver>(pQueue);
		REQUIRE_NOTHROW(pPutter->put(generateData(1)));
		auto pService = queryInterface<IService>(pCurrQueueMgr);
		REQUIRE_NOTHROW(pService->start());
		REQUIRE_NOTHROW(pPutter->put(generateData(2)));
	}
}

//
//
//
TEST_CASE("QueueManager.stop()")
{
	ObjPtr<IQueueManager> pCurrQueueMgr;

	REQUIRE_NOTHROW(pCurrQueueMgr = createQueueManager(Dictionary({
		{"autoStart", true},
		{"queues", Dictionary({
			{"queue1", Dictionary()},
			{"queue2", Dictionary()},
		})},
	})));

	ObjPtr<IQueue> pQueue2;
	REQUIRE_NOTHROW(pQueue2 = pCurrQueueMgr->getQueue("queue2"));
	auto pPutter2 = queryInterface<IDataReceiver>(pQueue2);
	REQUIRE_NOTHROW(pPutter2->put(generateData(1)));
	REQUIRE_NOTHROW(pPutter2->put(generateData(2)));
	
	auto pService = queryInterface<IService>(pCurrQueueMgr);
	REQUIRE_NOTHROW(pService->stop());
	REQUIRE_THROWS_AS(pPutter2->put(generateData(3)), error::OperationDeclined);
	auto pQueue3 = pCurrQueueMgr->addQueue("queue3", Dictionary());
	auto pPutter3 = queryInterface<IDataReceiver>(pQueue3);
	REQUIRE_THROWS_AS(pPutter3->put(generateData(1)), error::OperationDeclined);
	auto pGetter2 = queryInterface<IDataProvider>(pQueue2);
	REQUIRE_NOTHROW(pGetter2->get() != nullptr);
	
	REQUIRE_NOTHROW(pService->shutdown());
	REQUIRE_THROWS_AS(pPutter2->put(generateData(4)), error::OperationDeclined);
	REQUIRE_NOTHROW(pQueue2->getSize() == 1);
	REQUIRE_NOTHROW(pGetter2->get() == nullptr);

	auto vInfo = Dictionary();
	REQUIRE_NOTHROW(vInfo = pCurrQueueMgr->getInfo());
	REQUIRE(vInfo.get("size", 0) == 0);
}

//
//
//
TEST_CASE("QueueManager.addQueue()")
{
	ObjPtr<IQueueManager> pCurrQueueMgr;
	REQUIRE_NOTHROW(pCurrQueueMgr = createQueueManager(Dictionary({
		{"queues", Dictionary({
			{"queue1", Dictionary()},
			{"queue2", Dictionary()},
		})},
	})));

	(void)pCurrQueueMgr->addQueue("queue3", Dictionary());
	auto vInfo = Dictionary();
	REQUIRE_NOTHROW(vInfo = pCurrQueueMgr->getInfo());
	REQUIRE(vInfo.get("size", 0) == 3);
	ObjPtr<IQueue> pQueue3;
	REQUIRE_NOTHROW(pQueue3 = pCurrQueueMgr->getQueue("queue3"));
}

//
//
//
TEST_CASE("QueueManager.deleteQueue()")
{
	ObjPtr<IQueueManager> pCurrQueueMgr;
	REQUIRE_NOTHROW(pCurrQueueMgr = createQueueManager(Dictionary({
		{"queues", Dictionary({
			{"queue1", Dictionary()},
			{"queue2", Dictionary()},
		})},
	})));

	REQUIRE_NOTHROW(pCurrQueueMgr->deleteQueue("queue1"));
	auto vInfo = Dictionary();
	REQUIRE_NOTHROW(vInfo = pCurrQueueMgr->getInfo());
	REQUIRE(vInfo.get("size", 0) == 1);
	REQUIRE_NOTHROW(pCurrQueueMgr->deleteQueue("queue1", true));
	REQUIRE_THROWS_AS(pCurrQueueMgr->deleteQueue("queue1"), error::InvalidArgument);

	REQUIRE_NOTHROW(nullptr == pCurrQueueMgr->getQueue("queue1", true));
	REQUIRE_THROWS_AS(pCurrQueueMgr->getQueue("queue1"), error::InvalidArgument);

	REQUIRE_NOTHROW(pCurrQueueMgr->deleteQueue("queue2"));
	REQUIRE_NOTHROW(vInfo = pCurrQueueMgr->getInfo());
	REQUIRE(vInfo.get("size", 0) == 0);
}

//
//
//
TEST_CASE("QueueManager.saveState()")
{
	ObjPtr<IQueueManager> pCurrQueueMgr;

	REQUIRE_NOTHROW(pCurrQueueMgr = createQueueManager(Dictionary({
		{"autoStart", true},
		{"queues", Dictionary({
			{"queue1", Dictionary({{"maxSize", 100}})},
			{"queue2", Dictionary({{"maxSize", 200}, {"persistent", true}})},
			{"queue3", Dictionary({{"maxSize", 300}})},
		})},
	})));

	ObjPtr<IQueue> pQueue2;
	REQUIRE_NOTHROW(pQueue2 = pCurrQueueMgr->getQueue("queue2"));
	auto pPutter2 = queryInterface<IDataReceiver>(pQueue2);
	REQUIRE_NOTHROW(pPutter2->put(generateData(1)));
	REQUIRE_NOTHROW(pPutter2->put(generateData(2)));

	auto pService = queryInterface<IService>(pCurrQueueMgr);
	Variant vData;
	REQUIRE_NOTHROW(vData = pService->saveState());
	REQUIRE(vData.isDictionaryLike());
	REQUIRE(vData.getSize() == 1);
	REQUIRE(vData.has("queue2"));
}

//
//
//
TEST_CASE("QueueManager.loadState()")
{
	ObjPtr<IQueueManager> pCurrQueueMgr;
	auto pNotifyAcceptor = createObject<NotifyAcceptor>();

	REQUIRE_NOTHROW(pCurrQueueMgr = createQueueManager(Dictionary({
		{"autoStart", true},
		{"notifyAcceptor", pNotifyAcceptor},
		{"queues", Dictionary({
			{"queue1", Dictionary({{"maxSize", 100}})},
			{"queue2", Dictionary({{"maxSize", 200}, {"persistent", true}})},
			{"queue3", Dictionary({{"maxSize", 300}, {"persistent", true}})},
		})},
	})));

	auto vInfo = Dictionary();
	REQUIRE_NOTHROW(vInfo = pCurrQueueMgr->getInfo());
	REQUIRE(vInfo.get("size", 0) == 3);

	auto pReceiver1 = queryInterface<IDataReceiver>(pCurrQueueMgr->getQueue("queue1"));
	pReceiver1->put(1);
	auto pReceiver2 = queryInterface<IDataReceiver>(pCurrQueueMgr->getQueue("queue2"));
	pReceiver2->put(21);
	pReceiver2->put(22);

	auto pReceiver3 = queryInterface<IDataReceiver>(pCurrQueueMgr->getQueue("queue3"));
	pReceiver3->put(31);
	pReceiver3->put(32);
	pReceiver3->put(33);
	REQUIRE(pCurrQueueMgr->getQueue("queue3")->getSize() == 3);

	auto vState = Dictionary({
			{"queue3", Dictionary({{ "items", 30 }})},
			{"queue4", Dictionary({{ "items", 41 }})}
		});

	auto pService = queryInterface<IService>(pCurrQueueMgr);
	REQUIRE_NOTHROW(pService->loadState(vState));

	REQUIRE_NOTHROW(vInfo = pCurrQueueMgr->getInfo());
	REQUIRE(vInfo.get("size", 0) == 3);

	REQUIRE_NOTHROW(pCurrQueueMgr->getQueue("queue1"));
	REQUIRE_NOTHROW(pCurrQueueMgr->getQueue("queue2"));
	REQUIRE_NOTHROW(pCurrQueueMgr->getQueue("queue3"));
	REQUIRE(pCurrQueueMgr->getQueue("queue3")->getSize() == 4);
	REQUIRE(pCurrQueueMgr->getQueue("queue3")->getNotificationAcceptor() == pNotifyAcceptor);
	REQUIRE_THROWS_AS(pCurrQueueMgr->getQueue("queue4"), error::InvalidArgument);
}