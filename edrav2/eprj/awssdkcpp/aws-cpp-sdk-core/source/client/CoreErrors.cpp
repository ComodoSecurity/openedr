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

#include <aws/core/client/AWSError.h>
#include <aws/core/client/CoreErrors.h>
#include <aws/core/utils/HashingUtils.h>

using namespace Aws::Client;
using namespace Aws::Utils;
using namespace Aws::Http;

//we can't use a static map here due to memory allocation ordering. 
//instead we compute the hash of these strings to avoid so many string compares.
static const int INCOMPLETE_SIGNATURE_EXCEPTION_HASH = HashingUtils::HashString("IncompleteSignatureException");
static const int INCOMPLETE_SIGNATURE_HASH = HashingUtils::HashString("IncompleteSignature");
static const int INVALID_SIGNATURE_EXCEPTION_HASH = HashingUtils::HashString("InvalidSignatureException");
static const int INVALID_SIGNATURE_HASH = HashingUtils::HashString("InvalidSignature");
static const int INTERNAL_FAILURE_HASH = HashingUtils::HashString("InternalFailure");
static const int INTERNAL_FAILURE_EXCEPTION_HASH = HashingUtils::HashString("InternalFailureException");
static const int INTERNAL_SERVER_ERROR_HASH = HashingUtils::HashString("InternalServerError");
static const int INTERNAL_ERROR_HASH = HashingUtils::HashString("InternalError");
static const int INVALID_ACTION_EXCEPTION_HASH = HashingUtils::HashString("InvalidActionException");
static const int INVALID_ACTION_HASH = HashingUtils::HashString("InvalidAction");
static const int INVALID_CLIENT_TOKEN_ID_HASH = HashingUtils::HashString("InvalidClientTokenId");
static const int INVALID_CLIENT_TOKEN_ID_EXCEPTION_HASH = HashingUtils::HashString("InvalidClientTokenIdException");
static const int INVALID_PARAMETER_COMBINATION_EXCEPTION_HASH = HashingUtils::HashString("InvalidParameterCombinationException");
static const int INVALID_PARAMETER_COMBINATION_HASH = HashingUtils::HashString("InvalidParameterCombination");
static const int INVALID_PARAMETER_VALUE_EXCEPTION_HASH = HashingUtils::HashString("InvalidParameterValueException");
static const int INVALID_PARAMETER_VALUE_HASH = HashingUtils::HashString("InvalidParameterValue");
static const int INVALID_QUERY_PARAMETER_HASH = HashingUtils::HashString("InvalidQueryParameter");
static const int INVALID_QUERY_PARAMETER_EXCEPTION_HASH = HashingUtils::HashString("InvalidQueryParameterException");
static const int MALFORMED_QUERY_STRING_HASH = HashingUtils::HashString("MalformedQueryString");
static const int MISSING_ACTION_HASH = HashingUtils::HashString("MissingAction");
static const int MISSING_ACTION_EXCEPTION_HASH = HashingUtils::HashString("MissingActionException");
static const int MALFORMED_QUERY_STRING_EXCEPTION_HASH = HashingUtils::HashString("MalformedQueryStringException");
static const int MISSING_AUTHENTICATION_TOKEN_HASH = HashingUtils::HashString("MissingAuthenticationToken");
static const int MISSING_AUTHENTICATION_TOKEN_EXCEPTION_HASH = HashingUtils::HashString("MissingAuthenticationTokenException");
static const int MISSING_PARAMETER_EXCEPTION_HASH = HashingUtils::HashString("MissingParameterException");
static const int MISSING_PARAMETER_HASH = HashingUtils::HashString("MissingParameter");
static const int OPT_IN_REQUIRED_HASH = HashingUtils::HashString("OptInRequired");
static const int REQUEST_EXPIRED_HASH = HashingUtils::HashString("RequestExpired");
static const int REQUEST_EXPIRED_EXCEPTION_HASH = HashingUtils::HashString("RequestExpiredException");
static const int SERVICE_UNAVAILABLE_HASH = HashingUtils::HashString("ServiceUnavailable");
static const int SERVICE_UNAVAILABLE_EXCEPTION_HASH = HashingUtils::HashString("ServiceUnavailableException");
static const int SERVICE_UNAVAILABLE_ERROR_HASH = HashingUtils::HashString("ServiceUnavailableError");
static const int THROTTLING_HASH = HashingUtils::HashString("Throttling");
static const int THROTTLING_EXCEPTION_HASH = HashingUtils::HashString("ThrottlingException");
static const int THROTTLED_EXCEPTION_HASH = HashingUtils::HashString("ThrottledException");
static const int VALIDATION_ERROR_HASH = HashingUtils::HashString("ValidationError");
static const int VALIDATION_ERROR_EXCEPTION_HASH = HashingUtils::HashString("ValidationErrorException");
static const int VALIDATION_EXCEPTION_HASH = HashingUtils::HashString("ValidationException");
static const int ACCESS_DENIED_HASH = HashingUtils::HashString("AccessDenied");
static const int ACCESS_DENIED_EXCEPTION_HASH = HashingUtils::HashString("AccessDeniedException");
static const int RESOURCE_NOT_FOUND_HASH = HashingUtils::HashString("ResourceNotFound");
static const int RESOURCE_NOT_FOUND_EXCEPTION_HASH = HashingUtils::HashString("ResourceNotFoundException");
static const int UNRECOGNIZED_CLIENT_HASH = HashingUtils::HashString("UnrecognizedClient");
static const int UNRECOGNIZED_CLIENT_EXCEPTION_HASH = HashingUtils::HashString("UnrecognizedClientException");
static const int SLOW_DOWN_HASH = HashingUtils::HashString("SlowDown");
static const int SLOW_DOWN_EXCEPTION_HASH = HashingUtils::HashString("SlowDownException");
static const int SIGNATURE_DOES_NOT_MATCH_HASH = HashingUtils::HashString("SignatureDoesNotMatch");
static const int SIGNATURE_DOES_NOT_MATCH_EXCEPTION_HASH = HashingUtils::HashString("SignatureDoesNotMatchException");
static const int INVALID_ACCESS_KEY_ID_HASH = HashingUtils::HashString("InvalidAccessKeyId");
static const int INVALID_ACCESS_KEY_ID_EXCEPTION_HASH = HashingUtils::HashString("InvalidAccessKeyIdException");
static const int REQUEST_TIME_TOO_SKEWED_HASH = HashingUtils::HashString("RequestTimeTooSkewed");
static const int REQUEST_TIME_TOO_SKEWED_EXCEPTION_HASH = HashingUtils::HashString("RequestTimeTooSkewedException");
static const int REQUEST_TIMEOUT_HASH = HashingUtils::HashString("RequestTimeout");
static const int REQUEST_TIMEOUT_EXCEPTION_HASH = HashingUtils::HashString("RequestTimeoutException");

AWSError<CoreErrors> CoreErrorsMapper::GetErrorForName(const char* errorName)
{
  int errorHash = HashingUtils::HashString(errorName);

  if (errorHash == INCOMPLETE_SIGNATURE_HASH || errorHash == INCOMPLETE_SIGNATURE_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::INCOMPLETE_SIGNATURE, false);
  }
  else if (errorHash == INVALID_SIGNATURE_EXCEPTION_HASH || errorHash == INVALID_SIGNATURE_HASH)
  {
      return AWSError<CoreErrors>(CoreErrors::INVALID_SIGNATURE, false);
  }
  else if (errorHash ==  INTERNAL_FAILURE_EXCEPTION_HASH || errorHash == INTERNAL_FAILURE_HASH || errorHash == INTERNAL_SERVER_ERROR_HASH || errorHash == INTERNAL_ERROR_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::INTERNAL_FAILURE, true);
  }
  else if (errorHash == INVALID_ACTION_EXCEPTION_HASH || errorHash == INVALID_ACTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::INVALID_ACTION, false);
  }
  else if (errorHash == INVALID_CLIENT_TOKEN_ID_HASH || errorHash == INVALID_CLIENT_TOKEN_ID_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::INVALID_CLIENT_TOKEN_ID, false);
  }
  else if (errorHash == INVALID_PARAMETER_COMBINATION_EXCEPTION_HASH || errorHash == INVALID_PARAMETER_COMBINATION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::INVALID_PARAMETER_COMBINATION, false);
  }
  else if (errorHash == INVALID_PARAMETER_VALUE_EXCEPTION_HASH || errorHash == INVALID_PARAMETER_VALUE_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::INVALID_PARAMETER_VALUE, false);
  }
  else if (errorHash == INVALID_QUERY_PARAMETER_HASH || errorHash == INVALID_QUERY_PARAMETER_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::INVALID_QUERY_PARAMETER, false);
  }
  else if (errorHash == MALFORMED_QUERY_STRING_HASH || errorHash == MALFORMED_QUERY_STRING_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::MALFORMED_QUERY_STRING, false);
  }
  else if (errorHash == MISSING_ACTION_EXCEPTION_HASH || errorHash == MISSING_ACTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::MISSING_ACTION, false);
  }
  else if (errorHash == MISSING_AUTHENTICATION_TOKEN_HASH || errorHash == MISSING_AUTHENTICATION_TOKEN_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::MISSING_AUTHENTICATION_TOKEN, false);
  }
  else if (errorHash == MISSING_PARAMETER_EXCEPTION_HASH || errorHash == MISSING_PARAMETER_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::MISSING_PARAMETER, false);
  }
  else if (errorHash == OPT_IN_REQUIRED_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::OPT_IN_REQUIRED, false);
  }
  else if (errorHash == REQUEST_EXPIRED_HASH || errorHash == REQUEST_EXPIRED_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::REQUEST_EXPIRED, true);
  }
  else if (errorHash == SERVICE_UNAVAILABLE_HASH || errorHash == SERVICE_UNAVAILABLE_EXCEPTION_HASH || errorHash == SERVICE_UNAVAILABLE_ERROR_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::SERVICE_UNAVAILABLE, true);
  }
  else if (errorHash == THROTTLING_HASH || errorHash == THROTTLING_EXCEPTION_HASH || errorHash == THROTTLED_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::THROTTLING, true);
  }
  else if (errorHash == VALIDATION_ERROR_HASH || errorHash == VALIDATION_ERROR_EXCEPTION_HASH || errorHash == VALIDATION_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::VALIDATION, false);
  }
  else if (errorHash == ACCESS_DENIED_HASH || errorHash == ACCESS_DENIED_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::ACCESS_DENIED, false);
  }
  else if (errorHash == RESOURCE_NOT_FOUND_HASH || errorHash == RESOURCE_NOT_FOUND_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::RESOURCE_NOT_FOUND, false);
  }
  else if (errorHash == UNRECOGNIZED_CLIENT_HASH || errorHash == UNRECOGNIZED_CLIENT_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::UNRECOGNIZED_CLIENT, false);
  }
  else if (errorHash == SLOW_DOWN_HASH || errorHash == SLOW_DOWN_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::SLOW_DOWN, true);
  }
  else if (errorHash == SIGNATURE_DOES_NOT_MATCH_HASH || errorHash == SIGNATURE_DOES_NOT_MATCH_EXCEPTION_HASH)
  {
      return AWSError<CoreErrors>(CoreErrors::SIGNATURE_DOES_NOT_MATCH, false);
  }
  else if (errorHash == INVALID_ACCESS_KEY_ID_HASH || errorHash == INVALID_ACCESS_KEY_ID_EXCEPTION_HASH)
  {
      return AWSError<CoreErrors>(CoreErrors::INVALID_ACCESS_KEY_ID, false);
  }
  else if (errorHash == REQUEST_TIME_TOO_SKEWED_HASH || errorHash == REQUEST_TIME_TOO_SKEWED_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::REQUEST_TIME_TOO_SKEWED, true);
  }
  else if (errorHash == REQUEST_TIMEOUT_EXCEPTION_HASH || errorHash == REQUEST_TIMEOUT_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::REQUEST_TIMEOUT, true);
  }

  return AWSError<CoreErrors>(CoreErrors::UNKNOWN, false);
}

AWS_CORE_API AWSError<CoreErrors> CoreErrorsMapper::GetErrorForHttpResponseCode(HttpResponseCode code)
{
    // best effort attempt to map HTTP response codes to CoreErrors
    bool retryable = IsRetryableHttpResponseCode(code);
    switch(code)
    {
        case HttpResponseCode::UNAUTHORIZED:
        case HttpResponseCode::FORBIDDEN:
            return AWSError<CoreErrors>(CoreErrors::ACCESS_DENIED, retryable);
        case HttpResponseCode::NOT_FOUND:
            return AWSError<CoreErrors>(CoreErrors::RESOURCE_NOT_FOUND, retryable);
        case HttpResponseCode::TOO_MANY_REQUESTS:
            return AWSError<CoreErrors>(CoreErrors::SLOW_DOWN, retryable);
        case HttpResponseCode::INTERNAL_SERVER_ERROR:
            return AWSError<CoreErrors>(CoreErrors::INTERNAL_FAILURE, retryable);
        case HttpResponseCode::BANDWIDTH_LIMIT_EXCEEDED:
            return AWSError<CoreErrors>(CoreErrors::THROTTLING, retryable);
        case HttpResponseCode::SERVICE_UNAVAILABLE:
            return AWSError<CoreErrors>(CoreErrors::SERVICE_UNAVAILABLE, retryable);
        case HttpResponseCode::REQUEST_TIMEOUT:
        case HttpResponseCode::AUTHENTICATION_TIMEOUT:
        case HttpResponseCode::LOGIN_TIMEOUT:
        case HttpResponseCode::GATEWAY_TIMEOUT:
        case HttpResponseCode::NETWORK_READ_TIMEOUT:
        case HttpResponseCode::NETWORK_CONNECT_TIMEOUT:
            return AWSError<CoreErrors>(CoreErrors::REQUEST_TIMEOUT, retryable);
        default:
            int codeValue = static_cast<int>(code);
            return AWSError<CoreErrors>(CoreErrors::UNKNOWN, codeValue >= 500 && codeValue < 600);
    }
}
