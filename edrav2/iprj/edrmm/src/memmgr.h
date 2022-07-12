//
// EDRAv2.edrmm project
//
// Autor: Yury Podpruzhnikov (08.04.2019)
// Reviewer: Denis Bogdanov (22.02.2019)
//
///
/// @file Memory manager
///
/// @addtogroup edrmm
/// 
/// Memory manager provides memory allocation and leak detection functions.
/// 
/// @{
#pragma once

namespace cmd {
namespace edrmm {

//
// Saves initial memory checkpoint
//
void startMemoryLeaksMonitoring();

//
// Saves memory leak information into file
//
void saveMemoryLeaks();

} // namespace edrmm
} // namespace cmd

