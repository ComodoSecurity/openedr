//
// edrav2.libcloud project
//
// Author: Denis Kroshin (25.08.2019)
// Reviewer: Denis Bogdanov (02.09.2019)
//
///
/// @file Context service implementation
///
/// @addtogroup edr
/// @{
#include "pch.h"
#include "signcontext.h"

namespace cmd {

#undef CMD_COMPONENT
#define CMD_COMPONENT "ctxsvc"

//
//
//
void ContextService::Basket::init(Variant vParams)
{
	maxSize = vParams.get("maxSize", c_nBasketMaxSize);
	wrnSize = vParams.get("wrnSize", maxSize / 5 * 4);
	purgeTimeout = vParams.get("purgeTimeout", c_nPurgeTimeout);
}

//
//
//
void ContextService::Basket::reset()
{
	std::scoped_lock _lock(mtxData);
	mapKeys.clear();
	size = 0;
	expireTimeSum = 0;
	purgeGroup.clear();
	lastPurgeTime = getCurrentTime();
	if (fFastPurge && timerPurge)
		timerPurge->reschedule(purgeTimeout);
	fFastPurge = false;
	fForcePurge = false;
}

//
//
//
bool ContextService::Basket::has(const Hash nKeyId)
{
	std::scoped_lock _lock(mtxData);
	auto itList = mapKeys.find(nKeyId);
	return (itList != mapKeys.end() && !itList->second.empty());
}

//
//
//
Variant ContextService::Basket::load(const Hash nKeyId, const bool fRemove)
{
	std::scoped_lock _lock(mtxData);
	auto itList = mapKeys.find(nKeyId);
	if (itList == mapKeys.end() || itList->second.empty())
		return {};

	Sequence clData;
	Time expTimeSum = 0;
	for (auto itCtx : itList->second)
	{
		clData.push_back(itCtx.data);
		expTimeSum += itCtx.expireTime;
	}

	if (fRemove)
	{
		size -= itList->second.size();
		expireTimeSum -= expTimeSum;
		mapKeys.erase(itList);
	}
	return clData;
}

//
//
//
void ContextService::Basket::save(const Hash nKeyId, const Context& ctx, const Size nLimit)
{
	std::scoped_lock _lock(mtxData);
	auto& ctxList = mapKeys[nKeyId];
	// TODO: Switch from list to something with quick size() or count it manualy
	if (ctxList.size() >= nLimit)
	{
		auto expireTime = ctxList.front().expireTime;
		ctxList.pop_front();
		--size;
		expireTimeSum -= expireTime;
	}

	ctxList.push_back(ctx);
	++size;
	expireTimeSum += ctx.expireTime;

	// Schedule purge if basket rich limit
	if (size >= maxSize && !fForcePurge && timerPurge)
	{
		auto nCurrentTime = getCurrentTime();
		auto nPurgeTime = lastPurgeTime + c_nMinPurgeDelta;
		if (nCurrentTime < nPurgeTime)
			// Reschedule to minimal delta from last purge
			timerPurge->reschedule(nPurgeTime - nCurrentTime);
		else
			// Reschedule for "just now"
			timerPurge->reschedule(10, true);
		fForcePurge = true;
		LOGLVL(Detailed, "Basket <" << name << "> is full, purge process is rescheduled");
	}
}

//
//
//
void ContextService::Basket::removeKey(const Hash nKeyId)
{
	std::scoped_lock _lock(mtxData);
	auto itKey = mapKeys.find(nKeyId);
	if (itKey == mapKeys.end())
		return;

	for (auto itCtx : itKey->second)
	{
		--size;
		expireTimeSum -= itCtx.expireTime;
	}

	mapKeys.erase(itKey);
	return;
}

//
//
//
void ContextService::Basket::removeGroup(const Hash nGroupId, const bool fForce)
{
	std::scoped_lock _lock(mtxData);
	if (fForce)
	{
		for (auto itKey = mapKeys.begin(); itKey != mapKeys.end();)
		{
			auto& ctxList = itKey->second;
			for (auto itCtx = ctxList.begin(); itCtx != ctxList.end();)
				if (itCtx->group == nGroupId)
				{
					--size;
					expireTimeSum -= itCtx->expireTime;
					itCtx = ctxList.erase(itCtx);
				}
				else
					++itCtx;

			if (ctxList.empty())
				itKey = mapKeys.erase(itKey);
			else
				++itKey;
		}
	}
	else
		// Mark group as "deleted"
		purgeGroup.insert(nGroupId);
}

//
//
//
Time ContextService::Basket::purgeRound(const Time expireTime)
{
	Size nCount = 0;
	Time minExpireTime = c_nMaxTime;
	Time maxExpireTime = 0;
	const auto fCheckGroups = !purgeGroup.empty();

	for (auto itList = mapKeys.begin(); itList != mapKeys.end();)
	{
		auto& ctxList = itList->second;
		for (auto itCtx = ctxList.begin(); itCtx != ctxList.end();)
		{
			if (expireTime >= itCtx->expireTime ||
				(fCheckGroups && itCtx->group != c_nEmptyHash &&
					purgeGroup.find(itCtx->group) != purgeGroup.end()))
			{
				--size;
				expireTimeSum -= itCtx->expireTime;
				itCtx = ctxList.erase(itCtx);
			}
			else
			{
				if (minExpireTime > itCtx->expireTime)
					minExpireTime = itCtx->expireTime;
				if (maxExpireTime < itCtx->expireTime)
					maxExpireTime = itCtx->expireTime;

				++itCtx;
				++nCount;
			}
		}

		if (ctxList.empty())
			itList = mapKeys.erase(itList);
		else
			++itList;
	}
	purgeGroup.clear();

	if (nCount != size)
		error::LogicError(SL, FMT("Mismatched basket size, real <" << nCount << ">, counter <" << size << ">")).log();

	return minExpireTime;
}

//
//
//
bool ContextService::Basket::purge()
{
	std::scoped_lock _lock(mtxData);
	LOGLVL(Debug, "Purge started <ContextService>. Basket <"<< name <<">, size <" << size << 
		">, fast <" << fFastPurge << ">, force <" << fForcePurge << ">");

	auto nCurrentTime = getCurrentTime();
	Time nNearestExpireTime = (size >= maxSize) ?
		// Aggressive purge if we reached limit
		purgeRound(expireTimeSum / size) :
		// Normal purge
		purgeRound(nCurrentTime);

	if (timerPurge) // Can be NULL in tests
	{
		if (size >= wrnSize)
		{
			// TODO: Can we implement more smart logic?
			timerPurge->reschedule(nNearestExpireTime - nCurrentTime + c_nMinPurgeDelta);
			fFastPurge = true;
		}
		else if (fFastPurge || fForcePurge)
		{
			timerPurge->reschedule(purgeTimeout);
			fFastPurge = false;
		}
	}
	fForcePurge = false;
	lastPurgeTime = getCurrentTime();
	LOGLVL(Debug, "Purge finished <ContextService>. Basket <" << name << ">, size <" << size << 
		">, fast <" << fFastPurge << ">, force <" << fForcePurge << ">");
	return true;
}

//
//
//
void ContextService::Basket::schedule(ThreadPool* pThreadPool)
{
	if (timerPurge)
		timerPurge->reschedule();
	else
	{
		auto weakThis = weak_from_this();
		timerPurge = pThreadPool->runWithDelay(purgeTimeout, [weakThis]()
		{
			auto pThis = weakThis.lock();
			if (pThis == nullptr)
				return false;
			pThis->purge();
			return true;
		});
	}
}

//
//
//
void ContextService::Basket::cancel()
{
	if (timerPurge)
		timerPurge->cancel();
}



Variant ContextService::Basket::getInfo()
{
	return Dictionary({ 
		{"size", size},
		{"keys", mapKeys.size()},
	});
}

//
//
//
void ContextService::finalConstruct(Variant vConfig)
{
	m_vConfig = vConfig.get("baskets", {});
}

//
//
//
ContextService::Hash ContextService::getKeyId(Variant vParams)
{
	TRACE_BEGIN
	return crypt::getHash<Hasher>(vParams, crypt::VariantHash::AsEvent);
	TRACE_END("Fail to generate id")
}

//
//
//
Size ContextService::createBasket(const std::string sName)
{
	auto id = m_pStorage.size();
	auto pBasket = std::make_shared<Basket>();
	pBasket->name = sName;
	// Initialize basket from config
	pBasket->init(m_vConfig.get(sName, {}));
	// EDR-2877: schedule purge on creation if service run
	if (m_fInitialized)
		pBasket->schedule(&m_threadPool);
	m_pStorage.push_back(pBasket);
	m_pNames[sName] = id;
	return id;
}

//
//
//
Size ContextService::getBasketId(const std::string sName)
{
	std::scoped_lock _lock(m_mtxData);
	auto itName = m_pNames.find(sName);
	if (itName != m_pNames.end())
		return itName->second;
	return createBasket(sName);
}

//
//
//
ContextService::BasketPtr ContextService::getBasket(Variant vParams, bool fCreate)
{
	if (vParams.has("id"))
	{
		Size nBasketId = vParams["id"];

		std::scoped_lock _lock(m_mtxData);
		if (nBasketId < m_pStorage.size())
			return m_pStorage[nBasketId];
		return nullptr;
	}

	if (vParams.has("name"))
	{
		std::string sName = vParams["name"];

		std::scoped_lock _lock(m_mtxData);
		auto itName = m_pNames.find(sName);
		if (itName != m_pNames.end())
			return m_pStorage[itName->second];

		if (fCreate)
			return m_pStorage[createBasket(sName)];
		return nullptr;
	}

	error::InvalidArgument(SL, "Basket 'name' or 'id' must be specified.").throwException();
}

//
//
//
bool ContextService::has(Variant vParams)
{
	auto pBasket = getBasket(vParams);
	if (pBasket == nullptr)
		return false;
	return pBasket->has(getKeyId(vParams["key"]));
}

//
//
//
Variant ContextService::load(Variant vParams)
{
	auto pBasket = getBasket(vParams);
	if (pBasket == nullptr)
		return {};

	return pBasket->load(getKeyId(vParams["key"]), vParams.get("delete", false));
}

//
//
//
void ContextService::save(Variant vParams)
{
	auto pBasket = getBasket(vParams, true);
	if (pBasket == nullptr)
		return;

	Size nLimit = vParams.get("limit", 1);
	if (nLimit == 0)
		error::InvalidArgument(SL, "Limit can't be zero").throwException();

	Context ctx = {
		vParams.get("data", {}),
		getCurrentTime() + Time(vParams["timeout"]),
		vParams.has("group") ? getKeyId(vParams["group"]) : Hash()
	};
	pBasket->save(getKeyId(vParams["key"]), ctx, nLimit);
}

//
//
//
void ContextService::remove(Variant vParams)
{
	auto pBasket = getBasket(vParams);
	if (pBasket == nullptr)
		error::NotFound(SL, "Basket not found").throwException();

	if (vParams.has("key"))
		pBasket->removeKey(getKeyId(vParams["key"]));
	else if (vParams.has("group"))
		pBasket->removeGroup(getKeyId(vParams["group"]), vParams.get("force", false));
	else
		// we never remove basket itself, we just clear data
		pBasket->reset();
}

//
//
//
void ContextService::purge(Variant vParams)
{
	auto pBasket = getBasket(vParams);
	if (pBasket == nullptr)
		error::InvalidArgument(SL, "Basket not found").throwException();

	pBasket->purge();
}

//
//
//
void ContextService::loadState(Variant vState)
{

}

//
//
//
variant::Variant ContextService::saveState()
{
	return {};
}

//
//
//
void ContextService::start()
{
	TRACE_BEGIN;
	LOGLVL(Detailed, "Context Service is being started");

	std::scoped_lock _lock(m_mtxStartStop);
	if (m_fInitialized)
	{
		LOGINF("Context Service is already started");
		return;
	}

	{
		std::scoped_lock _lock2(m_mtxData);
		if (m_threadPool.getThreadsCount() == 0)
			m_threadPool.addThreads(1);

		for (auto itBasket : m_pStorage)
			itBasket->schedule(&m_threadPool);
	}

	m_fInitialized = true;

	LOGLVL(Detailed, "Context Service is started");
	TRACE_END("Fail to start Context Service");
}

//
//
//
void ContextService::stop()
{
	TRACE_BEGIN;
	LOGLVL(Detailed, "Context Service is being stopped");

	std::scoped_lock _lock(m_mtxStartStop);
	if (!m_fInitialized)
		return;

	{
		std::scoped_lock _lock2(m_mtxData);
		for (auto itBasket : m_pStorage)
			itBasket->cancel();
	}

	m_fInitialized = false;
	LOGLVL(Detailed, "Context Service is stopped");
	TRACE_END("Fail to stop Context Service");
}

//
//
//
void ContextService::shutdown()
{
	LOGLVL(Detailed, "Context Service is being shutdowned");

	std::scoped_lock _lock(m_mtxData);
	m_threadPool.stop(true);

	LOGLVL(Detailed, "Context Service is shutdowned");
}

//
//
//
Variant ContextService::getStatistic()
{
	Sequence sqBaskets;
	{
		std::scoped_lock _lock(m_mtxData);
		for (auto itLink : m_pNames)
		{
			auto pBasket = m_pStorage[itLink.second];
			auto vBasket = pBasket->getInfo();
			vBasket.put("name", itLink.first);
			sqBaskets.push_back(vBasket);
		}
	}
	return Dictionary({ {"baskets", sqBaskets} });
}

//
//
//
Variant ContextService::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;

	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant ContextService::execute()
	///
	/// ##### getBasketId()
	/// Returns a basket identifier for the specified basket name.
	///   * name [string] - basket name.
	///
	if (vCommand == "getBasketId")
		return getBasketId(vParams["name"]);

	///
	/// @fn Variant ContextService::execute()
	///
	/// ##### has()
	/// Checks if the basket has a context.
	///   * id [int] - basket identifier.
	///   * name [string] - basket name,
	///   * key [var] - Variant with data for key calculation.
	///
	if (vCommand == "has")
		return has(vParams);

	///
	/// @fn Variant ContextService::execute()
	///
	/// ##### save()
	/// Save the data to the context.
	///   * id [int] - basket identifier.
	///   * name [string] - basket name.
	///   * key [var] - Variant with data for key calculation.
	///   * data [string] - blob of data to save in context.
	///   * timeout [int] - life time of context in ms.
	///   * group [var, opt] - Variant with data for context group calculation.
	///   * limit [int] - maximan number of contexts with the same key.
	///
	if (vCommand == "save")
	{
		save(vParams);
		return {};
	}

	///
	/// @fn Variant ContextService::execute()
	///
	/// ##### load()
	/// Loads the data from the context.
	///   * id [int] - basket identifier.
	///   * name [string] - basket name.
	///   * key [var] - Variant with data for key calculation.
	///   * delete [bool, opt] - remove data after load.
	///
	if (vCommand == "load")
		return load(vParams);

	///
	/// @fn Variant ContextService::execute()
	///
	/// ##### remove()
	/// Removes the data from the context.
	///   * id [int] - basket identifier.
	///   * name [string] - basket name.
	///   * key [var] - Variant with data for key calculation.
	///   * group [var] - Variant with data for context group calculation.
	///   * force [bool, opt] - flag for group removal.
	///
	if (vCommand == "remove")
	{
		remove(vParams);
		return {};
	}

	///
	/// @fn Variant ContextService::execute()
	///
	/// ##### purge()
	/// Removes data from the context.
	///   * id [int] - basket identifier.
	///   * name [string] - basket name.
	///
	if (vCommand == "purge")
	{
		purge(vParams);
		return {};
	}

	///
	/// @fn Variant ContextService::execute()
	///
	/// ##### getStatistic()
	/// Returns the storage statistics.
	///
	if (vCommand == "getStatistic")
		return getStatistic();

	///
	/// @fn Variant ContextService::execute()
	///
	/// ##### start()
	/// Starts the service.
	///
	if (vCommand == "start")
	{
		start();
		return {};
	}

	///
	/// @fn Variant ContextService::execute()
	///
	/// ##### stop()
	/// Stops the service.
	///
	if (vCommand == "stop")
	{
		stop();
		return {};
	}

	error::OperationNotSupported(SL,
		FMT("Context Service doesn't support command <" << vCommand << ">")).throwException();
	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
}

} // namespace cmd 

/// @} 
