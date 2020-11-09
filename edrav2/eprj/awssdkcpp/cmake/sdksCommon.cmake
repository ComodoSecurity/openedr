#
# Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
# 
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
# 
#  http://aws.amazon.com/apache2.0
# 
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.
#

# helper function that that gives primitive map functionality by treating a colon as the key-value separator, while the list semi-colon separates pairs
# to use, pass the list-map in as a third parameter (see helper functions below)
function(get_map_element KEY VALUE_VAR)
    foreach(ELEMENT_PAIR ${ARGN})
        STRING(REGEX REPLACE "([^:]+):.*" "\\1" KEY_RESULT ${ELEMENT_PAIR})
        if(${KEY_RESULT} STREQUAL ${KEY} )
            STRING(REGEX REPLACE "[^:]+:(.*)" "\\1" VALUE_RESULT ${ELEMENT_PAIR})
            set(${VALUE_VAR} "${VALUE_RESULT}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${VALUE_VAR} "" PARENT_SCOPE)
endfunction(get_map_element)

# a bunch of key-value retrieval functions for the list-maps we defined above
function(get_c2j_date_for_service SERVICE_NAME C2J_DATE_VAR)
    get_map_element(${SERVICE_NAME} TEMP_VAR ${C2J_LIST})
    set(${C2J_DATE_VAR} "${TEMP_VAR}" PARENT_SCOPE)
endfunction()

function(get_c2j_name_for_service SERVICE_NAME C2J_NAME_VAR)
    get_map_element(${SERVICE_NAME} TEMP_VAR ${C2J_SPECIAL_NAME_LIST})
    if(TEMP_VAR)
        set(${C2J_NAME_VAR} "${TEMP_VAR}" PARENT_SCOPE)
    else()
        set(${C2J_NAME_VAR} "${SERVICE_NAME}" PARENT_SCOPE)
    endif()
endfunction()

function(get_test_projects_for_service SERVICE_NAME TEST_PROJECT_NAME_VAR)
    get_map_element(${SERVICE_NAME} TEMP_VAR ${SDK_TEST_PROJECT_LIST})
    set(${TEST_PROJECT_NAME_VAR} "${TEMP_VAR}" PARENT_SCOPE)
endfunction()

function(get_dependencies_for_sdk PROJECT_NAME DEPENDENCY_LIST_VAR)
    get_map_element(${PROJECT_NAME} TEMP_VAR ${SDK_DEPENDENCY_LIST})
    # "core" is the default dependency for every sdk. 
    # Since we removed the hand-written C2J_LIST and instead auto generating it based on models,
    # and location of models may not exist or incorrect when SDK is installed and then source has been deleted by customers.
    # we end up getting an incomplete C2J_LIST when customers call find_package(AWSSDK). But C2J_LIST is only used in customers code for dependencies completing.
    set(${DEPENDENCY_LIST_VAR} "${TEMP_VAR},core" PARENT_SCOPE)
endfunction()

function(get_dependencies_for_test TEST_NAME DEPENDENCY_LIST_VAR)
    get_map_element(${TEST_NAME} TEMP_VAR ${TEST_DEPENDENCY_LIST})
    set(${DEPENDENCY_LIST_VAR} "${TEMP_VAR}" PARENT_SCOPE)
endfunction()

# returns all sdks, including itself, that depend on the supplied sdk
# this is kind of a reverse function of get_dependencies_for_sdk
function(get_sdks_depending_on SDK_NAME DEPENDING_SDKS_VAR)
    set(TEMP_SDK_LIST "${SDK_NAME}")
    foreach(SDK_DEP ${SDK_DEPENDENCY_LIST})
        STRING(REGEX REPLACE "([^:]+):.*" "\\1" KEY_RESULT ${SDK_DEP})
        STRING(REGEX REPLACE "[^:]+:(.*)" "\\1" VALUE_RESULT ${SDK_DEP})
        STRING(REPLACE "," ";" LIST_RESULT ${VALUE_RESULT})
        list(FIND LIST_RESULT ${SDK_NAME} FIND_INDEX)
        if(FIND_INDEX GREATER -1)
            list(APPEND TEMP_SDK_LIST ${KEY_RESULT})
        endif()
    endforeach()

    set(${DEPENDING_SDKS_VAR} "${TEMP_SDK_LIST}" PARENT_SCOPE)
endfunction()

# function that automatically picks up models from <sdkrootdir>/code-generation/api-descriptions/ directory and build
# C2J_LIST needed for generation, services have multiple models will use the latest model (decided by model files' date)
# services have the name format abc.def.ghi will be renamed to ghi-def-abc (dot will not be accepted as Windows directory name ) 
# and put into C2J_SPECIAL_NAME_LIST, but rumtime.lex will be renamed to lex based on historical reason.
function(build_sdk_list)
    file(GLOB ALL_MODEL_FILES "${CMAKE_CURRENT_SOURCE_DIR}/code-generation/api-descriptions/*-[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9].normal.json")
    foreach(model IN LISTS ALL_MODEL_FILES)
        get_filename_component(modelName "${model}" NAME)
        STRING(REGEX MATCH "([0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9])" date "${modelName}")
        STRING(REGEX REPLACE "-[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9].normal.json" "" svc "${modelName}")
        #special svc name conversion, e.g: runtime.lex->lex; abc.def.ghi->ghi-def-abc
        if ("${svc}" STREQUAL "runtime.lex")
            LIST(APPEND C2J_SPECIAL_NAME_LIST "lex:runtime.lex")
            set(svc "lex")
        elseif ("${svc}" STREQUAL "transfer")
            LIST(APPEND C2J_SPECIAL_NAME_LIST "awstransfer:transfer")
            set(svc "awstransfer")
        elseif("${svc}" MATCHES "\\.")
            string(REPLACE "." ";" nameParts ${svc})
            LIST(REVERSE nameParts)
            string(REPLACE ";" "-" tmpSvc "${nameParts}")
            LIST(APPEND C2J_SPECIAL_NAME_LIST "${tmpSvc}:${svc}")
            set(svc "${tmpSvc}")
        elseif("${svc}" STREQUAL "ec2")
            if(PLATFORM_ANDROID AND CMAKE_HOST_WIN32)  # ec2 isn't building for android on windows atm due to an internal compiler error, TODO: investigate further
                continue()
            endif()
        endif()
        get_map_element(${svc} existingDate ${C2J_LIST})
        if ("${existingDate}" STREQUAL "")
            LIST(APPEND C2J_LIST "${svc}:${date}")
        elseif(${existingDate} STRLESS ${date})
            LIST(REMOVE_ITEM C2J_LIST "${svc}:${existingDate}")
            LIST(APPEND C2J_LIST "${svc}:${date}")
        endif()
    endforeach()
    set(C2J_LIST "${C2J_LIST}" PARENT_SCOPE)
    set(C2J_SPECIAL_NAME_LIST "${C2J_SPECIAL_NAME_LIST}" PARENT_SCOPE)
endfunction()


set(HIGH_LEVEL_SDK_LIST "")
list(APPEND HIGH_LEVEL_SDK_LIST "access-management") 
list(APPEND HIGH_LEVEL_SDK_LIST "identity-management") 
list(APPEND HIGH_LEVEL_SDK_LIST "queues") 
list(APPEND HIGH_LEVEL_SDK_LIST "transfer") 
list(APPEND HIGH_LEVEL_SDK_LIST "s3-encryption") 
list(APPEND HIGH_LEVEL_SDK_LIST "text-to-speech") 

set(SDK_TEST_PROJECT_LIST "")
list(APPEND SDK_TEST_PROJECT_LIST "cognito-identity:aws-cpp-sdk-cognitoidentity-integration-tests")
list(APPEND SDK_TEST_PROJECT_LIST "dynamodb:aws-cpp-sdk-dynamodb-integration-tests")
list(APPEND SDK_TEST_PROJECT_LIST "identity-management:aws-cpp-sdk-identity-management-tests")
list(APPEND SDK_TEST_PROJECT_LIST "lambda:aws-cpp-sdk-lambda-integration-tests")
list(APPEND SDK_TEST_PROJECT_LIST "s3:aws-cpp-sdk-s3-integration-tests")
list(APPEND SDK_TEST_PROJECT_LIST "s3control:aws-cpp-sdk-s3control-integration-tests")
list(APPEND SDK_TEST_PROJECT_LIST "redshift:aws-cpp-sdk-redshift-integration-tests")
list(APPEND SDK_TEST_PROJECT_LIST "sqs:aws-cpp-sdk-sqs-integration-tests")
list(APPEND SDK_TEST_PROJECT_LIST "transfer:aws-cpp-sdk-transfer-tests")
list(APPEND SDK_TEST_PROJECT_LIST "s3-encryption:aws-cpp-sdk-s3-encryption-tests,aws-cpp-sdk-s3-encryption-integration-tests")
list(APPEND SDK_TEST_PROJECT_LIST "ec2:aws-cpp-sdk-ec2-integration-tests")
list(APPEND SDK_TEST_PROJECT_LIST "core:aws-cpp-sdk-core-tests")
list(APPEND SDK_TEST_PROJECT_LIST "text-to-speech:aws-cpp-sdk-text-to-speech-tests,aws-cpp-sdk-polly-sample")

set(SDK_DEPENDENCY_LIST "")
list(APPEND SDK_DEPENDENCY_LIST "access-management:iam,cognito-identity,core")
list(APPEND SDK_DEPENDENCY_LIST "identity-management:cognito-identity,sts,core")
list(APPEND SDK_DEPENDENCY_LIST "queues:sqs,core")
list(APPEND SDK_DEPENDENCY_LIST "transfer:s3,core")
list(APPEND SDK_DEPENDENCY_LIST "s3-encryption:s3,kms,core")
list(APPEND SDK_DEPENDENCY_LIST "text-to-speech:polly,core")

set(TEST_DEPENDENCY_LIST "")
list(APPEND TEST_DEPENDENCY_LIST "cognito-identity:access-management,iam,core")
list(APPEND TEST_DEPENDENCY_LIST "identity-management:cognito-identity,sts,core")
list(APPEND TEST_DEPENDENCY_LIST "lambda:access-management,cognito-identity,iam,kinesis,core")
list(APPEND TEST_DEPENDENCY_LIST "sqs:access-management,cognito-identity,iam,core")
list(APPEND TEST_DEPENDENCY_LIST "transfer:s3,core")
list(APPEND TEST_DEPENDENCY_LIST "s3-encryption:s3,kms,core")
list(APPEND TEST_DEPENDENCY_LIST "s3control:access-management,cognito-identity,iam,core")
list(APPEND TEST_DEPENDENCY_LIST "text-to-speech:polly,core")

build_sdk_list()

# make a list of the generated clients
set(GENERATED_SERVICE_LIST "")
foreach(GENERATED_C2J_SERVICE IN LISTS C2J_LIST)
    STRING(REGEX REPLACE "([^:]+):.*" "\\1" SERVICE_RESULT ${GENERATED_C2J_SERVICE})
    list(APPEND GENERATED_SERVICE_LIST ${SERVICE_RESULT})
    list(APPEND SDK_DEPENDENCY_LIST "${SERVICE_RESULT}:core")
endforeach()
