
message(STATUS "Generating windows build config")
set(SDK_INSTALL_BINARY_PREFIX "windows")
set(USE_WINDOWS_DLL_SEMANTICS 1)

if(BUILD_SHARED_LIBS)
    SET(SUFFIX dll)
else()
    SET(SUFFIX lib)
endif()

macro(apply_post_project_platform_settings)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDK_INSTALL_BINARY_PREFIX "${SDK_INSTALL_BINARY_PREFIX}/intel64")
    else()
        set(SDK_INSTALL_BINARY_PREFIX "${SDK_INSTALL_BINARY_PREFIX}/ia32")
    endif()    
   
    set(PLATFORM_DEP_LIBS Userenv version ws2_32)    
    set(PLATFORM_DEP_LIBS_ABSTRACT_NAME Userenv version ws2_32)

endmacro()
