//
// edrav2.edrpm project
//
// Author: Yury Podpruzhnikov (26.04.2019)
// Reviewer: Denis Bogdanov (05.05.2019)
//
// Basis of trampolines
//
#pragma once
#include "hookapi.h"

namespace cmd {
namespace edrpm {

//////////////////////////////////////////////////////////////////////////
//
// TrampManager
//

// Unique id of hook
typedef uint64_t TrampId;


//
// Action with tramps
//
enum class TrampAction
{
	Start,  //< Set hook
	Stop,   //< Reset hook
	DisableCallback,  //< Disable call callbacks
	EnableCallback, //< Enable call callbacks
};

//
// Trampoline interface for TrampManager
//
class ITramp
{
protected:
	~ITramp() {}

	//
	// Registration in TrampManager
	//
	void addIntoTrampManager(TrampId nTrampId);

public:

	//
	// Apply tramp action
	//
	virtual void controlHook(TrampAction) = 0;
};

//
// CallbackRegistrator interface for TrampManager
//
class ICallbackRegistrator
{
protected:
	
	//
	//
	//
	~ICallbackRegistrator() {}

	//
	// auto registration
	//
	ICallbackRegistrator();

public:

	//
	// Registers callback
	//
	virtual void registerCallbacks() = 0;
};

//
// Global trampoline manager
// It is accessed via getTrampManager()
//
class TrampManager
{
private:
	struct TrampInfo
	{
		ITramp* pTramp = nullptr; //< Trampoline
		bool fAllowStart = true; //< The field for ability to switch off hook installation
	};

	// Trampoline list
	std::map<TrampId, TrampInfo> m_tramps;

	// Callbacks list
	std::vector<ICallbackRegistrator*> m_callbackRegistrators;

	std::atomic_bool m_fCallbackRegistered = false;
	std::atomic_bool m_fStarted = false;

	// NOTE: m_tramps and m_callbackRegistrators should be filled in main thread only.
	// Later they should be constant.

protected:

	friend ICallbackRegistrator;
	friend ITramp;

	//
	// Tramp auto registration API
	//
	void addTramp(TrampId nId, ITramp* pTramp)
	{
		// NOTE: It should be used in main thread only.
		m_tramps[nId] = TrampInfo{ pTramp, true };
	}

	//
	// CallbackRegistrator auto registration API
	//
	void addCallbackRegistrator(ICallbackRegistrator* pCallbackRegistrator)
	{
		// NOTE: It should be used in main thread only.
		m_callbackRegistrators.push_back(pCallbackRegistrator);
	}

public:

	//
	// Hooks all tramps
	//
	void startTramps()
	{
		if (m_fStarted)
			return;

		if (!m_fCallbackRegistered)
		{
			// Add callbacks into trampolines
			for (auto& pCallbackRegistrator : m_callbackRegistrators)
			{
				pCallbackRegistrator->registerCallbacks();
			}
			m_fCallbackRegistered = true;
		}

		// Start trampolines
		for (auto& [id, info] : m_tramps)
		{
			if (info.fAllowStart)
			{
				info.pTramp->controlHook(TrampAction::Start);
			}
		}

		m_fStarted = true;
	}

	//
	// Disables starting of trampoline.
	// It does not work if trampoline is already started.
	//
	void disableTrampStart(TrampId nTrampId)
	{
		auto iter = m_tramps.find(nTrampId);
		if (iter == m_tramps.end())
			return;
		iter->second.fAllowStart = false;
	}

	//
	// Unhooks all trams
	//
	void stopTramps()
	{
		if (!m_fStarted)
			return;

		// Start trampolines
		for (auto& [id, info] : m_tramps)
		{
			info.pTramp->controlHook(TrampAction::Stop);
		}

		m_fStarted = false;
	}

	//
	// Enable all trams
	//
	void enableTramps()
	{
		// Start trampolines
		for (auto& [id, info] : m_tramps)
		{
			info.pTramp->controlHook(TrampAction::EnableCallback);
		}
	}

	//
	// Disable all trams
	//
	void disableTramps()
	{
		// Start trampolines
		for (auto& [id, info] : m_tramps)
		{
			info.pTramp->controlHook(TrampAction::DisableCallback);
		}
	}
};

//
// Returns TrampManager
//
TrampManager& getTrampManager();


//////////////////////////////////////////////////////////////////////////
//
// Tramp basis
//

bool isTrampolineAlreadyCalled();
void markTrampolineAsCalled(bool val);

inline TrampId getTrampId(const char* sDll, const char* sName)
{
	xxh::hash_state64_t hash;
	hash.update(std::string(sDll));
	hash.update(std::string(sName));
	return (TrampId)hash.digest();
}

//
// Tramp Base class.
//
// It contains all features and data.
// All data should be static!
//
template<typename TrampTraits>
class TrampBase : protected ITramp
{
private:

	//
	// Helper for
	// - declaration of callback types and 
	// - definition hook function
	//
	template<typename T>
	struct _Helper;

	template<typename R, typename... Args>
	struct _Helper<R __stdcall (Args...)>
	{
		//
		// preCallback type
		//
		// It has same arguments as hooked function
		// and returns void.
		//
		using FnPreCallback = void(Args...);

		//
		// postCallback type
		//
		// First argument is returned value, 
		// others are same arguments as hooked function.
		// Returns void.
		//
		// If return type is void first argument is absent
		//
		static constexpr bool c_fReturnVoid = std::is_void_v<R>;
		using ResultType = std::conditional_t <c_fReturnVoid, DWORD, R>;
		using FnPostCallback = std::conditional_t < c_fReturnVoid, 
			void(Args...), void(ResultType, Args...)>;

		//
		// Hooking function.
		//
		// This function should be registered for hooking.
		//
		static R __stdcall hookFunction(Args... args)
		{
			if (g_data.fnRaw == nullptr)
			{
				UserBreak(TrampTraits::getFnName());

				if constexpr (c_fReturnVoid)
					return;
				else
					return ResultType{};
			}

			// Save current error state
			auto nError = ::GetLastError();

			// Check second entrance it hook
			bool fIsDoubleEnter = isTrampolineAlreadyCalled();

			if (!fIsDoubleEnter)
				markTrampolineAsCalled(true);

			// Reads the flag once
			// If preCallback is called, then postCallback is called too
			bool fEnableCallback = g_data.fEnableCallback && !fIsDoubleEnter;

			// Calls preCallback
			if (fEnableCallback && !g_data.preCallbacks.empty())
			{
				for (auto fnCallback : g_data.preCallbacks)
				{
					try
					{
						fnCallback(args...);
					}
					catch (const std::exception& ex)
					{
						logError(std::string("Fail to call preCallback for <") +
							TrampTraits::getDllName() + "!" + TrampTraits::getFnName() +
							">, error <" + ex.what() + ">");
					}
					catch (...)
					{
						logError(std::string("Fail to call preCallback for <") +
							TrampTraits::getDllName() + "!" + TrampTraits::getFnName() + ">");
					}
				}
			}

			// Restore error before original function call
			::SetLastError(nError);

			// Calls original function
			ResultType res = ResultType{};
			if constexpr (c_fReturnVoid)
				g_data.fnRaw(args...);
			else
				res = g_data.fnRaw(args...);

			// Save result of original function
			nError = ::GetLastError();

			// Calls postCallback
			if (fEnableCallback && !g_data.postCallbacks.empty())
			{
				for (auto fnCallback : g_data.postCallbacks)
				{
					try
					{
						if constexpr (c_fReturnVoid)
							fnCallback(args...);
						else
							fnCallback(res, args...);
					}
					catch (const std::exception& ex)
					{
						logError(std::string("Fail to call postCallback for <") +
							TrampTraits::getDllName() + "!" + TrampTraits::getFnName() +
							">, error <" + ex.what() + ">");
					}
					catch (...)
					{
						logError(std::string("Fail to call postCallback for <") +
							TrampTraits::getDllName() + "!" + TrampTraits::getFnName() + ">");
					}
				}
			}

			if (!fIsDoubleEnter)
				markTrampolineAsCalled(false);

			// Restore error before exit
			::SetLastError(nError);

			// Returns result
			if constexpr (c_fReturnVoid)
				return;
			else
				return res;
		}
	};

	typedef _Helper<typename TrampTraits::Fn> Helper;

	// Checking that hookFunction and original function have the same types.
	static_assert(std::is_same_v<decltype(&Helper::hookFunction), typename TrampTraits::Fn*>,
		"Hook declaration is invalid. Hooking function has another type than hooked function.");

public:

	// Hooked function type
	typedef typename TrampTraits::Fn* FnPtr;
	// preCallback function type
	typedef typename Helper::FnPreCallback* FnPreCallbackPtr;
	// postCallback function type
	typedef typename Helper::FnPostCallback* FnPostCallbackPtr;

private:

	//
	// Data of trampoline
	// All data should be static for access from hooking function
	// All data is grouped into one struct for move-convenient initialization.
	//
	struct StaticData
	{
		// Pointer to original function
		FnPtr fnRaw = nullptr;

		// Flag is that callback calling is enabled
		// If it is false, just original function will be called
		std::atomic_bool fEnableCallback = true;

		// Flag is that hook was set
		// This is disable add callback
		std::atomic_bool fStarted = false;

		// preCallback list
		std::vector<FnPreCallbackPtr> preCallbacks;

		// postCallback list
		std::vector<FnPostCallbackPtr> postCallbacks;
	};

	static StaticData g_data;

	//
	// Utility function for call GetProcAddress
	//
	static FnPtr getProcAddress()
	{
		HMODULE hModule = LoadLibraryA(TrampTraits::getDllName());
		if (hModule == nullptr)
			return nullptr;

		auto pFn = GetProcAddress(hModule, TrampTraits::getFnName());
		return (FnPtr)pFn;
	}

	//
	// Calculates id of trampoline
	// Returns hash of DLL and function name.
	//
	static TrampId getId()
	{
		return getTrampId(TrampTraits::getDllName(), TrampTraits::getFnName());
	}

protected:

	//
	// Controlling trampoline from TrampManager
	//
	void controlHook(TrampAction eAction) override
	{
		switch (eAction)
		{
			case TrampAction::Start:
			{
				if (g_data.fStarted)
					return;

				// Don't hook if no callbacks
				if (g_data.postCallbacks.empty() && g_data.preCallbacks.empty())
					return;

				// For convenient debug
				const char* sDllName = TrampTraits::getDllName();
				const char* sFnName = TrampTraits::getFnName();
				DWORD nFlags = TrampTraits::c_eHookFlags;
#if defined(FEATURE_ENABLE_MADCHOOK)
				g_data.fStarted = HookAPI(sDllName, sFnName, &Helper::hookFunction, (LPVOID*)&g_data.fnRaw, nFlags);
				if (g_data.fStarted)
				{
					dbgPrint(std::string("Hook function <") + sDllName + ":" + sFnName + ">");
				}
				else
				{
					auto sErr = std::string("Can't hook function <") + sDllName + ":" + sFnName + ">";
					auto nErr = ::GetLastError();
					if (nErr == DoubleHookError)
						sErr += ", error <Already hooked>";
					else if (nErr == CodeNotInterceptableError)
						sErr += ", error <Code not intrceptable>";
					else if (nErr == MixtureModeError)
						sErr += ", error <Mixture more fail>";
					else if (nErr == WinsockModeError)
						sErr += ", error <Winsock not support mixture mode>";
					else if (nErr == MixtureDisabledError)
						sErr += ", error <Mixture mode disabled>";
					logError(sErr);
				}
#else
				g_data.fStarted = detours::HookAPI(sDllName, sFnName, &Helper::hookFunction, (LPVOID*)&g_data.fnRaw);
				if (g_data.fStarted)
				{
					dbgPrint(std::string("Hook function <") + sDllName + ":" + sFnName + ">");
				}
				else
				{
					auto sErr = std::string("Can't hook function <") + sDllName + ":" + sFnName + ">";
					auto nErr = ::GetLastError();
					if (ERROR_INVALID_BLOCK == nErr)
						sErr += ", error <The function referenced is too small to be detoured>";
					else if (ERROR_INVALID_HANDLE == nErr)
						sErr += ", error <The Code ppPointer parameter is null or points to a null pointer>";
					else if (ERROR_INVALID_OPERATION == nErr)
						sErr += ", error <No pending transaction exists>";
					else if (ERROR_NOT_ENOUGH_MEMORY == nErr)
						sErr += ", error <Not enough memory exists to complete the operation>";

					logError(sErr);
				}
#endif // FEATURE_ENABLE_MADCHOOK

				return;
			}
			case TrampAction::Stop:
			{
				if (!g_data.fStarted)
					return;
				if (g_data.fnRaw == nullptr)
					return;

				// For convenient debug
				const char* sDllName = TrampTraits::getDllName();
				const char* sFnName = TrampTraits::getFnName();
#if defined(FEATURE_ENABLE_MADCHOOK)
				if (IsHookInstalled((LPVOID*)&g_data.fnRaw))
				{
					if (UnhookAPI((LPVOID*)&g_data.fnRaw))
						dbgPrint(std::string("Unhook function <") + sDllName + ":" + sFnName + ">");
					else
						logError(std::string("Can't unhook function <") + sDllName + ":" + sFnName +
							">, in use < " + std::to_string(IsHookInUse((LPVOID*)&g_data.fnRaw)) + ">");
				}
#else
				if (detours::UnhookAPI((LPVOID*)&g_data.fnRaw, &Helper::hookFunction))
					dbgPrint(std::string("Unhook function <") + sDllName + ":" + sFnName + ">");
				else
					logError(std::string("Can't unhook function <") + sDllName + ":" + sFnName + ">");
#endif
				return;
			}
			case TrampAction::DisableCallback:
			{
				g_data.fEnableCallback = false;
				return;
			}
			case TrampAction::EnableCallback:
			{
				g_data.fEnableCallback = true;
				return;
			}
		}
	}
public:

	TrampBase()
	{
		addIntoTrampManager(getId());
	}

	//
	// Adds preCallback 
	//
	void addPreCallback(FnPreCallbackPtr fn)
	{
		if (g_data.fStarted)
			return;
		g_data.preCallbacks.push_back(fn);
	}

	//
	// Adds postCallback 
	//
	void addPostCallback(FnPostCallbackPtr fn)
	{
		if (g_data.fStarted)
			return;
		g_data.postCallbacks.push_back(fn);
	}
};

//
// Definition of TrampBase static data 
//
template<typename TrampTraits>
typename TrampBase<TrampTraits>::StaticData TrampBase<TrampTraits>::g_data;

#if !defined(FEATURE_ENABLE_MADCHOOK) 
///
/// This flags doesn't used in detours
/// 
#define NO_SAFE_UNHOOKING	0
#define FOLLOW_JMP			0
#define SAFE_HOOKING		0
#endif

///
/// Name Of trampoline for fast declaration
/// @param FnId - Hooked function declaration
///
#define _TRAMP_NAME(FnId) CONCAT_ID(g_trampolineOf_, FnId)

///
/// Trampoline definition
/// @param sDllName - Name of DLL (ansi)
/// @param FnId - Hooked function declaration
///
/// Example:
/// DECLARE_TRAMP("user32.dll", MessageBoxA);
///
#define DECLARE_TRAMP(sDllName, FnId) _DECLARE_TRAMP(sDllName, FnId, FnId, SAFE_HOOKING)

//
// Trampoline with flags
//
#define DECLARE_TRAMP_FLAGS(sDllName, FnId, eHookFlags) \
	_DECLARE_TRAMP(sDllName, FnId, FnId, SAFE_HOOKING | eHookFlags)

//
// Trampoline with TrampolineId.
// It is used if need to hook several function with same FnId from different DLL
//
#define DECLARE_TRAMP_ALIAS(sDllName, FnId, TrampolineId) \
	_DECLARE_TRAMP(sDllName, FnId, TrampolineId, SAFE_HOOKING)

#define _DECLARE_TRAMP(sDllName, FnId, TrampolineId, eHookFlags) \
	_DECLARE_TRAMP2(CONCAT_ID(CONCAT_ID(HookTraits_, TrampolineId), __LINE__), sDllName, FnId, TrampolineId, eHookFlags)

#define _DECLARE_TRAMP2(TraitsName, sDllName, FnId, TrampolineId, eHookFlags) \
	_DECLARE_TRAMP3(TraitsName, sDllName, FnId, TrampolineId, eHookFlags)
#define _DECLARE_TRAMP3(TraitsName, sDllName, FnId, TrampolineId, eHookFlags) \
	struct TraitsName { \
		static const char* getDllName() { return sDllName; } \
		static const char* getFnName() { return #FnId; } \
		static constexpr uint32_t c_eHookFlags = eHookFlags; \
		typedef decltype(FnId) Fn; \
	}; \
	EXTERN_IN_DECLARE_TRAMP TrampBase<TraitsName> _TRAMP_NAME(TrampolineId);

//
// Macros for switch declaration to definition 
// Default value is declaration (adding "extern")
// Redefine with empty value for definition
//
#define EXTERN_IN_DECLARE_TRAMP extern


//////////////////////////////////////////////////////////////////////////
//
// CallbackRegistrator basis
//

///
/// PreCallback registration
/// @param FnId - Hooked function declaration
/// @param fnCallback - pointer to callback function
///
/// Example:
/// REGISTER_PRE_CALLBACK(MessageBoxA, callback);
///
#define REGISTER_PRE_CALLBACK_IF(TrampolineId, fCondition, fnCallback) \
class CONCAT_ID(CONCAT_ID(PreCallbackRegistrator_, TrampolineId), __LINE__) : public ICallbackRegistrator { \
	void registerCallbacks() override { if (fCondition) _TRAMP_NAME(TrampolineId).addPreCallback(fnCallback); } \
} CONCAT_ID(CONCAT_ID(g_preCallbackRegistrator_, TrampolineId), __LINE__);

#define REGISTER_PRE_CALLBACK(TrampolineId, fnCallback) REGISTER_PRE_CALLBACK_IF(TrampolineId, true, fnCallback)

///
/// PostCallback registration
/// @param TrampolineId - usually same as FnId - Hooked function declaration
/// @param fnCallback - pointer to callback function
///
/// Example:
/// REGISTER_POST_CALLBACK(MessageBoxA, callback);
///
#define REGISTER_POST_CALLBACK_IF(TrampolineId, fCondition, fnCallback) \
class CONCAT_ID(CONCAT_ID(PreCallbackRegistrator_, TrampolineId), __LINE__) : public ICallbackRegistrator { \
	void registerCallbacks() override { if (fCondition) _TRAMP_NAME(TrampolineId).addPostCallback(fnCallback); } \
} CONCAT_ID(CONCAT_ID(g_preCallbackRegistrator_, TrampolineId), __LINE__);

#define REGISTER_POST_CALLBACK(TrampolineId, fnCallback) REGISTER_POST_CALLBACK_IF(TrampolineId, true, fnCallback)

} // namespace edrpm
} // namespace cmd

