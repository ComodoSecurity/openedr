//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _CTRLIO_H
#define _CTRLIO_H

#include "nfdriver.h"
#include "hashtable.h"

#if _RELEASE_LOG
#define DbgStatus(_x_) ctrl_queuePush _x_
#else
#define DbgStatus(_x_)
#endif

NTSTATUS ctrl_queuePush(int code, HASH_ID id);
NTSTATUS ctrl_queuePushEx(int code, HASH_ID id, char * buf, int len);

NTSTATUS ctrl_create(PIRP irp, PIO_STACK_LOCATION irpSp);
NTSTATUS ctrl_close(PIRP irp, PIO_STACK_LOCATION irpSp);
NTSTATUS ctrl_read(PIRP irp, PIO_STACK_LOCATION irpSp);
NTSTATUS ctrl_write(PIRP irp, PIO_STACK_LOCATION irpSp);

BOOLEAN ctrl_attached();

void ctrl_deferredCompleteIrp(PIRP irp, BOOLEAN longDelay);

void ctrl_completeIoDpc(
    IN PKDPC  Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    );

void ctrl_defferedCompletePendedIoRequest();

void ctrl_completePendedIoRequestDpc(
    IN PKDPC  Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    );

NTSTATUS   
ctrl_getAddrInformation(PNF_DATA pData);

NTSTATUS ctrl_open(PIRP irp, PIO_STACK_LOCATION irpSp);

#endif