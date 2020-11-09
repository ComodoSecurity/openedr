include(sdksCommon)

set(SDK_DEPENDENCY_BUILD_LIST "")

if(REGENERATE_CLIENTS)
    message(STATUS "Checking for SDK generation requirements")
    include(FindJava)

    if(NOT Java_JAVA_EXECUTABLE OR NOT Java_JAVAC_EXECUTABLE)
        message(FATAL_ERROR "Generating SDK clients requires a jdk 1.8 installation")
    endif()

    find_program(MAVEN_PROGRAM mvn)
    if(NOT MAVEN_PROGRAM)
        message(FATAL_ERROR "Generating SDK clients requires a maven installation")
    endif()
endif()


if(BUILD_ONLY)
    set(SDK_BUILD_LIST ${BUILD_ONLY})
    foreach(TARGET IN LISTS BUILD_ONLY)
        message(STATUS "Considering ${TARGET}")
        get_dependencies_for_sdk(${TARGET} DEPENDENCY_LIST)
        if(DEPENDENCY_LIST)
            STRING(REPLACE "," ";" LIST_RESULT ${DEPENDENCY_LIST})
            foreach(DEPENDENCY IN LISTS LIST_RESULT)
                list(APPEND SDK_DEPENDENCY_BUILD_LIST ${DEPENDENCY})
            endforeach()
        endif()

        get_dependencies_for_test(${TARGET} DEPENDENCY_LIST)
        if(DEPENDENCY_LIST)
            STRING(REPLACE "," ";" LIST_RESULT ${DEPENDENCY_LIST})
            foreach(DEPENDENCY IN LISTS LIST_RESULT)
                list(APPEND SDK_DEPENDENCY_BUILD_LIST ${DEPENDENCY})
            endforeach()
        endif()
    endforeach()
    LIST(REMOVE_DUPLICATES SDK_BUILD_LIST)
    LIST(REMOVE_DUPLICATES SDK_DEPENDENCY_BUILD_LIST)  
else()
    set(TEMP_SDK_BUILD_LIST ${GENERATED_SERVICE_LIST})
    list(APPEND TEMP_SDK_BUILD_LIST "core")

    list(APPEND TEMP_SDK_BUILD_LIST ${HIGH_LEVEL_SDK_LIST})

    set(SDK_BUILD_LIST ${TEMP_SDK_BUILD_LIST})

    # remove any missing targets from the build list, factoring in dependencies appropriately
    foreach(SDK IN LISTS TEMP_SDK_BUILD_LIST)
        set(REMOVE_SDK 0)

        set(SDK_DIR "aws-cpp-sdk-${SDK}")
        
        if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SDK_DIR}" AND NOT REGENERATE_CLIENTS)
            set(REMOVE_SDK 1)
        endif()

        if(REMOVE_SDK)
            get_sdks_depending_on(${SDK} REMOVE_LIST)
            foreach(REMOVE_SDK IN LISTS REMOVE_LIST)
                list(REMOVE_ITEM SDK_BUILD_LIST ${REMOVE_SDK})
            endforeach()
        endif()
    endforeach()
endif()

# SDK_BUILD_LIST is now a list of present SDKs that can be processed unconditionally
if(ADD_CUSTOM_CLIENTS OR REGENERATE_CLIENTS)
    execute_process(
        COMMAND ${PYTHON_CMD} scripts/generate_sdks.py --prepareTools
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

if(REGENERATE_CLIENTS)
    message(STATUS "Regenerating clients that have been selected for build.")
    set(MERGED_BUILD_LIST ${SDK_BUILD_LIST})
    list(APPEND MERGED_BUILD_LIST ${SDK_DEPENDENCY_BUILD_LIST})
    LIST(REMOVE_DUPLICATES MERGED_BUILD_LIST)
    
    foreach(SDK IN LISTS MERGED_BUILD_LIST)
        get_c2j_date_for_service(${SDK} C2J_DATE)
        get_c2j_name_for_service(${SDK} C2J_NAME)
        set(SDK_C2J_FILE "${CMAKE_CURRENT_SOURCE_DIR}/code-generation/api-descriptions/${C2J_NAME}-${C2J_DATE}.normal.json")
               
        if(EXISTS ${SDK_C2J_FILE})
            message(STATUS "Clearing existing directory for ${SDK} to prepare for generation.")
            file(REMOVE_RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/aws-cpp-sdk-${SDK}")
            
            execute_process(
                COMMAND ${PYTHON_CMD} scripts/generate_sdks.py --serviceName ${SDK} --apiVersion ${C2J_DATE} --outputLocation ./
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
            message(STATUS "Generated service: ${SDK}, version: ${C2J_DATE}")
        else()
           message(STATUS "Directory for ${SDK} is either missing a service definition, is a custom client, or it is not a generated client. Skipping.")
        endif()
    endforeach()

    foreach(SDK IN LISTS HIGH_LEVEL_SDK_LIST)
        if (BUILD_ONLY) 
            list(FIND BUILD_ONLY ${SDK} INDEX)
            # when BUILD_ONLY is set only update high level sdks specified in it.
            if ((INDEX GREATER 0) OR (INDEX EQUAL 0))
                execute_process(
                    COMMAND ${PYTHON_CMD} scripts/prepare_regenerate_high_level_sdks.py --highLevelSdkName ${SDK}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                )
            endif()
        else()
            execute_process(
                COMMAND ${PYTHON_CMD} scripts/prepare_regenerate_high_level_sdks.py --highLevelSdkName ${SDK}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
        endif()
    endforeach()

endif()

#at this point, if user has specified some customized clients, generate them and add them to the build here.
foreach(custom_client ${ADD_CUSTOM_CLIENTS})
    message(STATUS "${custom_client}")
    STRING(REGEX MATCHALL "serviceName=(.*), ?version=(.*)" CLIENT_MATCHES "${custom_client}")
    set(C_LEN 0)
    LIST(LENGTH CLIENT_MATCHES C_LEN)
    if(CMAKE_MATCH_COUNT GREATER 0)
        set(C_SERVICE_NAME ${CMAKE_MATCH_1})
        set(C_VERSION ${CMAKE_MATCH_2})
        
        set(SDK_C2J_FILE "${CMAKE_CURRENT_SOURCE_DIR}/code-generation/api-descriptions/${C_SERVICE_NAME}-${C_VERSION}.normal.json")        
        if(NOT (EXISTS ${SDK_C2J_FILE}))
            message(FATAL_ERROR "${C_SERVICE_NAME} is required for build, but C2J file '${SDK_C2J_FILE}' is missing!")
        endif()
        
        message(STATUS "Clearing existing directory for ${C_SERVICE_NAME} to prepare for generation.")
        file(REMOVE_RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/aws-cpp-sdk-${C_SERVICE_NAME}")
        message(STATUS "generating client for ${C_SERVICE_NAME} version ${C_VERSION}")
        execute_process(
            COMMAND ${PYTHON_CMD} scripts/generate_sdks.py --serviceName ${C_SERVICE_NAME} --apiVersion ${C_VERSION} --outputLocation ./
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
        LIST(APPEND SDK_BUILD_LIST ${C_SERVICE_NAME})        
    endif()
endforeach(custom_client)

# when building a fixed target set, missing SDKs are an error
# when building "everything", we forgive missing SDKs; should this become an option instead?
if(BUILD_ONLY)    
    # make sure all the sdks/c2js are present; a missing sdk-directory or c2j file is a build error when building a manually-specified set
    foreach(SDK IN LISTS SDK_BUILD_LIST)
        set(SDK_DIR "aws-cpp-sdk-${SDK}")

        if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SDK_DIR}")
            message(FATAL_ERROR "${SDK} is required for build, but ${SDK_DIR} directory is missing!")
        endif()        
    endforeach()

    set(TEMP_SDK_DEPENDENCY_BUILD_LIST ${SDK_DEPENDENCY_BUILD_LIST})
    foreach (SDK IN LISTS TEMP_SDK_DEPENDENCY_BUILD_LIST)
        list(FIND SDK_BUILD_LIST ${SDK} DEPENDENCY_INDEX)
        if(DEPENDENCY_INDEX LESS 0)
            # test dependencies should also be built from source instead of locating by calling find_package
            # which may cause version conflicts as well as double targeting built targets
            set(SDK_DIR "aws-cpp-sdk-${SDK}")
            if (NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SDK_DIR}")
                message(FATAL_ERROR "${SDK} is required for build, but ${SDK_DIR} directory is missing!")
            endif ()               
        else ()
            list(REMOVE_ITEM SDK_DEPENDENCY_BUILD_LIST ${SDK})
        endif ()
    endforeach ()

    foreach (SDK IN LISTS SDK_DEPENDENCY_BUILD_LIST)
        list(APPEND SDK_BUILD_LIST "${SDK}")
    endforeach()
endif()

LIST(REMOVE_DUPLICATES SDK_BUILD_LIST)
LIST(REMOVE_DUPLICATES SDK_DEPENDENCY_BUILD_LIST) 

function(add_sdks)
    LIST(APPEND EXPORTS "")
    foreach(SDK IN LISTS SDK_BUILD_LIST)
        set(SDK_DIR "aws-cpp-sdk-${SDK}")

        add_subdirectory("${SDK_DIR}")
        LIST(APPEND EXPORTS "${SDK_DIR}")
    endforeach()    

    #testing
    if(ENABLE_TESTING)
        add_subdirectory(testing-resources)

        # android-unified-tests includes all the tests in our code base, those tests related services may not be incldued in BUILD_ONLY,
        # means, those services will not be built, but will be tried to linked against with test targets, which will cause link time error.
        # See https://github.com/aws/aws-sdk-cpp/issues/786. We should only build android-unified-tests when doing a full build.
        if(PLATFORM_ANDROID AND NOT BUILD_SHARED_LIBS AND NOT BUILD_ONLY)
            add_subdirectory(android-unified-tests)
        else()
            foreach(SDK IN LISTS SDK_BUILD_LIST)
                get_test_projects_for_service(${SDK} TEST_PROJECTS)
                if(TEST_PROJECTS)
                    STRING(REPLACE "," ";" LIST_RESULT ${TEST_PROJECTS})
                    foreach(TEST_PROJECT IN LISTS LIST_RESULT)
                        if(TEST_PROJECT)                         
                            # before we add the test, make sure all of its dependencies are present
                            set(ADD_TEST 1)
                            get_dependencies_for_test(${SDK} DEPENDENCY_LIST)
                            if(DEPENDENCY_LIST)
                                STRING(REPLACE "," ";" LIST_RESULT ${DEPENDENCY_LIST})
                                foreach(DEPENDENCY IN LISTS LIST_RESULT)
                                    list(FIND SDK_BUILD_LIST ${DEPENDENCY} DEPENDENCY_INDEX)
                                    if(DEPENDENCY_INDEX LESS 0)
                                        message(STATUS "Removing test project ${TEST_PROJECT} due to missing dependency {$DEPENDENCY}")
                                        set(ADD_TEST 0)
                                    endif()
                                endforeach()
                            endif()

                            if(ADD_TEST)
                                add_subdirectory(${TEST_PROJECT})
                            endif()
                        endif()
                    endforeach()
                endif()
             endforeach()
        endif()
    endif()

    # the catch-all config needs to list all the targets in a dependency-sorted order
    include(dependencies)
    sort_links(EXPORTS)

    # make an everything config by just including all the individual configs
    file(WRITE ${CMAKE_BINARY_DIR}/aws-sdk-cpp-config.cmake "")
    foreach(EXPORT_TARGET IN LISTS EXPORTS)
        file(APPEND ${CMAKE_BINARY_DIR}/aws-sdk-cpp-config.cmake "include(\"\${CMAKE_CURRENT_LIST_DIR}/${EXPORT_TARGET}/${EXPORT_TARGET}-config.cmake\")\n")
    endforeach()
endfunction()
