//
// edrav2.libcore project
//
// Author: Yury Ermakov (07.10.2019)
// Reviewer:
//
#include "pch.h"
#include <events.hpp>

namespace openEdr {

namespace eventtype {

constexpr char c_sLleNone[] = "LLE_NONE";
constexpr char c_sLleProcessCreate[] = "LLE_PROCESS_CREATE";
constexpr char c_sLleProcessDelete[] = "LLE_PROCESS_DELETE";
constexpr char c_sLleFileCreate[] = "LLE_FILE_CREATE";
constexpr char c_sLleFileDelete[] = "LLE_FILE_DELETE";
constexpr char c_sLleFileClose[] = "LLE_FILE_CLOSE";
constexpr char c_sLleFileDataChange[] = "LLE_FILE_DATA_CHANGE";
constexpr char c_sLleFileDataReadFull[] = "LLE_FILE_DATA_READ_FULL";
constexpr char c_sLleFileDataWriteFull[] = "LLE_FILE_DATA_WRITE_FULL";
constexpr char c_sLleRegistryKeyCreate[] = "LLE_REGISTRY_KEY_CREATE";
constexpr char c_sLleRegistryKeyNameChange[] = "LLE_REGISTRY_KEY_NAME_CHANGE";
constexpr char c_sLleRegistryKeyDelete[] = "LLE_REGISTRY_KEY_DELETE";
constexpr char c_sLleRegistryValueSet[] = "LLE_REGISTRY_VALUE_SET";
constexpr char c_sLleRegistryValueDelete[] = "LLE_REGISTRY_VALUE_DELETE";
constexpr char c_sLleProcessOpen[] = "LLE_PROCESS_OPEN";
constexpr char c_sLleProcessMemoryRead[] = "LLE_PROCESS_MEMORY_READ";
constexpr char c_sLleProcessMemoryWrite[] = "LLE_PROCESS_MEMORY_WRITE";
constexpr char c_sLleWindowProcGlobalHook[] = "LLE_WINDOW_PROC_GLOBAL_HOOK";
constexpr char c_sLleKeyboardGlobalRead[] = "LLE_KEYBOARD_GLOBAL_READ";
constexpr char c_sLleDiskRawWriteAccess[] = "LLE_DISK_RAW_WRITE_ACCESS";
constexpr char c_sLleVolumeRawWriteAccess[] = "LLE_VOLUME_RAW_WRITE_ACCESS";
constexpr char c_sLleDeviceRawWriteAccess[] = "LLE_DEVICE_RAW_WRITE_ACCESS";
constexpr char c_sLleDiskLinkCreate[] = "LLE_DISK_LINK_CREATE";
constexpr char c_sLleVolumeLinkCreate[] = "LLE_VOLUME_LINK_CREATE";
constexpr char c_sLleDeviceLinkCreate[] = "LLE_DEVICE_LINK_CREATE";
constexpr char c_sLleInjectionActivity[] = "LLE_INJECTION_ACTIVITY";
constexpr char c_sLleKeyboardBlock[] = "LLE_KEYBOARD_BLOCK";
constexpr char c_sLleClipboardRead[] = "LLE_CLIPBOARD_READ";
//constexpr char c_sMleFileCopyToUsb[] = "MLE_FILE_COPY_TO_USB";
//constexpr char c_sMleFileCopyToShare[] = "MLE_FILE_COPY_TO_SHARE";
//constexpr char c_sMleFileCopyFromUsb[] = "MLE_FILE_COPY_FROM_USB";
//constexpr char c_sMleFileCopyFromShare[] = "MLE_FILE_COPY_FROM_SHARE";
constexpr char c_sLleKeyboardGlobalWrite[] = "LLE_KEYBOARD_GLOBAL_WRITE";
constexpr char c_sLleMicrophoneEnum[] = "LLE_MICROPHONE_ENUM";
constexpr char c_sLleMicrophoneRead[] = "LLE_MICROPHONE_READ";
constexpr char c_sLleMouseGlobalWrite[] = "LLE_MOUSE_GLOBAL_WRITE";
constexpr char c_sLleWindowDataRead[] = "LLE_WINDOW_DATA_READ";
constexpr char c_sLleDesktopWallpaperSet[] = "LLE_DESKTOP_WALLPAPER_SET";
constexpr char c_sLleMouseBlock[] = "LLE_MOUSE_BLOCK";
constexpr char c_sLleNetworkConnectIn[] = "LLE_NETWORK_CONNECT_IN";
constexpr char c_sLleNetworkConnectOut[] = "LLE_NETWORK_CONNECT_OUT";
constexpr char c_sLleNetworkListen[] = "LLE_NETWORK_LISTEN";
//constexpr char c_sMleFileCopy[] = "MLE_FILE_COPY";
constexpr char c_sLleUserLogon[] = "LLE_USER_LOGON";
constexpr char c_sLleUserImpersonation[] = "LLE_USER_IMPERSONATION";
constexpr char c_sLleNetworkRequestDns[] = "LLE_NETWORK_REQUEST_DNS";
constexpr char c_sLleNetworkRequestData[] = "LLE_NETWORK_REQUEST_DATA";



} // namespace eventtype

//
//
//
const char* getEventTypeString(Event eventType)
{
	switch (eventType)
	{
		case Event::LLE_NONE: return eventtype::c_sLleNone;
		case Event::LLE_PROCESS_CREATE: return eventtype::c_sLleProcessCreate;
		case Event::LLE_PROCESS_DELETE: return eventtype::c_sLleProcessDelete;
		case Event::LLE_FILE_CREATE: return eventtype::c_sLleFileCreate;
		case Event::LLE_FILE_DELETE: return eventtype::c_sLleFileDelete;
		case Event::LLE_FILE_CLOSE: return eventtype::c_sLleFileClose;
		case Event::LLE_FILE_DATA_CHANGE: return eventtype::c_sLleFileDataChange;
		case Event::LLE_FILE_DATA_READ_FULL: return eventtype::c_sLleFileDataReadFull;
		case Event::LLE_FILE_DATA_WRITE_FULL: return eventtype::c_sLleFileDataWriteFull;
		case Event::LLE_REGISTRY_KEY_CREATE: return eventtype::c_sLleRegistryKeyCreate;
		case Event::LLE_REGISTRY_KEY_NAME_CHANGE: return eventtype::c_sLleRegistryKeyNameChange;
		case Event::LLE_REGISTRY_KEY_DELETE: return eventtype::c_sLleRegistryKeyDelete;
		case Event::LLE_REGISTRY_VALUE_SET: return eventtype::c_sLleRegistryValueSet;
		case Event::LLE_REGISTRY_VALUE_DELETE: return eventtype::c_sLleRegistryValueDelete;
		case Event::LLE_PROCESS_OPEN: return eventtype::c_sLleProcessOpen;
		case Event::LLE_PROCESS_MEMORY_READ: return eventtype::c_sLleProcessMemoryRead;
		case Event::LLE_PROCESS_MEMORY_WRITE: return eventtype::c_sLleProcessMemoryWrite;
		case Event::LLE_WINDOW_PROC_GLOBAL_HOOK: return eventtype::c_sLleWindowProcGlobalHook;
		case Event::LLE_KEYBOARD_GLOBAL_READ: return eventtype::c_sLleKeyboardGlobalRead;
		case Event::LLE_DISK_RAW_WRITE_ACCESS: return eventtype::c_sLleDiskRawWriteAccess;
		case Event::LLE_VOLUME_RAW_WRITE_ACCESS: return eventtype::c_sLleVolumeRawWriteAccess;
		case Event::LLE_DEVICE_RAW_WRITE_ACCESS: return eventtype::c_sLleDeviceRawWriteAccess;
		case Event::LLE_DISK_LINK_CREATE: return eventtype::c_sLleDiskLinkCreate;
		case Event::LLE_VOLUME_LINK_CREATE: return eventtype::c_sLleVolumeLinkCreate;
		case Event::LLE_DEVICE_LINK_CREATE: return eventtype::c_sLleDeviceLinkCreate;
		case Event::LLE_INJECTION_ACTIVITY: return eventtype::c_sLleInjectionActivity;
		case Event::LLE_KEYBOARD_BLOCK: return eventtype::c_sLleKeyboardBlock;
		case Event::LLE_CLIPBOARD_READ: return eventtype::c_sLleClipboardRead;
		//case Event::MLE_FILE_COPY_TO_USB: return eventtype::c_sMleFileCopyToUsb;
		//case Event::MLE_FILE_COPY_TO_SHARE: return eventtype::c_sMleFileCopyToShare;
		//case Event::MLE_FILE_COPY_FROM_USB: return eventtype::c_sMleFileCopyFromUsb;
		//case Event::MLE_FILE_COPY_FROM_SHARE: return eventtype::c_sMleFileCopyFromShare;
		case Event::LLE_KEYBOARD_GLOBAL_WRITE: return eventtype::c_sLleKeyboardGlobalWrite;
		case Event::LLE_MICROPHONE_ENUM: return eventtype::c_sLleMicrophoneEnum;
		case Event::LLE_MICROPHONE_READ: return eventtype::c_sLleMicrophoneRead;
		case Event::LLE_MOUSE_GLOBAL_WRITE: return eventtype::c_sLleMouseGlobalWrite;
		case Event::LLE_WINDOW_DATA_READ: return eventtype::c_sLleWindowDataRead;
		case Event::LLE_DESKTOP_WALLPAPER_SET: return eventtype::c_sLleDesktopWallpaperSet;
		case Event::LLE_MOUSE_BLOCK: return eventtype::c_sLleMouseBlock;
		case Event::LLE_NETWORK_CONNECT_IN: return eventtype::c_sLleNetworkConnectIn;
		case Event::LLE_NETWORK_CONNECT_OUT: return eventtype::c_sLleNetworkConnectOut;
		case Event::LLE_NETWORK_LISTEN: return eventtype::c_sLleNetworkListen;
		//case Event::MLE_FILE_COPY: return eventtype::c_sMleFileCopy;
		case Event::LLE_USER_LOGON: return eventtype::c_sLleUserLogon;
		case Event::LLE_USER_IMPERSONATION: return eventtype::c_sLleUserImpersonation;
		case Event::LLE_NETWORK_REQUEST_DNS: return eventtype::c_sLleNetworkRequestDns;
		case Event::LLE_NETWORK_REQUEST_DATA: return eventtype::c_sLleNetworkRequestData;
	}
	error::InvalidArgument(SL, FMT("Event type <" << (uint64_t)eventType <<
		"> has no text string")).throwException();
}

} // namespace openEdr
