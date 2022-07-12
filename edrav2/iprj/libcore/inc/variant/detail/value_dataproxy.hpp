//
// edrav2.libcore
// DataProxy implementation (Lazy object calculation)
// 
// Autor: Yury Podpruzhnikov (19.02.2019)
// Reviewer: Denis Bogdanov (25.02.2019)
//
#pragma once
#include "value.hpp"

namespace cmd {
namespace variant {
namespace detail {

//
// Interface of all DataProxy
//
class IDataProxy
{
public:
	// Each DataProxy must have virtual destructor
	virtual ~IDataProxy() {}

	//
	// Calculates and returns proxied data
	//
	virtual Variant getData() const = 0;

	//
	// Creates a clone of DataProxy
	//
	virtual Variant clone() const = 0;

	//
	// print to string (for logging)
	//
	virtual std::string print() const = 0;
};

//
// DataProxy with Cache.
// Calculates proxied data only once.
//
// Supports lazy cloning. 
// If data is not calculated, it creates CachedDataProxy clone.
// This clone calculates and clones data after first access.
//
class CachedDataProxy : public IDataProxy
{
private:
	// deny coping and moving
	CachedDataProxy(const CachedDataProxy&) = delete;
	CachedDataProxy(CachedDataProxy&&) = delete;
	CachedDataProxy& operator=(const CachedDataProxy&) = delete;
	CachedDataProxy& operator=(CachedDataProxy&&) = delete;

	//
	// Shared data for each clone.
	// CachedDataProxy is duplicated on each cloning. But SharedData is still common.
	// SharedData contains fnCalculate and its result because fnCalculate must be called once.
	//
	struct SharedData
	{
		DataProxyCalculator fnCalculate;
		Variant vCache; //< Cached data
		bool fCalculated = false; //< vCache is filled. Data is calculated.
		std::mutex m_mtxCalculation; //< CachedDataProxy synchronization (common for every clones)
	};
	std::shared_ptr<SharedData> m_pSharedData; 	//< Shared data for every clone

	//
	// This clone info
	//

	bool m_fNeedCloneData = false; //< This CachedDataProxy is a clone. Needs to clone ProxiedData after calculation.
	mutable Variant m_vCache; //< This clone data
	mutable bool m_fCalculated = false; //< vCache is filled. Data is calculated.

	// Private parameter of constructor
	struct PrivateKey {};

public:
	// "private" constructor supporting std::make_shared 
	// Create may use std::make_unique, but constructor can't be called external
	CachedDataProxy(const PrivateKey&)
	{
	}

	//
	// Creator
	// @param fnCalculate Info to calculate proxied data
	// @return shared_ptr<IDataProxy>
	//
	static Variant create(const DataProxyCalculator& fnCalculate)
	{
		auto pThis = std::make_shared<CachedDataProxy>(PrivateKey());
		pThis->m_pSharedData = std::make_shared<SharedData>();
		pThis->m_pSharedData->fnCalculate = fnCalculate;
		return createVariant(std::shared_ptr<IDataProxy>(pThis));
	}

	//
	// Returns proxied data. Calculates and clones it if necessary.
	//
	Variant getData() const override
	{
		std::scoped_lock lock(m_pSharedData->m_mtxCalculation);
		// This clone has data
		if (m_fCalculated)
			return m_vCache;

		// Request data from SharedData
		if (!m_pSharedData->fCalculated)
		{
			if (!m_pSharedData->fnCalculate)
				error::LogicError(SL, "Invalid dataproxy state.").throwException();
			m_pSharedData->vCache = m_pSharedData->fnCalculate();
			m_pSharedData->fCalculated = true;
			// Clear calculator to release data
			m_pSharedData->fnCalculate = nullptr;
		}
		Variant vResult = m_pSharedData->vCache;

		// Force resolving
		variant::detail::resolveDataProxy(vResult, false);

		// Cloning data if it is needed
		m_vCache = m_fNeedCloneData ? vResult.clone() : vResult;
		m_fCalculated = true;
		return m_vCache;
	}

	//
	// If data is calculated, returns its clone.
	// Otherwise returns a clone of CachedDataProxy.
	//
	Variant clone() const override
	{
		std::scoped_lock lock(m_pSharedData->m_mtxCalculation);
		// If data is already calculated, returns its clone.
		if (m_pSharedData->fCalculated)
			return m_pSharedData->vCache.clone();
	
		// Creates lazy clone
		auto pClone = std::make_shared<CachedDataProxy>(PrivateKey());
		pClone->m_pSharedData = m_pSharedData;
		pClone->m_fNeedCloneData = true;
		return createVariant(std::shared_ptr<IDataProxy>(pClone));
	}

	//
	//
	//
	std::string print() const override
	{
		return "DataProxy<cached=true>";
	}
};

//
// DataProxy without Cache.
// Calculates data every access.
//
class NonCachedDataProxy : public IDataProxy, 
	public std::enable_shared_from_this<NonCachedDataProxy>
{
private:
	// deny coping and moving
	NonCachedDataProxy(const NonCachedDataProxy&) = delete;
	NonCachedDataProxy(NonCachedDataProxy&&) = delete;
	NonCachedDataProxy& operator=(const NonCachedDataProxy&) = delete;
	NonCachedDataProxy& operator=(NonCachedDataProxy&&) = delete;

	DataProxyCalculator m_fnCalculate;

	// Private parameter of constructor
	struct PrivateKey {};

public:
	// "private" constructor supporting std::make_shared 
	// Create may use std::make_unique, but constructor can't be called external
	NonCachedDataProxy(const PrivateKey&)
	{
	}

	//
	// Creator
	// @param fnCalculate Info to calculate proxied data
	// @return shared_ptr<IDataProxy>
	//
	static Variant create(const DataProxyCalculator& fnCalculate)
	{
		auto pThis = std::make_shared<NonCachedDataProxy>(PrivateKey());
		pThis->m_fnCalculate = fnCalculate;
		return createVariant(std::shared_ptr<IDataProxy>(pThis));
	}

	//
	// Returns proxied data. Calculates and clones it if necessary.
	//
	virtual Variant getData() const override
	{
		return m_fnCalculate();
	}

	//
	// If data is calculated, returns its clone.
	// Otherwise returns a clone of CachedDataProxy.
	//
	virtual Variant clone() const override
	{
		return createVariant(std::shared_ptr<IDataProxy>(const_cast<NonCachedDataProxy*>(this)->shared_from_this()));
	}

	//
	//
	//
	std::string print() const override
	{
		return "DataProxy<cached=false>";
	}
};

} // namespace detail
} // namespace variant
} // namespace cmd
