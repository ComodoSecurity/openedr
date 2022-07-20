//
// edrav2.libcore.unittest
//
// ThreadPool object test 
//
// Autor: Denis Bogdanov (18.01.2019)
// Reviewer: ???
//
#include "pch.h"

using namespace cmd;
using namespace std::chrono_literals;

//
//
//
TEST_CASE("ThreadPool.creation")
{
	// TODO: Delete log initialization
	LOGINF("");

	SECTION("default")
	{
		ThreadPool tp("test");
		REQUIRE(tp.getPriority() == ThreadPool::Priority::Normal);
		REQUIRE(tp.getThreadsCount() == 0);
	}

	SECTION("high_priority")
	{
		ThreadPool tp("test", 0, ThreadPool::Priority::High);
		REQUIRE(tp.getThreadsCount() == 0);
		REQUIRE(tp.getPriority() == ThreadPool::Priority::High);
	}

	SECTION("stop")
	{
		ThreadPool tp("test");
		REQUIRE(tp.getThreadsCount() == 0);
		REQUIRE_NOTHROW(tp.stop());
		REQUIRE(tp.getThreadsCount() == 0);
	}

	SECTION("stop_with_threads")
	{
		ThreadPool tp("test", 3);
		REQUIRE(tp.getThreadsCount() == 3);
		REQUIRE(tp.getPriority() == ThreadPool::Priority::Normal);
		REQUIRE_NOTHROW(tp.stop());
		REQUIRE(tp.getThreadsCount() == 0);
	}

	SECTION("start")
	{
		ThreadPool tp("test");
		REQUIRE(tp.getThreadsCount() == 0);
		Size nRetSize = 0;
		REQUIRE_NOTHROW(nRetSize = tp.addThreads(5));
		REQUIRE(nRetSize == 5);
		REQUIRE(tp.getThreadsCount() == 5);
		REQUIRE_NOTHROW(tp.stop());
		REQUIRE(tp.getThreadsCount() == 0);
	}

	SECTION("multiple_start")
	{
		ThreadPool tp("test", 3);
		REQUIRE(tp.getThreadsCount() == 3);
		REQUIRE_NOTHROW(tp.addThreads(3));
		REQUIRE(tp.getThreadsCount() == 6);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		REQUIRE_NOTHROW(tp.addThreads(3));
		REQUIRE(tp.getThreadsCount() == 9);
		REQUIRE_NOTHROW(tp.stop());
		REQUIRE(tp.getThreadsCount() == 0);
	}

}

//
//
//
void runFunc(std::atomic_int& nCount, int nTimeout, double, void* )
{
	nCount++;
	std::this_thread::sleep_for(std::chrono::milliseconds(nTimeout));
}

//
//
//
TEST_CASE("ThreadPool.run()")
{
	// TODO: Delete log initialization
	LOGINF("");

	SECTION("function")
	{
		std::atomic_int nCounter = 0;
		ThreadPool tp("test", 1);
		REQUIRE(tp.getThreadsCount() == 1);
		tp.run(runFunc, std::ref(nCounter), 500, 0.8, nullptr);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		REQUIRE(Size(nCounter) == 1);
		REQUIRE(tp.getThreadsCount() == 1);
	}

	SECTION("lambda")
	{
		std::atomic_int nCounter = 0;
		ThreadPool tp("test", 1);
		REQUIRE(tp.getThreadsCount() == 1);
		tp.run([](std::atomic_int& nCount) {nCount++; }, std::ref(nCounter));
		std::this_thread::sleep_for(std::chrono::seconds(1));
		REQUIRE(Size(nCounter) == 1);
		REQUIRE(tp.getThreadsCount() == 1);
	}

	SECTION("multiple_run")
	{
		std::atomic_int nCounter = 0;
		ThreadPool tp("test", 3);
		REQUIRE(tp.getThreadsCount() == 3);
		for (int i = 0; i < 10; ++i)
			tp.run(runFunc, std::ref(nCounter), 200 + (100 * i), 0.8, nullptr);
		std::this_thread::sleep_for(std::chrono::seconds(5));
		REQUIRE(Size(nCounter) == 10);
		REQUIRE(tp.getThreadsCount() == 3);
	}

}

//
//
//
TEST_CASE("ThreadPool.runWithDelay()")
{
	// TODO: Delete log initialization
	LOGINF("");

	SECTION("once")
	{
		std::atomic_int nCounter = 0;
		ThreadPool tp("test", 1);
		REQUIRE(tp.getThreadsCount() == 1);
		auto pCtx = tp.runWithDelay(2000, [](std::atomic_int& nCount)
			{nCount++; return false; }, std::ref(nCounter));
		std::this_thread::sleep_for(std::chrono::seconds(1));
		REQUIRE(Size(nCounter) == 0);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		REQUIRE(Size(nCounter) == 1);
		REQUIRE(tp.getThreadsCount() == 1);
	}

	SECTION("loop")
	{
		std::atomic_int nCounter = 0;
		ThreadPool tp("test", 1);
		REQUIRE(tp.getThreadsCount() == 1);
		auto pCtx = tp.runWithDelay(200, [](std::atomic_int& nCount)
			{nCount++; return nCount < 5; }, std::ref(nCounter));
		std::this_thread::sleep_for(std::chrono::seconds(2));
		REQUIRE(Size(nCounter) == 5);
		REQUIRE(tp.getThreadsCount() == 1);
	}

	SECTION("infinite_loop")
	{
		std::atomic_int nCounter = 0;
		ThreadPool tp("test", 1);
		REQUIRE(tp.getThreadsCount() == 1);
		{
			auto pCtx = tp.runWithDelay(50, [](std::atomic_int& nCount)
				{nCount++; return true; }, std::ref(nCounter));
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
		REQUIRE(Size(nCounter) > 5);
		int nOldCounter = nCounter;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		REQUIRE(Size(nCounter) == nOldCounter);
		REQUIRE(tp.getThreadsCount() == 1);
	}

	SECTION("concurent_loop")
	{
		std::atomic_int nCounter = 0;
		ThreadPool tp("test", 5);
		REQUIRE(tp.getThreadsCount() == 5);
		std::vector<ThreadPool::TimerContextPtr> runsCtx;
		for (Size i = 0; i < 100; i++)
			runsCtx.push_back(tp.runWithDelay(50 + i * 10, [](std::atomic_int& nCount)
				{nCount++; return true; }, std::ref(nCounter)));
		std::this_thread::sleep_for(std::chrono::seconds(5));
		Size nCount = Size(nCounter);
		REQUIRE(nCount > 100);
		REQUIRE_NOTHROW(runsCtx.clear()); 
		int nOldCounter = nCounter;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		REQUIRE(Size(nCounter) == nOldCounter);
		REQUIRE(tp.getThreadsCount() == 5);
	}

}

//
//
//
TEST_CASE("ThreadPool.TimerContext_cancel()")
{
	// TODO: Delete log initialization
	LOGINF("");

	std::atomic_int nCounter = 0;
	ThreadPool tp("test", 1);
	REQUIRE(tp.getThreadsCount() == 1);
	auto pCtx = tp.runWithDelay(100, [](std::atomic_int& nCount)
	{nCount++; return true; }, std::ref(nCounter));
	std::this_thread::sleep_for(std::chrono::seconds(2));
	REQUIRE(Size(nCounter) > 5);
	REQUIRE_NOTHROW(pCtx->cancel());
	int nOldCounter = nCounter;
	std::this_thread::sleep_for(std::chrono::seconds(2));
	REQUIRE(Size(nCounter) == nOldCounter);
	REQUIRE(tp.getThreadsCount() == 1);
}

//
//
//
TEST_CASE("ThreadPool.TimerContext_reschedule()")
{
	// TODO: Delete log initialization
	LOGINF("");

	std::atomic_int nCounter = 0;
	ThreadPool tp("test", 1);
	REQUIRE(tp.getThreadsCount() == 1);
	auto pCtx = tp.runWithDelay(200, [](std::atomic_int& nCount)
	{nCount++; return true; }, std::ref(nCounter));
	std::this_thread::sleep_for(std::chrono::seconds(1));
	REQUIRE(Size(nCounter) > 3);
	REQUIRE_NOTHROW(pCtx->reschedule(50));
	std::this_thread::sleep_for(std::chrono::seconds(1));
	REQUIRE(Size(nCounter) > 15);
	REQUIRE(tp.getThreadsCount() == 1);
}

//
//
//
TEST_CASE("ThreadPool.threads_control")
{
	// TODO: Delete log initialization
	LOGINF("");

	SECTION("add")
	{
		ThreadPool tp("test", 3);
		REQUIRE(tp.getThreadsCount() == 3);
		REQUIRE(tp.addThreads(1) == 4);
		REQUIRE(tp.getThreadsCount() == 4);
		REQUIRE_NOTHROW(tp.stop());
		REQUIRE(tp.getThreadsCount() == 0);
	}

	SECTION("delete")
	{
		ThreadPool tp("test", 3);
		REQUIRE(tp.getThreadsCount() == 3);
		tp.deleteThread();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		REQUIRE(tp.getThreadsCount() == 2);
		REQUIRE_NOTHROW(tp.stop());
		REQUIRE(tp.getThreadsCount() == 0);
	}

	SECTION("change")
	{
		ThreadPool tp("test", 3);
		std::atomic_int nCounter = 0;
		REQUIRE(tp.getThreadsCount() == 3);
		for (int i = 0; i < 100; ++i)
			tp.run(runFunc, std::ref(nCounter), 100 + (50 * (i & 0x0F)), 0.8, nullptr);
		REQUIRE(tp.getThreadsCount() == 3);
		REQUIRE(tp.addThreads(1) == 4);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		REQUIRE(tp.addThreads(1) == 5);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		tp.deleteThread();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		tp.deleteThread();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		tp.deleteThread();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		REQUIRE_NOTHROW(tp.stop());
		REQUIRE(tp.getThreadsCount() == 0);
		REQUIRE(Size(nCounter) == 100);
	}
}

//
//
//
TEST_CASE("ThreadPool.global_run()")
{
	std::atomic_int nCounter = 0;
	run([&nCounter]() {nCounter++; });
	run([&nCounter]() {nCounter++; });
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	REQUIRE(Size(nCounter) == 2);
	run([&nCounter]() {nCounter++; });
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	REQUIRE(Size(nCounter) == 3);
}

//
//
//
TEST_CASE("ThreadPool.global_runWithDelay()")
{
	std::atomic_int nCounter = 0;
	auto r1 = runWithDelay(1000, [&nCounter]() -> bool {nCounter++; return false; });
	auto r2 = runWithDelay(2000, [&nCounter]() -> bool {nCounter++; return false; });
	auto r3 = runWithDelay(3000, [&nCounter]() -> bool {nCounter++; return false; });
	REQUIRE(Size(nCounter) == 0);
	std::this_thread::sleep_for(std::chrono::milliseconds(1500));
	REQUIRE(Size(nCounter) == 1);
	REQUIRE(!r1->isActive());
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	REQUIRE(Size(nCounter) == 2); 
	REQUIRE(!r2->isActive());
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	REQUIRE(Size(nCounter) == 3);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	REQUIRE(!r3->isActive());
}

//
//
//
class TestCommandProcessor : public ObjectBase<>, public ICommandProcessor
{
	int nCount = 0;
public:
	int getCount() { return nCount;}
	virtual Variant execute(Variant vCommand, Variant vParams) override
	{
		if (vCommand == "testTrue") { ++nCount; return true; }
		if (vCommand == "testFalse") { ++nCount; return false;}
		if (vCommand == "test3") { ++nCount; return nCount < 3; }
		error::InvalidArgument(SL).throwException();
		return {};
	}
};


//
//
//
TEST_CASE("ThreadPool.global_startTask(functor)")
{
	SECTION("default")
	{
		clearCatalog();
		std::atomic_int nCounter = 0;
		auto r1 = startTask("task1", 500, [&nCounter]() -> bool {nCounter++; return false; });
		REQUIRE(Size(nCounter) == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		REQUIRE(Size(nCounter) == 1);
		auto r1_1 = getTask("task1");
		REQUIRE(r1 == r1_1);
	}

	SECTION("loop")
	{
		clearCatalog();
		std::atomic_int nCounter = 0;
		auto r1 = startTask("task1", 500, [&nCounter]() -> bool {nCounter++; return true; });
		REQUIRE(Size(nCounter) == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		REQUIRE(Size(nCounter) > 2);
		auto r1_1 = getTask("task1");
		REQUIRE(r1 == r1_1);
	}

	SECTION("restart")
	{
		clearCatalog();
		std::atomic_int nCounter = 0;
		auto r1 = startTask("task1", 100, [&nCounter]() -> bool {nCounter++; return false; });
		REQUIRE_THROWS_AS(startTask("task1", 100, [&nCounter]() -> bool {nCounter++; return false; }), error::AlreadyExists);
		auto r3 = startTask("task1");
		REQUIRE(r1 == r3);
		REQUIRE(Size(nCounter) == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		REQUIRE(Size(nCounter) == 1);
	}

	SECTION("multithread")
	{
		clearCatalog();
		std::atomic_int nCounter = 0;
		auto r1 = startTask("task1", 1000, [&nCounter]() -> bool {nCounter++; return false; });
		auto r2 = startTask("task2", 2000, [&nCounter]() -> bool {nCounter++; return false; });
		auto r3 = startTask("task3", 3000, [&nCounter]() -> bool {nCounter++; return true; });
		REQUIRE(Size(nCounter) == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1500));
		REQUIRE(Size(nCounter) == 1);
		REQUIRE(!r1->isActive());
		auto r1_1 = startTask("task1");
		REQUIRE(r1->isActive());
		REQUIRE(r1 == r1_1);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		REQUIRE(Size(nCounter) == 3);
		REQUIRE(!r2->isActive());
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		REQUIRE(Size(nCounter) == 4);
		REQUIRE(r3->isActive());
	}

	clearCatalog();
}

//
//
//
TEST_CASE("ThreadPool.global_startTask(ICommand)")
{
	// TODO: Delete log initialization
	LOGINF("");

	SECTION("default")
	{
		clearCatalog();
		auto pCP = createObject<TestCommandProcessor>();
		auto pCmd = createCommand(pCP, "testFalse", {});
		auto r1 = startTask("task1", 500, pCmd);
		REQUIRE(pCP->getCount() == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		REQUIRE(pCP->getCount() == 1);
		auto r1_1 = getTask("task1");
		REQUIRE(r1 == r1_1);
	}

	SECTION("loop")
	{
		clearCatalog();
		auto pCP = createObject<TestCommandProcessor>();
		auto pCmd = createCommand(pCP, "testTrue", {});
		auto r1 = startTask("task1", 500, pCmd);
		REQUIRE(pCP->getCount() == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		REQUIRE(pCP->getCount() > 2);
		auto r1_1 = getTask("task1");
		REQUIRE(r1 == r1_1);
	}

	SECTION("restart")
	{
		auto pCP = createObject<TestCommandProcessor>();
		auto pCmd = createCommand(pCP, "testFalse", {});
		auto r1 = startTask("task1", 500, pCmd);
		REQUIRE(pCP->getCount() == 0);
		REQUIRE_THROWS_AS(startTask("task1", 500, pCmd), error::AlreadyExists);
		auto r3 = startTask("task1");
		REQUIRE(r1 == r3);
		REQUIRE(pCP->getCount() == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		REQUIRE(pCP->getCount() == 1);
	}

	clearCatalog();
}

//
//
//
TEST_CASE("ThreadPool.global_stopTask(functor)")
{
	// TODO: Delete log initialization
	LOGINF("");
	clearCatalog();
	std::atomic<Size> nCounter1 = 0;

	SECTION("stop")
	{
		auto r1 = startTask("task1", 500, [&nCounter1]() -> bool {nCounter1++; return true; });
		REQUIRE(Size(nCounter1) == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1500));
		REQUIRE(Size(nCounter1) > 0);
		stopTask("task1", false);
		Size nOldCounter1 = nCounter1;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		REQUIRE(Size(nCounter1) - nOldCounter1 <= 1);
		auto task1 = startTask("task1");
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		REQUIRE(Size(nCounter1) > nOldCounter1);
	}

	SECTION("stop_and_delete")
	{
		auto r1 = startTask("task1", 100, [&nCounter1]() -> bool {nCounter1++; return true; });
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		REQUIRE(Size(nCounter1) > 0);
		stopTask("task1", true);
		Size nOldCounter1 = nCounter1;
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		REQUIRE(Size(nCounter1) - nOldCounter1 <= 1);
		REQUIRE_THROWS_AS(startTask("task1"), error::NotFound);
	}

	SECTION("self_stop")
	{
		auto r1 = startTask("task1", 300, [&nCounter1]() -> bool {nCounter1++; return nCounter1 < 3; });
		REQUIRE(Size(nCounter1) == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1500));
		REQUIRE(Size(nCounter1) == 3);
	}

	SECTION("multithread")
	{
		std::atomic<Size> nCounter2 = 0;
		auto r1 = startTask("task1", 100, [&nCounter1]() -> bool {nCounter1++; return true; });
		auto r2 = startTask("task2", 200, [&nCounter2]() -> bool {nCounter2++; return true; });
		REQUIRE(Size(nCounter1) == 0);
		REQUIRE(Size(nCounter2) == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		REQUIRE(Size(nCounter2) > 2);
		REQUIRE(Size(nCounter1) > Size(nCounter2));
		Size nOldCounter1 = nCounter1;
		Size nOldCounter2 = nCounter2;
		stopTask("task1", false);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		REQUIRE(Size(nCounter2) > nOldCounter2);
		REQUIRE(Size(nCounter1) - nOldCounter1 <= 1);
		nOldCounter2 = nCounter2;
		auto task1 = startTask("task1");
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		REQUIRE(Size(nCounter2) > nOldCounter2);
		REQUIRE(Size(nCounter1) > nOldCounter1);
		nOldCounter1 = nCounter1;
		nOldCounter2 = nCounter2;
		stopTask("task1", true);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		REQUIRE(Size(nCounter2) > nOldCounter2);
		REQUIRE(Size(nCounter1) - nOldCounter1 <= 1);
		REQUIRE_THROWS_AS(startTask("task1"), error::NotFound);
	}

	clearCatalog();
}

//
//
//
TEST_CASE("ThreadPool.global_stopTask(ICommand)")
{
	// TODO: Delete log initialization
	LOGINF("");

	clearCatalog();
	auto pCP1 = createObject<TestCommandProcessor>();

	SECTION("stop")
	{
		auto pCmd1 = createCommand(pCP1, "testTrue", {});
		auto r1 = startTask("task1", 100, pCmd1);
		REQUIRE(pCP1->getCount() == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		REQUIRE(pCP1->getCount() > 0);
		stopTask("task1", false);
		int nOldCounter1 = pCP1->getCount();
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		REQUIRE(pCP1->getCount() - nOldCounter1 <= 1);
		auto task1 = startTask("task1");
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		REQUIRE(pCP1->getCount() > nOldCounter1);
	}

	SECTION("stop_and_delete")
	{
		auto pCmd1 = createCommand(pCP1, "testTrue", {});
		auto r1 = startTask("task1", 100, pCmd1);
		REQUIRE(pCP1->getCount() == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		REQUIRE(pCP1->getCount() > 0);
		stopTask("task1", true);
		int nOldCounter1 = pCP1->getCount();
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		REQUIRE(pCP1->getCount() - nOldCounter1 <= 1);
		REQUIRE_THROWS_AS(startTask("task1"), error::NotFound);
	}

	SECTION("self_stop")
	{
		auto pCmd1 = createCommand(pCP1, "test3", {});
		auto r1 = startTask("task1", 100, pCmd1);
		REQUIRE(pCP1->getCount() == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		REQUIRE(pCP1->getCount() == 3);
	}

	SECTION("multithread")
	{
		auto pCP2 = createObject<TestCommandProcessor>();
		auto pCmd1 = createCommand(pCP1, "testTrue", {});
		auto pCmd2 = createCommand(pCP2, "testTrue", {});
		auto r1 = startTask("task1", 100, pCmd1);
		auto r2 = startTask("task2", 200, pCmd2);
		REQUIRE(pCP1->getCount() == 0);
		REQUIRE(pCP2->getCount() == 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		REQUIRE(pCP2->getCount() > 2);
		REQUIRE(pCP1->getCount() > pCP2->getCount());
		int nOldCounter1 = pCP1->getCount();
		int nOldCounter2 = pCP2->getCount();
		stopTask("task1", false);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		REQUIRE(pCP2->getCount() > nOldCounter2);
		REQUIRE(pCP1->getCount() - nOldCounter1 <= 1);
		nOldCounter2 = pCP2->getCount();
		auto task1 = startTask("task1");
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		REQUIRE(pCP2->getCount() > nOldCounter2);
		REQUIRE(pCP1->getCount() > nOldCounter1);
		nOldCounter1 = pCP1->getCount();
		nOldCounter2 = pCP2->getCount();
		stopTask("task1", true);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		REQUIRE(pCP2->getCount() > nOldCounter2);
		REQUIRE(pCP1->getCount() - nOldCounter1 <= 1);
		REQUIRE_THROWS_AS(startTask("task1"), error::NotFound);
	}

	clearCatalog();
}
