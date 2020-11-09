//
// edrav2.libcore
// 
// Author: Denis Bogdanov (25.02.2019)
// Reviewer: Yury Podpruzhnikov (27.02.2019)
//
/// @file TheradPool objects declaration
/// \weakgroup threadpool ThreadPool
/// @{
#pragma once
#include "object.hpp"
#include "common.hpp"

namespace openEdr {

class ICommand;
class TimerContext;

///
/// A thread pool class allows to execute functional
/// entities asynchronously using own internal thread pool
/// for execution.
///
class ThreadPool
{
public:

	///
	/// Values that represent thread priorities
	///
	enum class Priority
	{
		High,
		Normal,
		Low
	};

	///
	/// Asychronous handler function for standard run() can get any 
	/// parameters (the std::bind is used) and have no return value.
	///
	typedef std::function<void()> Handler;

	///
	/// Asychronous handler function for runWithDelay() can get any 
	/// parameters (the std::bind is used) and returns boolean value.
	/// True value means that this handler should be rescheduled and
	/// it will be called again after specified timeout.
	///
	typedef std::function<bool()> TimerHandler;

	///
	/// Timer context object is used for controlling delayed asyc calls.
	///
	class ITimerContext
	{
	public:
		virtual ~ITimerContext() {};

		///
		/// Cancels current timer. All planned calls are cancelled despite
		/// of Handler's return value
		///
		/// @return Number of cancelled jobs (in terms of boost::asio)
		///
		virtual Size cancel() = 0;

		///
		/// Reshedules current timer. All current planned calls are cancelled.
		/// Handler will be started according to the new timeout. If the 
		/// c_nMaxSize as a timeout value is specified then the previously
		/// specified timeout will be used.
		///
		/// @param nMsDuration - Wait duration between calls
		/// @param fFirstNow - Specifies that first call shall be as early as possible
		///
		virtual void reschedule(Time nMsDuration = c_nMaxTime, bool fFirstNow = false) = 0;

		///
		/// Returns the flag if current timer is active or stopped
		///
		/// @return True - if the timer is active
		///
		virtual bool isActive() = 0;
	}; 

	typedef std::shared_ptr<ITimerContext> TimerContextPtr;

private:
	class Impl;
	typedef std::shared_ptr<Impl> ImplPtr;
	ImplPtr m_pImpl;

	friend TimerContext;

	void runHandler(Handler fnHandler);
	TimerContextPtr runHandler(Time nDuration, bool fStartNow, TimerHandler fnHandler);

	void checkImplIsValid(SourceLocation sl);

public:
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) noexcept;
	ThreadPool& operator=(ThreadPool&&) noexcept;

	///
	/// Constructor
	///
	/// @param sName Pool name
	/// @param nThreadsCount Number of initial threads (optional)
	/// @param ePriority The priority (optional)
	///
	ThreadPool(const std::string& sName, Size nThreadsCount = 0, Priority ePriority = Priority::Normal);
	
	~ThreadPool();

	///
	/// Starts the given number of threads count
	///
	/// @param nThreadsCount threads count.
	/// @return Number of current threads.
	///
	Size addThreads(Size nThreadsCount);
	
	///
	/// Stops the all threads (waiting all undone calls or not). 
	/// You can't start pool again after this call.
	///
	/// @param fForce - Flag allows to cancel all uncompleted job
	///
	void stop(bool fForce = false);

	///
	/// Delete one execution thread 
	///
	void deleteThread();

	///
	/// Gets current number os execution threads 
	///
	/// @returns The threads count.
	///
	Size getThreadsCount();

	///
	/// Gets the priority
	///
	/// @returns The priority.
	///
	Priority getPriority();

	///
	/// Runs the specified functor asynchronously
	/// If pool has no threads then the new thread will be added.
	///
	/// @tparam Functor Type of the functor.
	/// @tparam Args Type of the arguments.
	/// @param [in] func A functor for async call.
	/// @param args Variable arguments of functor
	///
	template<class Functor, class... Args>
	inline void run(Functor&& func, Args&&... args)
	{
		runHandler(std::bind(func, std::forward<Args>(args)...));
	}
	
	///
	/// Runs the specified functor asynchronously after exceeding of the specified timeout.
	/// If functor returned true then it will be called again in the specified timeout.
	/// If pool has no threads then the new thread will be added.
	///
	/// @tparam Functor - type of the functor.
	/// @tparam Args - type of the arguments.
	/// @param [in] nDuration - timeout for call (in milliseconds)
	/// @param [in] func - a functor for async call.
	/// @param args - variable arguments of functor
	/// @return Pointer to timer context object
	///
	template<class Functor, class... Args>
	[[nodiscard]]
	inline TimerContextPtr runWithDelay(Time nDuration,
		Functor&& func, Args&&... args)
	{
		return runHandler(nDuration, false, std::bind(func, std::forward<Args>(args)...));
	}

	///
	/// Runs the specified functor asynchronously in loop using the specified period.
	/// If functor returned true then it will be called again in the specified timeout.
	/// If pool has no threads then the new thread will be added.
	///
	/// @tparam Functor - type of the functor.
	/// @tparam Args - type of the arguments.
	/// @param [in] nTimeout - timeout for call (in milliseconds)
	/// @param [in] func - a functor for async call.
	/// @param args - variable arguments of functor
	/// @return Pointer to timer context object
	///
	template<class Functor, class... Args>
	[[nodiscard]]
	inline TimerContextPtr runInLoop(Size nTimeout,
		Functor&& func, Args&&... args)
	{
		return runHandler(nTimeout, true, std::bind(func, std::forward<Args>(args)...));
	}

	///
	/// Runs the specified command asynchronously
	/// If pool has no threads then the new thread will be added.
	///
	/// @param [in] pCmd - A command for async call.
	///
	void run(ObjPtr<ICommand> pCmd);

	///
	/// Runs the specified command asynchronously after exceeding of the specified timeout.
	/// If command returned true then it will be called again in the specified timeout.
	/// If pool has no threads then the new thread will be added.
	///
	/// @param [in] tDuration pimeout for call (in milliseconds)
	/// @param [in] pCmd - A command for async call.
	/// @return Pointer to timer context object
	///
	[[nodiscard]]
	TimerContextPtr runWithDelay(Time nDuration, ObjPtr<ICommand> pCmd);
};

//
//
//
class ITaskContext : OBJECT_INTERFACE
{
public:

	///
	/// Cancel the task
	///
	virtual Size cancel() = 0;
	
	/// 
	/// Reschedule the task
	///
	/// @param nMsDuration - Wait duration between calls
	/// @param fFirstNow - Specifies that first call shall be as early as possible
	///
	virtual void reschedule(Time nMsDuration = c_nMaxTime, bool fFirstNow = false) = 0;
	
	///
	/// Checks if the task is active now
	///
	/// @return true if the task is active
	///
	virtual bool isActive() = 0;
};

//
//
//
[[nodiscard]]
ObjPtr<ITaskContext> convertToTask(ThreadPool::TimerContextPtr pTimerContext);

} // namespace openEdr
/// @}
