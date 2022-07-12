//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Driver logging
///
/// @addtogroup edrdrv
/// @{
#pragma once
#include <stdarg.h>
#include "common.h"
#include "workerthread.h"
#include "log.h"

namespace cmd {
namespace log {

///
/// The structure for global data of logging subsystem.
///
struct SLogData
{
	bool m_fIsInitialized; ///< Logging was initialized

	//
	// Circle buffer
	//

	char* pBuffer; ///< Circle buffer
	size_t nBufferSize; ///< Circle buffer size
	FAST_MUTEX mtxBuffer; ///< Circle buffer guard

	// if nStartOffset == nEndOffset then BufferIsEmpty 
	size_t nStartOffset; ///< Circle buffer data start
	size_t nEndOffset; ///< Circle buffer data end

	//
	// File
	//
	FAST_MUTEX mtxFile; ///< Log file guard
	HANDLE hFile; ///< Log file handle

};

static SLogData g_LogData = {0};


//////////////////////////////////////////////////////////////////////////
//
// Worker interface
//
DoWorkResult writeLogDataToFile(void* /*pContext*/);
SystemThreadWorker g_Worker;

//
//
//
void initialize()
{
	if (g_LogData.m_fIsInitialized) return;

	RtlZeroMemory(&g_Worker, sizeof(g_Worker));

	ExInitializeFastMutex(&g_LogData.mtxBuffer);
	ExInitializeFastMutex(&g_LogData.mtxFile);

	g_LogData.nBufferSize = c_nLogFileBufferSize;
	g_LogData.pBuffer = (char*) ExAllocatePoolWithTag(NonPagedPool, 
		g_LogData.nBufferSize, c_nAllocTag);
	if (g_LogData.pBuffer == NULL) return;

	if (!g_Worker.initialize(&writeLogDataToFile, nullptr, 0)) return;
	
	g_LogData.m_fIsInitialized = TRUE;
}

//
//
//
void finalize()
{
	bool fWasInitialized = g_LogData.m_fIsInitialized;
	g_LogData.m_fIsInitialized = false;

	g_Worker.finalize();

	// Free buffer
	{
		if (fWasInitialized)
			ExAcquireFastMutex(&g_LogData.mtxBuffer);

		auto pBuffer = g_LogData.pBuffer;
		g_LogData.pBuffer = NULL;

		if (fWasInitialized)
			ExReleaseFastMutex(&g_LogData.mtxBuffer);

		if (pBuffer != NULL)
			ExFreePoolWithTag(pBuffer, c_nAllocTag);
	}

	// Free file
	if (fWasInitialized)
	{
		ExAcquireFastMutex(&g_LogData.mtxFile);
		HANDLE hFile = g_LogData.hFile;
		g_LogData.hFile = NULL;
		ExReleaseFastMutex(&g_LogData.mtxFile);
		if (hFile != NULL)
			ZwClose(hFile);
	}
}

//
//
//
NTSTATUS setLogFile(const WCHAR* sFileName)
{
	UNICODE_STRING usFileName;
	RtlInitUnicodeString(&usFileName, sFileName);
	return setLogFile(usFileName);
}


//
//
//
NTSTATUS setLogFile(UNICODE_STRING usFileName)
{
	if (!g_LogData.m_fIsInitialized) return LOGERROR(STATUS_UNSUCCESSFUL);

	HANDLE hFile = NULL;
	__try
	{
		OBJECT_ATTRIBUTES Attributes;
		InitializeObjectAttributes(&Attributes, &usFileName,
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

		IO_STATUS_BLOCK IoStatus = {};		
		IFERR_RET(ZwCreateFile(&hFile, GENERIC_WRITE | SYNCHRONIZE, &Attributes, 
			&IoStatus, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OVERWRITE_IF, 
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0));

		{
			NTSTATUS ns = STATUS_ALREADY_REGISTERED;

			ExAcquireFastMutex(&g_LogData.mtxFile);
			if (g_LogData.hFile == NULL)
			{
				g_LogData.hFile = hFile;
				hFile = NULL;
				ns = STATUS_SUCCESS;
			}
			ExReleaseFastMutex(&g_LogData.mtxFile);

			IFERR_RET(ns);
		}
	}
	__finally
	{
		if (hFile != NULL)
			ZwClose(hFile);
	}

	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
// Worker interface
//

struct LogData
{
	char* pData = NULL;
	size_t nDataSize = 0;
};

//
// Getting a next block from buffer to output into file
//
LogData getNextData()
{
	// Get data block
	__try
	{
		LogData data;
		ExAcquireFastMutex(&g_LogData.mtxBuffer);
		if (g_LogData.nStartOffset == g_LogData.nEndOffset) return data;

		data.pData = g_LogData.pBuffer + g_LogData.nStartOffset;
		if (g_LogData.nStartOffset > g_LogData.nEndOffset)
		{
			data.nDataSize = g_LogData.nBufferSize - g_LogData.nStartOffset;
			g_LogData.nStartOffset = 0;
		}
		else
		{
			data.nDataSize = g_LogData.nEndOffset - g_LogData.nStartOffset;
			g_LogData.nStartOffset = g_LogData.nEndOffset;
		}
		return data;
	}
	__finally
	{
		ExReleaseFastMutex(&g_LogData.mtxBuffer);
	}
}

//
// Getting log file handle
//
HANDLE getFileHandle()
{
	ExAcquireFastMutex(&g_LogData.mtxFile);
	HANDLE hFile = g_LogData.hFile;
	ExReleaseFastMutex(&g_LogData.mtxFile);
	return hFile;
}

//
// Writing block into file
//
NTSTATUS writeData(LogData data)
{
	HANDLE hFile = getFileHandle();
	if (hFile == NULL) return STATUS_SUCCESS;

	IO_STATUS_BLOCK ioStatusBlock;
	IFERR_RET(ZwWriteFile(hFile, NULL, NULL, NULL, &ioStatusBlock, data.pData, 
		(ULONG)data.nDataSize, NULL, NULL));

	return STATUS_SUCCESS;
}

//
// Process next block
//
DoWorkResult writeLogDataToFile(void* /*pContext*/)
{
	LogData data = getNextData();
	if (data.nDataSize == 0) return DoWorkResult::NoWork;

	NTSTATUS res = writeData(data);
	if (!NT_SUCCESS(res))
	{
		LOGERROR(res, "Can't write log");
		return DoWorkResult::Terminate;
	}

	bool fHasLogData = true;
	ExAcquireFastMutex(&g_LogData.mtxBuffer);
	if (g_LogData.nStartOffset == g_LogData.nEndOffset)
		fHasLogData = false;
	ExReleaseFastMutex(&g_LogData.mtxBuffer);

	return fHasLogData ? DoWorkResult::HasWork : DoWorkResult::NoWork;
}


//
// At APC irql folowing formats are denied:
// %C, %S, %lc, %ls, %wc, %ws, and %wZ
//
bool checkFormatSupportAPC(const char* sFormat)
{
	// Find denied symbols
	for (const char* pCur = sFormat; *pCur != 0; ++pCur)
	{
		if (*pCur != '%') continue;
		switch (pCur[1])
		{
		case '%':
			++pCur;
			break;
		case 'C':
		case 'S':
			return false;
		case 'l':
		case 'w':
			if (pCur[2] == 'c' || pCur[2] == 's' || pCur[2] == 'Z')
				return false;
			break;
		}
	}

	return true;
}

//
//
//
void vLogInfo(const char* sFormat, va_list va)
{
	// Logging work at IRQL <= APC_LEVEL
	if (KeGetCurrentIrql() > APC_LEVEL) return;

	// Special process for APC_LEVEL
	if (KeGetCurrentIrql() == APC_LEVEL && !checkFormatSupportAPC(sFormat))
		return;

	// Output to DbgView
	vDbgPrintExWithPrefix("[CMD] ", DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, sFormat, va);

	// Check Finalize or Error
	if (!g_LogData.m_fIsInitialized) return;
	if (g_Worker.isTerminated()) return;

	// Format to buffer. If result data too large then truncate.
	char pBuffer[512];
	size_t nDataSize = 0;
	{
		NTSTATUS ns = RtlStringCbVPrintfA(pBuffer, sizeof(pBuffer), sFormat, va);
		// Check unknown error
		if (NT_ERROR(ns) && ns != STATUS_BUFFER_OVERFLOW) return;
		if (NT_ERROR(RtlStringCbLengthA(pBuffer, sizeof(pBuffer), &nDataSize))) return;
	}

	// Add data to worker
	__try
	{
		ExAcquireFastMutex(&g_LogData.mtxBuffer);

		char* pData = pBuffer;

		// Write up to the buffer's end
		if (g_LogData.nEndOffset + nDataSize >= g_LogData.nBufferSize)
		{
			size_t nCopySize = g_LogData.nBufferSize - g_LogData.nEndOffset;
			memcpy(g_LogData.pBuffer + g_LogData.nEndOffset, pData, nCopySize);
					
			// if Start overflow
			if (g_LogData.nStartOffset > g_LogData.nEndOffset)
				g_LogData.nStartOffset = 1;
			g_LogData.nEndOffset = 0;

			nDataSize -= nCopySize;
			pData += nCopySize;
		}

		// Write rest data
		memcpy(g_LogData.pBuffer + g_LogData.nEndOffset, pData, nDataSize);
		size_t nNewEndOffset = g_LogData.nEndOffset + nDataSize;

		// if Start overflow
		if (g_LogData.nEndOffset < g_LogData.nStartOffset && 
			nNewEndOffset > g_LogData.nStartOffset)
			g_LogData.nStartOffset = (nNewEndOffset + 1) % g_LogData.nBufferSize;

		g_LogData.nEndOffset = nNewEndOffset;
	}
	__finally
	{
		ExReleaseFastMutex(&g_LogData.mtxBuffer);
	}

	if (getFileHandle() != NULL)
		g_Worker.notify();
}

//
//
//
void logInfo(const char* sFormat, ...)
{
	va_list va;
	va_start(va, sFormat);
	vLogInfo(sFormat, va);
	va_end(va);
}

} // namespace log
} // namespace cmd
/// @}
