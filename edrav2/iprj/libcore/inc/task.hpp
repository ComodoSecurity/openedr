//
// edrav2.libcore project
//
// Task management functions
//
// Reviewer: Denis Bogdanov (11.03.2019) 
// Reviewer: 
//
#pragma once
#include "object.hpp"
#include "threadpool.hpp"

namespace openEdr {


//
//
//
class TaskCatalogGuard
{
	Size m_nHash = 0;
	std::string m_sPath;

public:

	//
	//
	//
	TaskCatalogGuard(std::string_view sName) : m_sPath("tasks."),
		m_nHash(std::hash<std::thread::id>{}(std::this_thread::get_id()))
	{
		m_sPath += sName;
	}

	//
	//
	//
	~TaskCatalogGuard()
	{
		unlock();
	}

	//
	//
	//
	bool lock()
	{
		auto vData = initCatalogData(m_sPath, m_nHash);
		return (vData.getType() == variant::ValueType::Integer && vData == m_nHash);
	}

	//
	//
	//
	Variant put(Variant vData)
	{
		putCatalogData(m_sPath, vData);
		m_nHash = 0;
		return vData;
	}

	//
	//
	//
	void del(Variant oldVal)
	{
		// @podpruzhnikov FIXME: Are you sure that this code deletes data?
		(void) modifyCatalogData(m_sPath, [&oldVal](Variant currVal) -> std::optional<Variant>
		{
			if (currVal == oldVal) return nullptr;
			return std::nullopt;
		});
	}

	//
	//
	//
	Variant get()
	{
		while (true)
		{
			auto vData = getCatalogData(m_sPath, Variant());
			if (vData.isNull()) return vData;
			if (vData.getType() != variant::ValueType::Integer) return vData;
			if (vData == m_nHash) return vData;
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

	//
	//
	//
	void unlock()
	{
		if (m_nHash == 0) return;
		auto vData = getCatalogData(m_sPath, Variant());
		if (vData.getType() != variant::ValueType::Integer || vData != m_nHash) return;
		del(vData);
	}
};

///
/// Get task by its name
///
/// @param [in] sName - Task name
/// @param [in] fNoThrow - Shows if the exception shall be thrown if the task is not found
/// @return Pointer to the task context or nullptr if the task is not found and fNoThrow is set
/// @throw error::NotFound if the task is not found and fNoThrow is not set
/// @throw error::TypeError if the task descriptor in catalog is invalid
///
[[nodiscard]]
inline ObjPtr<ITaskContext> getTask(std::string_view sName, bool fNoThrow = false)
{
	TaskCatalogGuard guard(sName);

	auto vTaskContext = guard.get();
	if (vTaskContext.isNull())
	{
		if (fNoThrow) return nullptr;
		error::NotFound(FMT("Task <" << sName << "> is not found")).throwException();
	}

	// If the task is exist, return it
	auto pTaskContext = queryInterfaceSafe<ITaskContext>(vTaskContext);
	if (pTaskContext)
		return pTaskContext;

	error::TypeError(FMT("Invalid task descriptor <" << sName << "> ")).throwException();
}

///
/// Runs the specified task by name. The task should be created beforehand
///
/// @param [in] sName - Task name
/// @return Pointer to the task context
/// @throw error::NotFound if the task is not found
///
[[nodiscard]]
inline ObjPtr<ITaskContext> startTask(std::string_view sName)
{
	auto pTaskCtx = getTask(sName);
	if (!pTaskCtx->isActive())
		pTaskCtx->reschedule(c_nMaxTime, true);
	return pTaskCtx;
}

///
/// Stops the specified task and deletes it from tasks catalog section if it specified.
///
/// @param [in] sName - Task name
/// @param [in] fDelete - If specified then the task is deleted from catalog
/// @return false if specified task is not found
///
inline bool stopTask(std::string_view sName, bool fDelete = true)
{
	auto pTaskCtx = getTask(sName, true);
	if (pTaskCtx == nullptr) return false;
	pTaskCtx->cancel();
	if (!fDelete) return true;
	TaskCatalogGuard guard(sName);
	guard.del(pTaskCtx);
	return true;
}

///
/// Runs the specified functor asynchronously as a named task. It use system working 
/// thread pool after exceeding of the specified timeout. The functor is executed 
/// in the worker thread pool for avoiding influence to main thread timer.
///
/// @param [in] sName - Task name
/// @param [in] nTimeout - Timeout for call (in milliseconds)
/// @param [in] func - A functor for async call.
/// @param [in] Args - Functor's parameters
/// @return Pointer to timer context object
///
template<class Functor, class... Args>
[[nodiscard]]
inline ObjPtr<ITaskContext> startTask(std::string_view sName,
	Size nTimeout, Functor&& func, Args&&... args)
{
	TaskCatalogGuard guard(sName);
	if (!guard.lock())
		error::AlreadyExists(FMT("Task <" << sName << "> is already exists")).throwException();

	// Start in other thread
	return guard.put(convertToTask(getTimerThreadPool().
		runWithDelay(nTimeout, [sName, func, args...]() -> bool
		{
			getCoreThreadPool().run([sName, func, args...]()
			{
				if (func(std::forward<Args>(args)...)) return;
				stopTask(sName, false);
			});
			return true;
		})));
}

///
/// Runs the specified command asynchronously using system working thread pool after 
/// exceeding of the specified timeout. The functor is executed in the worker 
/// thread pool for avoiding influence to main thread timer.
///
/// @param [in] sName - Task name
/// @param [in] nTimeout - Timeout for call (in milliseconds)
/// @param [in] pCmd - A command for async call.
/// @return Pointer to timer context object
///
[[nodiscard]]
inline ObjPtr<ITaskContext> startTask(std::string_view sName, Size nTimeout,
	ObjPtr<ICommand> pCmd)
{
	TaskCatalogGuard guard(sName);
	if (!guard.lock())
		error::AlreadyExists(FMT("Task <" << sName << "> is already exists")).throwException();

	return guard.put(convertToTask(getTimerThreadPool().
		runWithDelay(nTimeout, [sName, pCmd]() -> bool
	{
		getCoreThreadPool().run([sName, pCmd]()
		{
			if (pCmd->execute()) return;
			stopTask(sName, false);
		});
		return true;
	})));
}

} // namespace openEdr
/// @}
