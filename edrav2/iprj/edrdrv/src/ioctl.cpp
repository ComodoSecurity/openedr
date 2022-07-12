//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Driver IOCTL
///
/// @addtogroup edrdrv
/// @{
#include "common.h"
#include "osutils.h"
#include "fltport.h"
#include "ioctl.h"
#include "config.h"
#include "procmon.h"
#include "filemon.h"
#include "regmon.h"

namespace cmd {
namespace drvioctl {

///
/// DeviceInputOutput class.
///
class DeviceInputOutput
{
public:
	PVOID m_pInput;
	ULONG m_nInputSize;
	PVOID m_pOutput;
	ULONG m_nOutputSize;
	PULONG m_pSizeWritten;

public:
	DeviceInputOutput(PVOID pInput_, ULONG dInput_, PVOID pOutput_, 
		ULONG dOutput_, PULONG pdInfo_) : 
		m_pInput(pInput_), m_nInputSize(dInput_), 
		m_pOutput(pOutput_), m_nOutputSize(dOutput_), 
		m_pSizeWritten(pdInfo_)
	{
	}
	
	///
	/// Checked coping data from the Input.
	///
	/// Data buffer must be filled full.
	///
	/// @param pData - data buffer.
	/// @param nExcpectedDataSize - data size.
	/// @return STATUS_INVALID_BUFFER_SIZE if Input is less than data.
	///
	NTSTATUS readInput(PVOID pData, ULONG nExcpectedDataSize)
	{
		if (nExcpectedDataSize > m_nInputSize)
			return STATUS_INVALID_BUFFER_SIZE;
		RtlCopyMemory(pData, m_pInput, nExcpectedDataSize);
		return STATUS_SUCCESS;
	}

	///
	/// Checked coping data from the Input.
	///
	/// Data buffer must be filled full.
	///
	/// @param Data - data buffer.
	/// @return STATUS_INVALID_BUFFER_SIZE if Input is less than data.
	///
	template<typename T>
	NTSTATUS readInput(T& Data)
	{
		return readInput((PVOID) &Data, sizeof(T));
	}
	
	///
	/// Checked getting data from the Input.
	///
	/// Verify data size.
	///
	/// @param ppData [out] - return pointer.
	/// @param nExcpectedDataSize - checked size.
	/// @return STATUS_INVALID_BUFFER_SIZE if Input is less than data.
	///
	NTSTATUS getInput(PVOID* ppData, ULONG nExcpectedDataSize)
	{
		if (nExcpectedDataSize > m_nInputSize) 
			return STATUS_INVALID_BUFFER_SIZE;
		*ppData = m_pInput;
		return STATUS_SUCCESS;
	}

	///
	/// Special getter for LBVS deserializer.
	///
	/// @param deserializer [out] - deserializer for filling.
	/// @return STATUS_DATA_ERROR if Input is invalid.
	///
	template<typename IdType>
	NTSTATUS getInput(variant::BasicLbvsDeserializer<IdType>& deserializer)
	{
		if(!deserializer.reset(m_pInput, m_nInputSize))
			return STATUS_DATA_ERROR;
		return STATUS_SUCCESS;
	}

	///
	/// Checked getting data from the Input.
	///
	/// Verify data size.
	///
	/// @param Data [out] - return pointer.
	/// @return STATUS_INVALID_BUFFER_SIZE if Input is less than data.
	///
	template<typename T>
	NTSTATUS getInput(T*& Data)
	{
		return getInput((PVOID*) &Data, sizeof(T));
	}

	///
	/// Checked writing data to the Output.
	///
	/// Verify data size.
	///
	/// @param pData - data buffer.
	/// @param nDataSize - data size.
	/// @return STATUS_BUFFER_TOO_SMALL if Output is less than data.
	///
	NTSTATUS writeOutput(PVOID pData, ULONG nDataSize)
	{
		if (nDataSize > m_nOutputSize) return STATUS_BUFFER_TOO_SMALL;
		RtlCopyMemory(m_pOutput, pData, nDataSize);
		*m_pSizeWritten = nDataSize;
		return STATUS_SUCCESS;
	}

	///
	/// Checked writing data to the Output.
	///
	/// Verify data size.
	///
	/// @param Data - data buffer.
	/// @return STATUS_BUFFER_TOO_SMALL if Output is less than data.
	///
	template<typename T>
	NTSTATUS writeOutput(const T& Data)
	{
		return writeOutput((PVOID)&Data, sizeof(T));
	}
	
	///
	/// Checked getting pointer to the Output.
	///
	/// Verify data size.
	///
	/// @param ppOutput - returned pointer.
	/// @return STATUS_BUFFER_TOO_SMALL if Output is less than data.
	///
	template<typename T>
	bool testOutput(T** ppOutput)
	{
		if (m_nOutputSize != sizeof(T)) return false;
		*ppOutput = (T*)m_pOutput;
		*m_pSizeWritten = sizeof(T);
		return true;
	}
};

//
//
//
void setLogFile(DeviceInputOutput &inOut, size_t nFileNameOffset)
{
	const UINT8* pInput = (const UINT8*)inOut.m_pInput;
	size_t nInputSize = inOut.m_nInputSize;

	// Check buffer size
	if (nFileNameOffset >= nInputSize)
	{
		LOGERROR(STATUS_INVALID_PARAMETER, "Invalid nLogFileOffset: %u. Out of buffer.\r\n", (ULONG)nFileNameOffset);
		return;
	}

	// Check zero end
	const WCHAR* sFileName = (const WCHAR*)(pInput + nFileNameOffset);
	const WCHAR* sBufferEnd = sFileName + (nInputSize - nFileNameOffset) / sizeof(WCHAR);

	bool fZeroEndIsFound = false;
	for (auto pCh = sFileName; pCh < sBufferEnd; ++pCh)
	{
		if (*pCh != 0) continue;
		fZeroEndIsFound = true;
		break;
	}

	if(!fZeroEndIsFound)
	{
		LOGERROR(STATUS_INVALID_PARAMETER, "Invalid nLogFileOffset: %u. Zero-end not found.\r\n", (ULONG)nFileNameOffset);
		return;
	}

	IFERR_LOG(log::setLogFile(sFileName), "Can't set Log file: <%S>.\r\n", sFileName);
}

//
//
//
NTSTATUS processRequest(DeviceInputOutput &inOut, ULONG IoControl)
{
	switch (IoControl)
	{
		case CMD_ERDDRV_IOCTL_START:
		{
			IFERR_RET(startMonitoring());
			return STATUS_SUCCESS;
		}
		case CMD_ERDDRV_IOCTL_STOP:
		{
			stopMonitoring(true);
			return STATUS_SUCCESS;
		}
		case CMD_ERDDRV_IOCTL_SET_CONFIG:
		{
			if (!procmon::isCurProcessTrusted())
				return LOGERROR(STATUS_ACCESS_DENIED, "Can't access from process: %Iu.\r\n",
				(ULONG_PTR)PsGetCurrentProcessId());

			LOGINFO2("Update config.\r\n");
			bool fChanged = false;
			IFERR_RET(loadMainConfigFromBuffer(inOut.m_pInput, inOut.m_nInputSize, fChanged));
			if(fChanged)
				IFERR_LOG(saveMainConfigToRegistry());
			return STATUS_SUCCESS;
		}
		case CMD_ERDDRV_IOCTL_UPDATE_FILE_RULES:
		{
			if (!procmon::isCurProcessTrusted())
				return LOGERROR(STATUS_ACCESS_DENIED, "Can't access from process: %Iu.\r\n",
				(ULONG_PTR)PsGetCurrentProcessId());

			IFERR_RET(filemon::updateFileRules(inOut.m_pInput, inOut.m_nInputSize));
			return STATUS_SUCCESS;
		}
		case CMD_ERDDRV_IOCTL_UPDATE_REG_RULES:
		{
			if (!procmon::isCurProcessTrusted())
				return LOGERROR(STATUS_ACCESS_DENIED, "Can't access from process: %Iu.\r\n",
				(ULONG_PTR)PsGetCurrentProcessId());

			IFERR_RET(regmon::updateRegRules(inOut.m_pInput, inOut.m_nInputSize));
			return STATUS_SUCCESS;
		}
		case CMD_ERDDRV_IOCTL_UPDATE_PROCESS_RULES:
		{
			if (!procmon::isCurProcessTrusted())
				return LOGERROR(STATUS_ACCESS_DENIED, "Can't access from process: %Iu.\r\n",
				(ULONG_PTR)PsGetCurrentProcessId());

			IFERR_RET(procmon::updateProcessRules(inOut.m_pInput, inOut.m_nInputSize));
			IFERR_RET(procmon::reapplyRulesForAllProcesses());
			return STATUS_SUCCESS;
		}
		case CMD_ERDDRV_IOCTL_SET_PROCESS_INFO:
		{
			if (!procmon::isCurProcessTrusted())
				return LOGERROR(STATUS_ACCESS_DENIED, "Can't access from process: %Iu.\r\n",
				(ULONG_PTR)PsGetCurrentProcessId());

			// Create deserializer
			variant::BasicLbvsDeserializer<SetProcessInfoField> deserializer;
			if (!deserializer.reset(inOut.m_pInput, inOut.m_nInputSize))
				return LOGERROR(STATUS_DATA_ERROR, "Invalid data format.\r\n");

			IFERR_RET(procmon::setProcessInfo(deserializer));
			return STATUS_SUCCESS;
		}
	}
	return STATUS_NOT_SUPPORTED;
}

//
//
//
NTSTATUS notifyOnDeviceIoControl(PIRP pIrp)
{
	PIO_STACK_LOCATION pisl = IoGetCurrentIrpStackLocation(pIrp);

	if (pisl->MajorFunction != IRP_MJ_DEVICE_CONTROL)
	{
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	ULONG dInfo = 0;
	DeviceInputOutput inOut(pIrp->AssociatedIrp.SystemBuffer,
		pisl->Parameters.DeviceIoControl.InputBufferLength,
		pIrp->AssociatedIrp.SystemBuffer,
		pisl->Parameters.DeviceIoControl.OutputBufferLength, &dInfo);
	ULONG IoControl = pisl->Parameters.DeviceIoControl.IoControlCode;

	NTSTATUS ns = processRequest(inOut, IoControl);
	IFERR_LOG(ns, "Processing ioctl(0x%08x) returns error.\r\n", IoControl);
	pIrp->IoStatus.Status = ns;
	pIrp->IoStatus.Information = dInfo;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return ns;
}


PRESET_STATIC_UNICODE_STRING(c_usNtDeviceName, CMD_ERDDRV_IOCTLDEVICE_NT_NAME);
PRESET_STATIC_UNICODE_STRING(c_usDosDeviceName, CMD_ERDDRV_IOCTLDEVICE_SYMLINK_NAME);

//
//
//
_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS initialize()
{
	LOGINFO2("Register drvioctl\r\n");

	bool fSuccessInit = false;
	__try
	{
		// Create devise
		IFERR_RET(IoCreateDevice(
			g_pCommonData->pDriverObject,      // Our Driver Object
			0,                              // We don't use a device extension
			&c_usNtDeviceName,              // Device name
			FILE_DEVICE_UNKNOWN,            // Device type
			FILE_DEVICE_SECURE_OPEN,        // Device characteristics
			FALSE,                          // Not an exclusive device
			&g_pCommonData->pIoctlDeviceObject)); // Returned ptr to Device Object

		// Create symlink
		IFERR_RET(IoCreateSymbolicLink(&c_usDosDeviceName, &c_usNtDeviceName));
		g_pCommonData->pusIoctlDeviceSymlink = &c_usDosDeviceName;

		fSuccessInit = true;
	}
	__finally
	{
		if (!fSuccessInit)
			finalize();
	}

	return STATUS_SUCCESS;
}

//
//
//
void finalize()
{
	if (g_pCommonData->pusIoctlDeviceSymlink != nullptr)
	{
		IoDeleteSymbolicLink(g_pCommonData->pusIoctlDeviceSymlink);
		g_pCommonData->pusIoctlDeviceSymlink = nullptr;
	}

	if (g_pCommonData->pIoctlDeviceObject != nullptr)
	{
		IoDeleteDevice(g_pCommonData->pIoctlDeviceObject);
		g_pCommonData->pIoctlDeviceObject = nullptr;
	}
}

namespace detail {

//
//
//
bool isSupportDevice(_DEVICE_OBJECT * pDeviceObject)
{
	return pDeviceObject == g_pCommonData->pIoctlDeviceObject;
}

//
//
//
NTSTATUS dispatchIrp(_DEVICE_OBJECT * /*pDeviceObject*/, _IRP * pIrp)
{
	return notifyOnDeviceIoControl(pIrp);
}

} // namespace detail

} // namespace drvioctl
} // namespace cmd
/// @}
