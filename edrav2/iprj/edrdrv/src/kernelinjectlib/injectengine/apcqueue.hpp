//
// edrav2.edrdrv project
//
// Author: Denis Shavrovskiy (15.03.2021)
// Reviewer:
//
///
/// @addtogroup kernelinjectlib
/// @{
#pragma once
#include "injector.hpp"

typedef
VOID (NTAPI* PKNORMAL_ROUTINE)(
	__in PVOID NormalContext,
	__in PVOID SystemArgument1,
	__in PVOID SystemArgument2
	);

namespace cmd {
namespace kernelInjectLib {
namespace apcQueue {

typedef 
VOID (*PAPC_CALLBACK)(
	__in PVOID Context
	);

struct ApcContext
{
	PAPC_CALLBACK Callback;
	PVOID Context;
};

class ApcQueue
{
	UniquePtr<KAPC> m_apc;
	UniquePtr<ApcContext> m_context;

public:
	ApcQueue() = default;
	ApcQueue(ApcQueue& src) = delete;
	ApcQueue(ApcQueue&& src) = delete;

	void initialize(const PRKTHREAD Thread, const ApcContext& Context, const KPROCESSOR_MODE PreviousMode = UserMode);
	void initialize(const PRKTHREAD Thread, PAPC_CALLBACK CallabckRoutine, PVOID CallbackContext);
	bool insert(PVOID SystemArgument1 = nullptr, PVOID SystemArgument2 = nullptr);

private:
	static void NTAPI NormalRoutine(
		__in PVOID NormalContext,
		__in PVOID SystemArgument1,
		__in PVOID SystemArgument2
	);

	static void NTAPI RundownRoutine(
		__in PRKAPC Apc
	);

	static void NTAPI KernelRoutine(
		__in PRKAPC Apc,
		__inout PKNORMAL_ROUTINE* NormalRoutine,
		__inout PVOID* NormalContext,
		__inout PVOID* SystemArgument1,
		__inout PVOID* SystemArgument2
	);
};

} // namespace apcQueue
} // namespace kernelInjectLib
} // namespace cmd

/// @}
