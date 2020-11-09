//
// EDRAv2.libcore project
//
// Author: Yury Ermakov (17.12.2018)
// Reviewer: Denis Bogdanov (18.01.2019)
//

///
/// @file Main header file for crash handling module
///

///
/// @addtogroup errors Errors processing
/// @{

#pragma once
#include "error\dump\crashpad.hpp"

#if defined(_WIN32)
#include "error\dump\dump_win.hpp"
#endif // _WIN32 

/// @}
