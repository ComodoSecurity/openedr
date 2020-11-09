/*
  * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  * 
  * Licensed under the Apache License, Version 2.0 (the "License").
  * You may not use this file except in compliance with the License.
  * A copy of the License is located at
  * 
  *  http://aws.amazon.com/apache2.0
  * 
  * or in the "license" file accompanying this file. This file is distributed
  * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
  * express or implied. See the License for the specific language governing
  * permissions and limitations under the License.
  */

#pragma once

#include <aws/core/SDKConfig.h>

#if defined (USE_WINDOWS_DLL_SEMANTICS) || defined (WIN32)
    #ifdef _MSC_VER
        #pragma warning(disable : 4251)
    #endif // _MSC_VER

    #ifdef USE_IMPORT_EXPORT
        #ifdef AWS_CORE_EXPORTS
            #define  AWS_CORE_API __declspec(dllexport)
        #else // AWS_CORE_EXPORTS
            #define  AWS_CORE_API __declspec(dllimport)
        #endif // AWS_CORE_EXPORTS
    #else // USE_IMPORT_EXPORT
        #define AWS_CORE_API
    #endif // USE_IMPORT_EXPORT
#else // defined (USE_WINDOWS_DLL_SEMANTICS) || defined (WIN32)
    #define AWS_CORE_API
#endif // defined (USE_WINDOWS_DLL_SEMANTICS) || defined (WIN32)

#if defined(AWS_DISABLE_DEPRECATION) && defined(_MSC_VER)
    #define AWS_DEPRECATED(msg)
    #define AWS_SUPPRESS_DEPRECATION(...) __VA_ARGS__
#elif defined (__cplusplus) && __cplusplus > 201103L // standard attributes are available since C++14
    #define AWS_DEPRECATED(msg) [[deprecated(msg)]]
#else
    #ifdef _MSC_VER
        #define AWS_DEPRECATED(msg) __declspec(deprecated(msg))
        #define AWS_SUPPRESS_WARNING(W, ...) __pragma(warning(push)) \
            __pragma(warning(disable:W))\
            __VA_ARGS__;\
            __pragma(warning(pop))
        #define AWS_SUPPRESS_DEPRECATION(...) AWS_SUPPRESS_WARNING(4996, __VA_ARGS__)
    #elif defined (__GNUC__)
        #define DO_PRAGMA(x) _Pragma(#x)
        #define AWS_DEPRECATED(msg) __attribute__((deprecated(msg)))
        /** 
         * WRAP() is a useless macro to get around GCC quirks related to expanding macros which includes _Pragma
         * see https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=715271
         * and https://stackoverflow.com/questions/49698645/how-to-use-pragma-operator-in-macro
         */
        #define WRAP(W) DO_PRAGMA(GCC diagnostic push) \
                        DO_PRAGMA(GCC diagnostic ignored W)
        #define AWS_SUPPRESS_WARNING(W, ...) \
            W \
            __VA_ARGS__; \
            DO_PRAGMA(GCC diagnostic pop)
        /**
         * Clang is the only compiler that does not emit warnings for deprecated functions used within deprecated functions.
         * Other compilers will emit a warning regardless of the context of the call. This is why this macro is needed. To
         * silence the redundant warnings.
         */
        #define AWS_SUPPRESS_DEPRECATION(...) AWS_SUPPRESS_WARNING(WRAP("-Wdeprecated-declarations"), __VA_ARGS__)
    #else
        #define AWS_DEPRECATED(msg)
    #endif
#endif

