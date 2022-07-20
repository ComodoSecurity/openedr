//
// edrav2.edrdrv project
//
// Author: Denis Shavrovskiy (15.03.2021)
// Reviewer:
//
///
/// @addtogroup kernelinjectlib
/// @{

#include "apcqueue.hpp"

typedef enum _KAPC_ENVIRONMENT
{
	OriginalApcEnvironment,
	AttachedApcEnvironment,
	CurrentApcEnvironment,
	InsertApcEnvironment
} KAPC_ENVIRONMENT, * PKAPC_ENVIRONMENT;

typedef VOID KKERNEL_ROUTINE(
	__in PRKAPC Apc,
	__inout PKNORMAL_ROUTINE* NormalRoutine,
	__inout PVOID* NormalContext,
	__inout PVOID* SystemArgument1,
	__inout PVOID* SystemArgument2
);

typedef KKERNEL_ROUTINE (NTAPI* PKKERNEL_ROUTINE);

typedef VOID (NTAPI* PKRUNDOWN_ROUTINE)(
	__in PRKAPC Apc
	);

extern "C"
NTKERNELAPI
VOID
NTAPI
KeInitializeApc(
	__out PRKAPC Apc,
	__in PRKTHREAD Thread,
	__in KAPC_ENVIRONMENT Environment,
	__in PKKERNEL_ROUTINE KernelRoutine,
	__in_opt PKRUNDOWN_ROUTINE RundownRoutine,
	__in_opt PKNORMAL_ROUTINE NormalRoutine,
	__in_opt KPROCESSOR_MODE ProcessorMode,
	__in_opt PVOID NormalContext
);

extern "C"
NTKERNELAPI
BOOLEAN
NTAPI
KeInsertQueueApc(
	__in PRKAPC Apc,
	__in_opt PVOID SystemArgument1,
	__in_opt PVOID SystemArgument2,
	__in KPRIORITY Increment
);

namespace cmd {
namespace kernelInjectLib {
namespace apcQueue {

void  ApcQueue::initialize(const PRKTHREAD Thread, PAPC_CALLBACK CallabckRoutine, PVOID CallbackContext)
{
	m_apc = new (NonPagedPool) KAPC();
	if (!m_apc)
		return;

	KeInitializeApc(m_apc.get(), Thread, OriginalApcEnvironment, KernelRoutine, RundownRoutine, (PKNORMAL_ROUTINE)CallabckRoutine, UserMode, CallbackContext);
}

void ApcQueue::initialize(const PRKTHREAD Thread, const ApcContext& Context, const KPROCESSOR_MODE PreviousMode)
{
	m_apc = new (NonPagedPool) KAPC();
	m_context = new (NonPagedPool) ApcContext();
	if (!m_apc || !m_context)
		return;

	memcpy(m_context.get(), &Context, sizeof(Context));
	KeInitializeApc(m_apc.get(), Thread, OriginalApcEnvironment, KernelRoutine, RundownRoutine, NormalRoutine, PreviousMode, m_context.get());
}

bool ApcQueue::insert(PVOID SystemArgument1, PVOID SystemArgument2)
{
	if (!m_apc)
		return false;

	if (KeInsertQueueApc(m_apc.get(), SystemArgument1, SystemArgument2, IO_NO_INCREMENT))
	{
		m_apc.release();
		m_context.release();
		return true;
	}
	return false;
}

void ApcQueue::NormalRoutine(PVOID NormalContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
	UniquePtr<ApcContext> context = static_cast<ApcContext*>(NormalContext);

	UNREFERENCED_PARAMETER(SystemArgument2);
	UNREFERENCED_PARAMETER(SystemArgument1);

	context->Callback(context->Context);
}

void ApcQueue::RundownRoutine(PRKAPC Apc)
{
	UNREFERENCED_PARAMETER(Apc);

	LOGERROR(STATUS_SUCCESS, "ApcQueue::RundownRoutine\r\n");
}

void ApcQueue::KernelRoutine(PRKAPC Apc, PKNORMAL_ROUTINE* NormalRoutine, PVOID* NormalContext, PVOID* SystemArgument1, PVOID* SystemArgument2)
{
	UniquePtr<KAPC> apc = Apc;

	UNREFERENCED_PARAMETER(NormalContext);
	UNREFERENCED_PARAMETER(NormalRoutine);
	UNREFERENCED_PARAMETER(SystemArgument1);
	UNREFERENCED_PARAMETER(SystemArgument2);
}

} // namespace apcQueue
} // namespace kernelInjectLib
} // namespace cmd

/// @}