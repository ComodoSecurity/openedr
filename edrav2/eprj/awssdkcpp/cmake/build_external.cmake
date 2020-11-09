# The NDK does not provide any http or crypto functionality out of the box; we build versions of zlib, openssl, and curl to account for this.
if(BUILD_CURL OR BUILD_OPENSSL OR BUILD_ZLIB)
    include(ExternalProject)

    set(EXTERNAL_CXX_FLAGS "-Wno-unused-private-field")
    set(EXTERNAL_C_FLAGS "")

    set(BASE_SDK_DIR ${CMAKE_BINARY_DIR} CACHE STRING "Android build" FORCE)

    # we patch the install process for each dependency to match what we need for 3rd party installation
    set(EXTERNAL_INSTALL_DIR ${CMAKE_BINARY_DIR}/external)

    # zlib
    if(BUILD_ZLIB)
        set(ZLIB_SOURCE_DIR ${CMAKE_BINARY_DIR}/zlib CACHE INTERNAL "zlib source dir")
        set(ZLIB_INSTALL_DIR ${EXTERNAL_INSTALL_DIR}/zlib CACHE INTERNAL "zlib install dir")
        set(ZLIB_INCLUDE_DIR ${ZLIB_INSTALL_DIR}/include/zlib CACHE INTERNAL "zlib include dir")
        set(ZLIB_LIBRARY_DIR ${ZLIB_INSTALL_DIR}/lib CACHE INTERNAL "zlib library dir")

        set( ZLIB_INCLUDE_FLAGS "-isystem ${ZLIB_INCLUDE_DIR}" CACHE INTERNAL "compiler flags to find zlib includes")
        set( ZLIB_LINKER_FLAGS "-L${ZLIB_LIBRARY_DIR}" CACHE INTERNAL "linker flags to find zlib")

        #zlib
        #based on http://stackoverflow.com/questions/16842218/how-to-use-cmake-externalproject-add-or-alternatives-in-a-cross-platform-way
        #likely, some of the things here are unnecessary
        ExternalProject_Add(ZLIB
            SOURCE_DIR ${ZLIB_SOURCE_DIR}
            URL https://sdk.amazonaws.com/cpp/builds/zlib-1.2.11.tar.gz
            URL_HASH "SHA256=c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1"
            PATCH_COMMAND ""
            CMAKE_ARGS
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DANDROID_NATIVE_API_LEVEL=${ANDROID_NATIVE_API_LEVEL}
            -DANDROID_ABI=${ANDROID_ABI}
            -DANDROID_TOOLCHAIN_NAME=${ANDROID_TOOLCHAIN_NAME}
            -DANDROID_STANDALONE_TOOLCHAIN=${ANDROID_STANDALONE_TOOLCHAIN}
            -DANDROID_STL=${ANDROID_STL}
            -DCMAKE_INSTALL_PREFIX=${ZLIB_INSTALL_DIR}
            -DCMAKE_CXX_FLAGS=${EXTERNAL_CXX_FLAGS}
            -DCMAKE_C_FLAGS=${EXTERNAL_C_FLAGS}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_SHARED_LIBS=0
            )

        if(UNIX)
            set(ZLIB_NAME libz)
        else()
            set(ZLIB_NAME zlib)
        endif()

        add_library(zlib UNKNOWN IMPORTED)
        set_property(TARGET zlib PROPERTY IMPORTED_LOCATION ${ZLIB_LIBRARY_DIR}/${ZLIB_NAME}.a)
        set(ZLIB_LIBRARIES "${ZLIB_LIBRARY_DIR}/${ZLIB_NAME}.a")
        set(CURL_ZLIB_DEPENDENCY "ZLIB")
    endif()

    # OpenSSL
    if(BUILD_OPENSSL)
        set(OPENSSL_SOURCE_DIR ${CMAKE_BINARY_DIR}/openssl-src CACHE INTERNAL "openssl source dir")
        set(OPENSSL_INSTALL_DIR ${EXTERNAL_INSTALL_DIR}/openssl CACHE INTERNAL "openssl install dir")
        set(OPENSSL_INCLUDE_DIR ${OPENSSL_INSTALL_DIR}/include CACHE INTERNAL "openssl include dir")
        set(OPENSSL_LIBRARY_DIR ${OPENSSL_INSTALL_DIR}/lib CACHE INTERNAL "openssl library dir")
        set(OPENSSL_CXX_FLAGS "${EXTERNAL_CXX_FLAGS} -fPIE" CACHE INTERNAL "openssl")
        set(OPENSSL_C_FLAGS "${EXTERNAL_C_FLAGS} -fPIE" CACHE INTERNAL "openssl")
        if(ANDROID_ABI STREQUAL "x86_64")
            set(OPENSSL_C_FLAGS "${OPENSSL_C_FLAGS} -DOPENSSL_NO_INLINE_ASM" CACHE INTERNAL "openssl")
        endif()
        set(OPENSSL_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie" CACHE INTERNAL "openssl")

        set(OPENSSL_INCLUDE_FLAGS "-isystem ${OPENSSL_INCLUDE_DIR} -isystem ${OPENSSL_INCLUDE_DIR}/openssl" CACHE INTERNAL "compiler flags to find openssl includes")
        set(OPENSSL_LINKER_FLAGS "-L${OPENSSL_LIBRARY_DIR}" CACHE INTERNAL "linker flags to find openssl")

        ExternalProject_Add(OPENSSL
            SOURCE_DIR ${OPENSSL_SOURCE_DIR}
            GIT_REPOSITORY https://github.com/openssl/openssl.git
            GIT_TAG e216bf9d7ca761718f34e8b3094fcb32c7a143e4 # 1.0.2j
	    UPDATE_COMMAND ""
            PATCH_COMMAND cd ${CMAKE_BINARY_DIR} && python ${AWS_NATIVE_SDK_ROOT}/android-build/configure_openssl_cmake.py --source ${AWS_NATIVE_SDK_ROOT} --dest ${OPENSSL_SOURCE_DIR}
            CMAKE_ARGS
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DANDROID_NATIVE_API_LEVEL=${ANDROID_NATIVE_API_LEVEL}
            -DANDROID_ABI=${ANDROID_ABI}
            -DANDROID_TOOLCHAIN_NAME=${ANDROID_TOOLCHAIN_NAME}
            -DANDROID_STANDALONE_TOOLCHAIN=${ANDROID_STANDALONE_TOOLCHAIN}
            -DANDROID_STL=${ANDROID_STL}
            -DCMAKE_INSTALL_PREFIX=${OPENSSL_INSTALL_DIR}
            -DCMAKE_CXX_FLAGS=${OPENSSL_CXX_FLAGS}
            -DCMAKE_C_FLAGS=${OPENSSL_C_FLAGS}
            -DCMAKE_EXE_LINKER_FLAGS=${OPENSSL_EXE_LINKER_FLAGS}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_SHARED_LIBS=0
            )

        add_library(ssl UNKNOWN IMPORTED)
        set_property(TARGET ssl PROPERTY IMPORTED_LOCATION ${OPENSSL_LIBRARY_DIR}/libssl.a)
        add_library(crypto UNKNOWN IMPORTED)
        set_property(TARGET crypto PROPERTY IMPORTED_LOCATION ${OPENSSL_LIBRARY_DIR}/libcrypto.a)

        set(OPENSSL_LIBRARIES "${OPENSSL_LIBRARY_DIR}/libssl.a;${OPENSSL_LIBRARY_DIR}/libcrypto.a")
        set(CURL_OPENSSL_DEPENDENCY "OPENSSL")
    endif()

    # curl
    if(BUILD_CURL)
        set(CURL_SOURCE_DIR ${CMAKE_BINARY_DIR}/curl CACHE INTERNAL "libcurl source dir")
        set(CURL_INSTALL_DIR ${EXTERNAL_INSTALL_DIR}/curl CACHE INTERNAL "libcurl install dir")
        set(CURL_INCLUDE_DIR ${CURL_INSTALL_DIR}/include CACHE INTERNAL "libcurl include dir")
        set(CURL_LIBRARY_DIR ${CURL_INSTALL_DIR}/lib CACHE INTERNAL "libcurl library dir")

        set( CURL_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS}" CACHE INTERNAL "" )
        set( CURL_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}" CACHE INTERNAL "" )

        set( CURL_CXX_FLAGS "${EXTERNAL_CXX_FLAGS} ${OPENSSL_INCLUDE_FLAGS} ${ZLIB_INCLUDE_FLAGS} -Wno-unused-value -fPIE ${ZLIB_LINKER_FLAGS} ${OPENSSL_LINKER_FLAGS}" CACHE INTERNAL "")
        set( CURL_C_FLAGS "${EXTERNAL_C_FLAGS}  ${OPENSSL_INCLUDE_FLAGS} ${ZLIB_INCLUDE_FLAGS} -Wno-unused-value -fPIE ${ZLIB_LINKER_FLAGS} ${OPENSSL_LINKER_FLAGS}" CACHE INTERNAL "")
        set( CURL_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie ${ZLIB_LINKER_FLAGS} ${OPENSSL_LINKER_FLAGS}" CACHE INTERNAL "" )

	if(ZLIB_LIBRARIES)
	    set(CURL_USE_ZLIB "ON")
	else()
	    set(CURL_USE_ZLIB "OFF")
	endif()

        ExternalProject_Add(CURL
                DEPENDS ${CURL_OPENSSL_DEPENDENCY} ${CURL_ZLIB_DEPENDENCY}
                SOURCE_DIR ${CURL_SOURCE_DIR}
                GIT_REPOSITORY https://github.com/bagder/curl.git
                GIT_TAG 44b9b4d4f56d6f6de92c89636994c03984e9cd01 # 7.52.1
                UPDATE_COMMAND ""
                PATCH_COMMAND ""
                CMAKE_ARGS
                -C ${AWS_NATIVE_SDK_ROOT}/android-build/CurlAndroidCrossCompile.cmake
                -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                -DANDROID_NATIVE_API_LEVEL=${ANDROID_NATIVE_API_LEVEL}
                -DANDROID_ABI=${ANDROID_ABI}
                -DANDROID_TOOLCHAIN_NAME=${ANDROID_TOOLCHAIN_NAME}
                -DANDROID_STANDALONE_TOOLCHAIN=${ANDROID_STANDALONE_TOOLCHAIN}
                -DANDROID_STL=${ANDROID_STL}
                -DCMAKE_INSTALL_PREFIX=${CURL_INSTALL_DIR}
                -DCMAKE_CXX_FLAGS=${CURL_CXX_FLAGS}
                -DCMAKE_C_FLAGS=${CURL_C_FLAGS}
                -DCMAKE_STATIC_LINKER_FLAGS=${CURL_STATIC_LINKER_FLAGS}
                -DCMAKE_SHARED_LINKER_FLAGS=${CURL_SHARED_LINKER_FLAGS}
                -DCMAKE_EXE_LINKER_FLAGS=${CURL_EXE_LINKER_FLAGS}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DOPENSSL_ROOT_DIR=${OPENSSL_SOURCE_DIR}
                -DOPENSSL_INCLUDE_DIR=${OPENSSL_INCLUDE_DIR}
                -DCURL_STATICLIB=ON
                -DBUILD_CURL_EXE=ON
                -DBUILD_CURL_TESTS=OFF
                -DCURL_ZLIB=${CURL_USE_ZLIB}
                )

        add_library(curl UNKNOWN IMPORTED)
        set_property(TARGET curl PROPERTY IMPORTED_LOCATION ${CURL_LIBRARY_DIR}/libcurl.a)

        set(CURL_LIBRARIES "${CURL_LIBRARY_DIR}/libcurl.a")
    endif()
endif()
