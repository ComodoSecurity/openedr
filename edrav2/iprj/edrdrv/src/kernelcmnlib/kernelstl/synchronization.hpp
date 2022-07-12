//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (15.06.2019)
// Reviewer:
//
///
/// @file Synchronization
///
/// @addtogroup kernelCmnLib
/// @{
#pragma once

namespace cmd {

//
// std::scoped_lock analog
//
template<typename Mutex>
class ScopedLock
{
	Mutex& m_mtx;
public:
	_Requires_lock_not_held_(m_mtx)
	_Acquires_lock_(m_mtx)
	_IRQL_saves_global_(OldIrql, m_mtx)
	#pragma warning(suppress : 28167)
	ScopedLock(Mutex& mtx) :m_mtx(mtx)
	{
		m_mtx.lock();
	}

	_Requires_lock_held_(m_mtx)
	_Releases_lock_(m_mtx)
	_IRQL_restores_global_(OldIrql, m_mtx)
	~ScopedLock()
	{
		m_mtx.unlock();
	}
};


//
// std::unique_lock analog
//
template<typename Mutex>
class UniqueLock
{
	Mutex& m_mtx;
	bool m_fOwnLock = false;
public:
	UniqueLock(Mutex& mtx) :m_mtx(mtx)
	{
		lock();
	}

	~UniqueLock()
	{
		unlock();
	}

	void lock()
	{
		m_mtx.lock();
		m_fOwnLock = true;
	}

	bool tryLock()
	{
		if (m_fOwnLock)
			return true;
		m_fOwnLock = m_mtx.tryLock();
		return m_fOwnLock;
	}

	void unlock()
	{
		m_mtx.unlock();
		m_fOwnLock = false;
	}

	bool hasLock() const
	{
		return m_fOwnLock;
	}

	operator bool() const
	{
		return hasLock();
	}
};



//
// std::shared_lock analog
//
template<typename Mutex>
class SharedLock
{
	Mutex& m_mtx;
	bool m_fOwnLock = false;
public:
	SharedLock(Mutex& mtx) :m_mtx(mtx)
	{
		lock();
	}

	~SharedLock()
	{
		unlock();
	}

	void lock()
	{
		m_mtx.lockShared();
		m_fOwnLock = true;
	}

	bool tryLock()
	{
		if (m_fOwnLock)
			return true;
		m_fOwnLock = m_mtx.tryLock();
		return m_fOwnLock;
	}

	void unlock()
	{
		m_mtx.unlockShared();
		m_fOwnLock = false;
	}

	bool hasLock() const
	{
		return m_fOwnLock;
	}

	operator bool() const
	{
		return hasLock();
	}
};


//
// FAST_MUTEX wrapper
//
class FastMutex
{
private:
	FAST_MUTEX m_fastMutex = {};
public:
	FastMutex()
	{
		ExInitializeFastMutex(&m_fastMutex);
	}

	_IRQL_requires_max_(APC_LEVEL)
	_IRQL_raises_(APC_LEVEL)
	_IRQL_saves_global_(OldIrql, m_fastMutex)
	void lock()
	{
		ExAcquireFastMutex(&m_fastMutex);
	}

	_IRQL_requires_max_(APC_LEVEL)
	_IRQL_requires_(APC_LEVEL)
	_IRQL_restores_global_(OldIrql, m_fastMutex)
	void unlock()
	{
		ExReleaseFastMutex(&m_fastMutex);
	}
};

//
// ERESOURCE wrapper
//
class EResource
{
private:
	BOOLEAN m_fInitialized = FALSE;
	ERESOURCE m_resource = {};
public:
	EResource() = default;

	//
	//
	//
	~EResource()
	{
		if (!m_fInitialized)
			return;
		ExDeleteResourceLite(&m_resource);
		m_fInitialized = FALSE;
	}

	//
	//
	//
	[[nodiscard]] NTSTATUS initialize()
	{
		if (m_fInitialized)
			return STATUS_SUCCESS;
		const NTSTATUS eStatus = ExInitializeResourceLite(&m_resource);
		if (NT_SUCCESS(eStatus))
			m_fInitialized = TRUE;
		return eStatus;
	}

	_IRQL_requires_max_(APC_LEVEL)
	void lock()
	{
		if (!m_fInitialized)
			return;

		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&m_resource, TRUE);
	}

	_IRQL_requires_max_(APC_LEVEL)
	bool tryLock()
	{
		if (!m_fInitialized)
			return false;

		KeEnterCriticalRegion();
		const bool fLocked = ExAcquireResourceExclusiveLite(&m_resource, FALSE);
		if (!fLocked)
			KeLeaveCriticalRegion();
		return fLocked;
	}

	_IRQL_requires_max_(APC_LEVEL)
	void unlock()
	{
		if (!m_fInitialized)
			return;
		ExReleaseResourceLite(&m_resource);
		KeLeaveCriticalRegion();
	}

	_IRQL_requires_max_(APC_LEVEL)
	void lockShared()
	{
		if (!m_fInitialized)
			return;
		KeEnterCriticalRegion();
		ExAcquireResourceSharedLite(&m_resource, TRUE);
	}

	_IRQL_requires_max_(APC_LEVEL)
	bool tryLockShared()
	{
		if (!m_fInitialized)
			return false;
		KeEnterCriticalRegion();
		const bool fLocked = ExAcquireResourceSharedLite(&m_resource, FALSE);
		if(!fLocked)
			KeLeaveCriticalRegion();
		return fLocked;
	}

	_IRQL_requires_max_(APC_LEVEL)
	void unlockShared()
	{
		if (!m_fInitialized)
			return;
		ExReleaseResourceLite(&m_resource);
		KeLeaveCriticalRegion();
	}
};

//
// KMUTEX wrapper
//
class KMutex
{
private:
	alignas(ULONG_PTR) KMUTEX m_mutex;
public:
	KMutex()
	{
		KeInitializeMutex(&m_mutex, 0);
	}

	_IRQL_requires_max_(APC_LEVEL)
	void lock()
	{
		KeWaitForSingleObject(&m_mutex, Executive, KernelMode, FALSE, nullptr);
	}

	void unlock()
	{
		KeReleaseMutex(&m_mutex, FALSE);
	}
};

//////////////////////////////////////////////////////////////////////////
//
// KSPIN_LOCK
//

//
// KSPIN_LOCK wrapper (in queued mode)
//
class KStackQueuedSpinLock
{
private:
	KSPIN_LOCK m_spinlock;
public:
	KStackQueuedSpinLock()
	{
		KeInitializeSpinLock(&m_spinlock);
	}

	_Requires_lock_not_held_(*pLockHandle)
	_Acquires_lock_(*pLockHandle)
	_IRQL_saves_global_(QueuedSpinLock, pLockHandle)
	_IRQL_requires_max_(DISPATCH_LEVEL)
	_IRQL_raises_(DISPATCH_LEVEL)
	void lock(PKLOCK_QUEUE_HANDLE pLockHandle)
	{
		KeAcquireInStackQueuedSpinLock(&m_spinlock, pLockHandle);
	}

	_Requires_lock_held_(*pLockHandle)
	_Releases_lock_(*pLockHandle)
	_IRQL_restores_global_(QueuedSpinLock, pLockHandle)
	_IRQL_requires_(DISPATCH_LEVEL)
	void unlock(PKLOCK_QUEUE_HANDLE pLockHandle)
	{
		KeReleaseInStackQueuedSpinLock(pLockHandle);
	}

	_Requires_lock_held_(*pLockHandle)
	_Releases_lock_(*pLockHandle)
	_IRQL_restores_global_(QueuedSpinLock, pLockHandle)
	_IRQL_requires_(DISPATCH_LEVEL)
	static void unlockStatic(PKLOCK_QUEUE_HANDLE pLockHandle)
	{
		KeReleaseInStackQueuedSpinLock(pLockHandle);
	}
};

//
// ScopedLock specialization for KStackQueuedSpinLock
//
template<>
class ScopedLock<KStackQueuedSpinLock>
{
	KLOCK_QUEUE_HANDLE lockHandle;
public:
	_IRQL_saves_global_(QueuedSpinLock, lockHandle)
	_IRQL_requires_max_(DISPATCH_LEVEL)
	_IRQL_raises_(DISPATCH_LEVEL)
	ScopedLock(KStackQueuedSpinLock& spinlock)
	{
		spinlock.lock(&lockHandle);
	}

	_IRQL_restores_global_(QueuedSpinLock, lockHandle)
	_IRQL_requires_(DISPATCH_LEVEL)
	~ScopedLock()
	{
		KStackQueuedSpinLock::unlockStatic(&lockHandle);
	}
};

//////////////////////////////////////////////////////////////////////////
//
// RWSpinLock
//

//
// EX_SPIN_LOCK wrapper (read/write spinlock)
//
class RWSpinLock
{
private:
	EX_SPIN_LOCK m_spinlock;
public:
	RWSpinLock(): m_spinlock(0)
	{
	}

	_Acquires_exclusive_lock_(*this)
	_Requires_lock_not_held_(*this)
	_IRQL_saves_global_(SpinLock, m_spinlock)
	_IRQL_raises_(DISPATCH_LEVEL)
	[[nodiscard]] KIRQL lock()
	{
		return ExAcquireSpinLockExclusive(&m_spinlock);
	}

	_Releases_exclusive_lock_(*this)
	_Requires_lock_held_(*this)
	_IRQL_restores_global_(SpinLock, m_spinlock)
	_IRQL_requires_(DISPATCH_LEVEL)
	void unlock(KIRQL eOldIrql)
	{
		ExReleaseSpinLockExclusive(&m_spinlock, eOldIrql);
	}

	_Acquires_shared_lock_(*this)
	_Requires_lock_not_held_(*this)
	_IRQL_saves_global_(SpinLock, m_spinlock)
	_IRQL_raises_(DISPATCH_LEVEL)
	[[nodiscard]] KIRQL lockShared()
	{
		return ExAcquireSpinLockShared(&m_spinlock);
	}

	_Releases_shared_lock_(*this)
	_Requires_lock_held_(*this)
	_IRQL_restores_global_(SpinLock, m_spinlock)
	_IRQL_requires_(DISPATCH_LEVEL)
	void unlockShared(KIRQL eOldIrql)
	{
		ExReleaseSpinLockShared(&m_spinlock, eOldIrql);
	}
};

//
// UniqueLock specialization for RWSpinLock
//
template<>
class UniqueLock<RWSpinLock>
{
	RWSpinLock& m_spinlock;
	KIRQL m_eOldIrql;
public:

	_Acquires_exclusive_lock_(m_spinlock)
	_Requires_lock_not_held_(m_spinlock)
	_IRQL_saves_global_(SpinLock, m_eOldIrql)
	_IRQL_raises_(DISPATCH_LEVEL)
	UniqueLock(RWSpinLock& spinlock) :m_spinlock(spinlock)
	{
		m_eOldIrql = spinlock.lock();
	}

	_Releases_exclusive_lock_(m_spinlock)
	_Requires_lock_held_(m_spinlock)
	_IRQL_restores_global_(SpinLock, m_eOldIrql)
	_IRQL_requires_(DISPATCH_LEVEL)
	~UniqueLock()
	{
		m_spinlock.unlock(m_eOldIrql);
	}
};

//
// SharedLock specialization for RWSpinLock
//
template<>
class SharedLock<RWSpinLock>
{
	RWSpinLock& m_spinlock;
	KIRQL m_eOldIrql;
public:

	_Acquires_shared_lock_(m_spinlock)
	_Requires_lock_not_held_(m_spinlock)
	_IRQL_saves_global_(SpinLock, m_eOldIrql)
	_IRQL_raises_(DISPATCH_LEVEL)
	SharedLock(RWSpinLock& spinlock) :m_spinlock(spinlock)
	{
		m_eOldIrql = spinlock.lockShared();
	}

	_Releases_shared_lock_(m_spinlock)
	_Requires_lock_held_(m_spinlock)
	_IRQL_restores_global_(SpinLock, m_eOldIrql)
	_IRQL_requires_(DISPATCH_LEVEL)
	~SharedLock()
	{
		m_spinlock.unlockShared(m_eOldIrql);
	}
};

} // namespace cmd

/// @}
