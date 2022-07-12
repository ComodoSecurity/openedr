//
// edrav2.libcore project
//
// Implementation of the Core object
//
// Author: Yury Podpruzhnikov (15.12.2018)
// Reviewer: Denis Bogdanov (25.01.2019) 
//
#include "pch.h"
#include <message.hpp>
#include <command.hpp>
#include <threadpool.hpp>

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "core"

namespace cmd {

//////////////////////////////////////////////////////////////////////////
//
// ObjectManager
//
//////////////////////////////////////////////////////////////////////////

//
//
//
class ObjectManager : public IObjectManager
{
private:
	struct ClassInfo 
	{
		Size nObjCount = 0;
		IObjectFactory* pFactory = nullptr;
	};

	typedef std::unordered_map<ClassId, ClassInfo> FactoryMap;

	std::atomic<ObjectId> m_nNextObjectId;
	FactoryMap m_FactoryMap;
	mutable std::mutex m_mtxFactoryMap;

	//
	// 
	//
	void changeObjCounter(ClassId nClassId, bool fInc)
	{
		std::scoped_lock _lock(m_mtxFactoryMap);
		FactoryMap::iterator Iter = m_FactoryMap.find(nClassId);
		if (Iter == m_FactoryMap.end())
			return;

		if (fInc)
			++(Iter->second.nObjCount);
		else
			--(Iter->second.nObjCount);
	}

public:
	//
	//
	//
	virtual ~ObjectManager() override {}

	//
	//
	//
	virtual ObjectId generateObjectId() override
	{
		return ++m_nNextObjectId;
	}

	//
	//
	//
	virtual void registerObjectFactory(ClassId nClassId, IObjectFactory* pObjectFactory) override
	{
		// Add only new ClassId
		std::scoped_lock _lock(m_mtxFactoryMap);
		if (m_FactoryMap.find(nClassId) == m_FactoryMap.end())
			m_FactoryMap[nClassId].pFactory = pObjectFactory;
	}

	//
	//
	//
	virtual ObjPtr<IObject> createInstance(ClassId nClassId, Variant vConfig, 
		SourceLocation pos) override
	{
		IObjectFactory* pObjectFactory;
		{
			std::scoped_lock _lock(m_mtxFactoryMap);
			FactoryMap::const_iterator Iter = m_FactoryMap.find(nClassId);
			if (Iter == m_FactoryMap.end())
				error::InvalidArgument(SL, FMT("Can't create object. Invalid nClassId: " << nClassId)).throwException();

			pObjectFactory = Iter->second.pFactory;

			// Deny creation of several services
			if (Iter->second.nObjCount == 1)
			{
				if (pObjectFactory->getClassInfo().fIsService)
					error::InvalidUsage(SL, FMT("Denied creation of several services. nClassId: " << nClassId)).throwException();
			}
		}

		auto pObj = pObjectFactory->createInstance(vConfig, pos);
		return pObj;
	}

	//
	//
	//
	virtual void notifyCreateInstance(ClassId nClassId) override
	{
		changeObjCounter(nClassId, true);
	}

	//
	//
	//
	virtual void notifyDestroyInstance(ClassId nClassId) override
	{
		changeObjCounter(nClassId, false);
	}

	//
	//
	//
	virtual Variant getStatInfo() const override
	{
		struct ClassStatInfo
		{
			ClassId nClassId = c_nInvalidClassId;
			Size nCount = 0;

			ClassStatInfo() = default;
			ClassStatInfo(ClassId _nClassId, Size _nCount) : nClassId(_nClassId), nCount(_nCount) {}
		};

		// Avoid long locking of m_mtxFactoryMap
		std::vector<ClassStatInfo> statInfo;
		{
			std::scoped_lock _lock(m_mtxFactoryMap);
			statInfo.reserve(m_FactoryMap.size());

			for (auto&[nClassId, info] : m_FactoryMap)
				statInfo.emplace_back(nClassId, info.nObjCount);
		}

		Sequence vStatInfo;
		Size nClassCount = statInfo.size();
		vStatInfo.setSize(nClassCount);

		for (Size i=0; i< nClassCount; ++i)
		{
			auto& info = statInfo[i];

			vStatInfo.put(i, Dictionary({
				{ "clsid", info.nClassId },
				{ "count", info.nCount },
			}));
		}

		return vStatInfo;
	}

	//
	//
	//
	virtual std::optional<ObjectClassInfo> getClassInfo(ClassId nClassId) const override
	{
		IObjectFactory* pObjectFactory;
		{
			std::scoped_lock _lock(m_mtxFactoryMap);
			FactoryMap::const_iterator Iter = m_FactoryMap.find(nClassId);
			if (Iter == m_FactoryMap.end())
				return std::nullopt;
			pObjectFactory = Iter->second.pFactory;
		}

		return pObjectFactory->getClassInfo();
	}
};


//////////////////////////////////////////////////////////////////////////
//
// CatalogManager
//
//////////////////////////////////////////////////////////////////////////

//
//
//
class CatalogManager : public ICatalogManager
{
private:
	std::recursive_mutex m_mutex;
	Variant m_vCatalog = Dictionary();

	Variant getCatalogSync()
	{
		std::scoped_lock lock(m_mutex);
		Variant vCatalog = m_vCatalog;
		return vCatalog;
	}

public:
	//
	//
	//
	virtual ~CatalogManager() override {}

	//
	//
	//
	virtual Variant getData(std::string_view sPath) override
	{
		TRACE_BEGIN;
		Variant vData = variant::getByPath(getCatalogSync(), sPath);
		return vData;
		//return vData.clone();
		TRACE_END(FMT("Bad catalog item <" << sPath << ">")); 
	}

	//
	//
	//
	/// @copydoc getCatalogData
	virtual std::optional<Variant> getDataSafe(std::string_view sPath) override
	{
		return variant::getByPathSafe(getCatalogSync(), sPath);
	}

	//
	//
	//
	virtual void putData(std::string_view sPath, Variant vData) override
	{
		std::scoped_lock lock(m_mutex);
		if (vData.isRawNull())
			variant::deleteByPath(m_vCatalog, sPath);
		else
			variant::putByPath(m_vCatalog, sPath, vData, true);
	}

	//
	//
	//
	virtual Variant initData(std::string_view sPath, Variant vDefault) override
	{
		std::scoped_lock lock(m_mutex);

		// try to get exist
		auto vData = variant::getByPathSafe(m_vCatalog, sPath);
		if (vData.has_value())
			return vData.value();

		// Path is not exist so add default
		variant::putByPath(m_vCatalog, sPath, vDefault, true);
		return vDefault;
	}

	//
	//
	//
	virtual Variant modifyData(std::string_view sPath, FnCatalogModifier fnModifier) override
	{
		std::scoped_lock lock(m_mutex);

		// get current value
		Variant vData = variant::getByPath(m_vCatalog, sPath, Variant());

		// call fnModifier
		auto vNewData = fnModifier(vData);
		if (!vNewData)
			return vData;

		// put modification
		if (vNewData.value().isRawNull())
			variant::deleteByPath(m_vCatalog, sPath);
		else
			variant::putByPath(m_vCatalog, sPath, vNewData.value(), true);
		return vNewData.value();
	}


	//
	//
	//
	void clear() override
	{
		std::scoped_lock lock(m_mutex);
		m_vCatalog.clear();
	}
};

//////////////////////////////////////////////////////////////////////////
//
// MessageProcessor
//
//////////////////////////////////////////////////////////////////////////

//
// MessageProcessor class
//
class MessageProcessor : public detail::IMessageProcessor
{
private:
	struct Subscriber
	{
		std::string sId;
		ObjPtr<ICommand> pCallback;
	};
	// @podpruzhnikov FIXME: We select data by keys. May usage of multimap is more effective
	typedef std::unordered_multimap<std::string, Subscriber> SubscriberMap;
	typedef std::vector<ObjPtr<ICommand>> SubscriberList;
	typedef std::unordered_map<std::string, bool> LoadMap;

	LoadMap m_mapLoadCfg;
	SubscriberMap m_mapSubscribers; // list of all commands
	std::mutex m_mtxMap;

	//
	// Fill commands by sMessageId
	//
	void collectSubscribers(const std::string& sMessageId, SubscriberList& subscribers)
	{
		TRACE_BEGIN;
		std::scoped_lock _lock(m_mtxMap);
		if (m_mapLoadCfg.find(sMessageId) == m_mapLoadCfg.end())
			loadConfigData(sMessageId);

		auto range = m_mapSubscribers.equal_range(sMessageId);
		for (auto iter = range.first; iter != range.second; ++iter) 
			subscribers.push_back(iter->second.pCallback);
		TRACE_END(FMT("Error during collecting subscribers for a message <" << sMessageId << ">"));
	}

	//
	// Loading data from config
	// NOTE: This method should only be called under the mutex
	//
	void loadConfigData(const std::string& sMessageId)
	{
		// @Bogdanov FIXME: Possible deadlock if sendMessage is called in createCommand  
		std::string sPath("app.config.messageHandlers.");
		sPath += sMessageId;
		Sequence seqHandlers(getCatalogData(sPath, Sequence()));

		for (auto vSubscriber : seqHandlers)
			m_mapSubscribers.insert({ {std::string(sMessageId), {
				vSubscriber.get("subscriptionId", "default_config"), 
				createCommand(vSubscriber) 
			}}});

		m_mapLoadCfg[sMessageId] = true;
	}

public:

	//
	//
	//
	void sendMessage(std::string_view sId, Variant vData) override
	{
		TRACE_BEGIN;
		std::string sMessageId(sId);
		SubscriberList commands;
		{
			collectSubscribers(sMessageId, commands);
			// Add broadcast commands support (when it is needed)
			// collectSubscribers("", commands);
		}

		Variant vParam = Dictionary({ {"messageId", sMessageId} });
		if (!vData.isNull())
			vParam.put("data", vData);

		LOGLVL(Detailed, "Process message <" << sMessageId << ">");

		for (auto& pCmd : commands)
		{
			try
			{
				std::string sInfo = pCmd->getDescriptionString();
				if (sInfo.empty())
					LOGLVL(Detailed, "Call command <" << 
						pCmd->getClassId() << ":" << hex(pCmd->getId()) << ">");
				else
					LOGLVL(Detailed, "Call command <" << sInfo << ">");
				(void)pCmd->execute(vParam);
			}
			catch (error::Exception& ex)
			{
				ex.log(SL, FMT("Can't process message <" << sMessageId << ">"));
			}
		}
		TRACE_END(FMT("Error during processing a message <" << sId << ">"));
	}

	//
	//
	//
	void subscribe(std::string_view sMessageId, ObjPtr<ICommand> pCallback, std::string_view sSubscriptionId) override
	{
		std::scoped_lock _lock(m_mtxMap);
		m_mapSubscribers.insert({ {std::string(sMessageId), {std::string(sSubscriptionId), pCallback } } });
	}

	//
	//
	//
	void unsubscribe(std::string_view sSubscriptionId) override
	{
		std::scoped_lock _lock(m_mtxMap);
		for (auto iter = m_mapSubscribers.begin(); iter != m_mapSubscribers.end();)
		{
			if (iter->second.sId == sSubscriptionId) 
				iter = m_mapSubscribers.erase(iter);
			else
				iter++;
		}
	}

	//
	//
	//
	void unsubscribeAll() override
	{
		std::scoped_lock _lock(m_mtxMap);
		m_mapSubscribers.clear();
	}
};

//////////////////////////////////////////////////////////////////////////
//
// ThreadPoolController
//
//////////////////////////////////////////////////////////////////////////

class ThreadPoolController : public detail::IThreadPoolController
{
	ThreadPool m_timerPool{"Application::DefaultTimerPool"};
	ThreadPool m_procPool{ "Application::DefaultPool" };

public:
	~ThreadPoolController()
	{
		m_timerPool.stop();
		m_procPool.stop();
	}

	//
	//
	//
	virtual ThreadPool& getCoreThreadPool() override
	{
		return m_procPool;
	}

	//
	//
	//
	virtual ThreadPool& getTimerThreadPool() override
	{
		return m_timerPool;
	}

};

//////////////////////////////////////////////////////////////////////////
//
// Core object is global manager aggregator.
// It is singleton.
// 
// It is created to share global managers between modules (DLL, SO).
//
// The most of Core features has global function wrapper (e.g. cmd::createObject). 
// Use them instead raw Core object access.
// Core object pointer should be get with the GetCore() function
//
// Core object is singleton and created at program startup
// It is not universal object
//
//////////////////////////////////////////////////////////////////////////

//
//
//
class ICore
{
protected:
	// Interface can not be destroyed externally
	virtual ~ICore() {}
public:
	virtual	IObjectManager* getObjectManager() = 0;
	virtual	ICatalogManager* getCatalogManager() = 0;
	virtual	detail::IMessageProcessor* getMessageProcessor() = 0;
	virtual detail::IThreadPoolController* getThreadPoolController() = 0;
};

//
// Core object 
//
// Singleton. Manager's aggregator.
// It can be shared between modules (DLL, SO).
//
class Core : public ICore
{
private:
	// For debug usage
	ThreadPoolController* m_pThreadPoolController = nullptr;
	ObjectManager* m_pObjectManager = nullptr;
	CatalogManager* m_pCatalogManager = nullptr;
	MessageProcessor* m_pMessageProcessor = nullptr;

public:

	//
	//
	//
	Core() {}

	//
	//
	//
	virtual ~Core() override {}

	//
	//
	//
	virtual detail::IThreadPoolController* getThreadPoolController() override
	{
		static ThreadPoolController g_ThreadPoolController;
		m_pThreadPoolController = &g_ThreadPoolController;
		return &g_ThreadPoolController;
	}


	//
	//
	//
	virtual	IObjectManager* getObjectManager() override
	{
		static ObjectManager g_ObjectManager;
		m_pObjectManager = &g_ObjectManager;
		return &g_ObjectManager;
	}

	//
	//
	//
	virtual	ICatalogManager* getCatalogManager() override
	{
		static CatalogManager g_CatalogManager;
		m_pCatalogManager = &g_CatalogManager;
		return &g_CatalogManager;
	}

	//
	//
	//
	virtual	detail::IMessageProcessor* getMessageProcessor() override
	{
		static MessageProcessor g_MessageProcessor;
		m_pMessageProcessor = &g_MessageProcessor;
		return &g_MessageProcessor;
	}
};


} // namespace cmd

// Pointer to core for debug usage
static cmd::Core* g_pCore = nullptr;

namespace cmd {

//
// Core creator
// Create a local Core and return it
//
// TODO:
// It creates a local core or requests core from another module.
// Function behavior could be configured for every module (EXE, DLL, SO).
//
ICore* createCore()
{
	static Core g_Core;
	g_pCore = &g_Core;
	return &g_Core;
}

////////////////////////////////////////////////////////////////////////////////
//
// Core managers getters and cachers
// 
////////////////////////////////////////////////////////////////////////////////

//
//
//
ICore& getCore()
{
	static ICore* g_pCache = createCore();
	return *g_pCache;
}

//
//
//
extern IObjectManager& getObjectManager()
{
	static IObjectManager* g_pCache = getCore().getObjectManager();
	return *g_pCache;
}

//
//
//
extern ICatalogManager& getCatalogManager()
{
	static ICatalogManager* g_pCache = getCore().getCatalogManager();
	return *g_pCache;
}

//
//
//
ThreadPool& getCoreThreadPool()
{
	auto& tp = getCore().getThreadPoolController()->getCoreThreadPool();
	if (tp.getThreadsCount() == 0)
		tp.addThreads(1);
	return tp;
}

//
//
//
ThreadPool& getTimerThreadPool()
{
	auto& tp = getCore().getThreadPoolController()->getTimerThreadPool();
	if (tp.getThreadsCount() == 0)
		tp.addThreads(1);
	return tp;
}

//
//
//
void shutdownGlobalPools()
{
	auto& ttp = getCore().getThreadPoolController()->getTimerThreadPool();
	ttp.stop(true);
	auto& ctp = getCore().getThreadPoolController()->getCoreThreadPool();
	ctp.stop(true);
}

namespace detail {

//
//
//
extern IMessageProcessor& getMessageProcessor()
{
	static IMessageProcessor* g_pCache = getCore().getMessageProcessor();
	return *g_pCache;
}

} // namespace detail
} // namespace cmd 




