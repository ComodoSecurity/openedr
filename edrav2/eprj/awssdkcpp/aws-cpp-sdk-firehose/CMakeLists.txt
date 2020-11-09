add_project(aws-cpp-sdk-firehose "C++ SDK for the AWS firehose service" aws-cpp-sdk-core)

file(GLOB AWS_FIREHOSE_HEADERS
    "include/aws/firehose/*.h"
)

file(GLOB AWS_FIREHOSE_MODEL_HEADERS
    "include/aws/firehose/model/*.h"
)

file(GLOB AWS_FIREHOSE_SOURCE
    "source/*.cpp"
)

file(GLOB AWS_FIREHOSE_MODEL_SOURCE
    "source/model/*.cpp"
)

file(GLOB FIREHOSE_UNIFIED_HEADERS
    ${AWS_FIREHOSE_HEADERS}
    ${AWS_FIREHOSE_MODEL_HEADERS}
)

file(GLOB FIREHOSE_UNITY_SRC
    ${AWS_FIREHOSE_SOURCE}
    ${AWS_FIREHOSE_MODEL_SOURCE}
)

if(ENABLE_UNITY_BUILD)
    enable_unity_build("FIREHOSE" FIREHOSE_UNITY_SRC)
endif()

file(GLOB FIREHOSE_SRC
    ${FIREHOSE_UNIFIED_HEADERS}
    ${FIREHOSE_UNITY_SRC}
)

if(WIN32)
    #if we are compiling for visual studio, create a sane directory tree.
    if(MSVC)
        source_group("Header Files\\aws\\firehose" FILES ${AWS_FIREHOSE_HEADERS})
        source_group("Header Files\\aws\\firehose\\model" FILES ${AWS_FIREHOSE_MODEL_HEADERS})
        source_group("Source Files" FILES ${AWS_FIREHOSE_SOURCE})
        source_group("Source Files\\model" FILES ${AWS_FIREHOSE_MODEL_SOURCE})
    endif(MSVC)
endif()

set(FIREHOSE_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/"
)

add_library(${PROJECT_NAME} ${LIBTYPE} ${FIREHOSE_SRC})
add_library(AWS::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set_compiler_flags(${PROJECT_NAME})
set_compiler_warnings(${PROJECT_NAME})

if(USE_WINDOWS_DLL_SEMANTICS AND BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "AWS_FIREHOSE_EXPORTS")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PLATFORM_DEP_LIBS} ${PROJECT_LIBS})


setup_install()

install (FILES ${AWS_FIREHOSE_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/aws/firehose)
install (FILES ${AWS_FIREHOSE_MODEL_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/aws/firehose/model)

if(PLATFORM_WINDOWS AND MSVC)
    file(GLOB NUGET_FILES "nuget/${PROJECT_NAME}.*.autopkg")
    install (FILES ${NUGET_FILES} DESTINATION nuget)
endif()

do_packaging()


