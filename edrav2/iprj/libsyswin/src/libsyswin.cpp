//
// EDRAv2.libsyswin project
//
// Library classes registration
//
#include "pch.h"
#include "winsvcctrl.h"
#include "procdataprovider.h"
#include "userdataprovider.h"
#include "filedataprovider.h"
#include "signdataprovider.h"

//
// Classes registration
//
CMD_BEGIN_LIBRARY_DEFINITION(libsyswin)
CMD_DEFINE_LIBRARY_CLASS(sys::win::WinServiceController);
CMD_DEFINE_LIBRARY_CLASS(sys::win::ProcessDataProvider);
CMD_DEFINE_LIBRARY_CLASS(sys::win::UserDataProvider);
CMD_DEFINE_LIBRARY_CLASS(sys::win::FileDataProvider);
CMD_DEFINE_LIBRARY_CLASS(sys::win::SignDataProvider);
CMD_DEFINE_LIBRARY_CLASS(sys::win::PathConverter);
CMD_DEFINE_LIBRARY_CLASS(sys::win::Token);
CMD_END_LIBRARY_DEFINITION(libsyswin)

