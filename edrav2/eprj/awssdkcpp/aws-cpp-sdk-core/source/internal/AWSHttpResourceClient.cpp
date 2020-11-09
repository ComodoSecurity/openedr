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

#include <aws/core/internal/AWSHttpResourceClient.h>
#include <aws/core/client/DefaultRetryStrategy.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/client/AWSError.h>
#include <aws/core/client/CoreErrors.h>

#include <sstream>

using namespace Aws::Utils;
using namespace Aws::Utils::Logging;
using namespace Aws::Http;
using namespace Aws::Client;
using namespace Aws::Internal;

static const char* EC2_SECURITY_CREDENTIALS_RESOURCE = "/latest/meta-data/iam/security-credentials";
static const char* EC2_REGION_RESOURCE = "/latest/meta-data/placement/availability-zone";

static const char* RESOURCE_CLIENT_CONFIGURATION_ALLOCATION_TAG = "AWSHttpResourceClient";
static const char* EC2_METADATA_CLIENT_LOG_TAG = "EC2MetadataClient";
static const char* ECS_CREDENTIALS_CLIENT_LOG_TAG = "ECSCredentialsClient";


namespace Aws
{
    namespace Client
    {
        Aws::String ComputeUserAgentString();
    }
}

static ClientConfiguration MakeDefaultHttpResourceClientConfiguration(const char *logtag)
{
    ClientConfiguration res;

    res.maxConnections = 2;
    res.scheme = Scheme::HTTP;

#if defined(WIN32) && defined(BYPASS_DEFAULT_PROXY)
    // For security reasons, we must bypass any proxy settings when fetching sensitive information, for example
    // user credentials. On Windows, IXMLHttpRequest2 does not support bypasing proxy settings, therefore,
    // we force using WinHTTP client. On POSIX systems, CURL is set to bypass proxy settings by default.
    res.httpLibOverride = TransferLibType::WIN_HTTP_CLIENT;
    AWS_LOGSTREAM_INFO(logtag, "Overriding the current HTTP client to WinHTTP to bypass proxy settings.");
#else
    (void) logtag;  // To disable warning about unused variable
#endif
    // Explicitly set the proxy settings to empty/zero to avoid relying on defaults that could potentially change
    // in the future.
    res.proxyHost = "";
    res.proxyUserName = "";
    res.proxyPassword = "";
    res.proxyPort = 0;

    // EC2MetatadaService throttles by delaying the response so the service client should set a large read timeout.
    // EC2MetatadaService delay is in order of seconds so it only make sense to retry after a couple of seconds.
    res.connectTimeoutMs = 1000;
    res.requestTimeoutMs = 5000;
    res.retryStrategy = Aws::MakeShared<DefaultRetryStrategy>(RESOURCE_CLIENT_CONFIGURATION_ALLOCATION_TAG, 4, 1000);

    return res;
}

AWSHttpResourceClient::AWSHttpResourceClient(const Aws::Client::ClientConfiguration& clientConfiguration, const char* logtag)
: m_logtag(logtag), m_retryStrategy(clientConfiguration.retryStrategy), m_httpClient(nullptr)
{
    AWS_LOGSTREAM_INFO(m_logtag.c_str(),
                       "Creating AWSHttpResourceClient with max connections"
                        << clientConfiguration.maxConnections
                        << " and scheme "
                        << SchemeMapper::ToString(clientConfiguration.scheme));

    m_httpClient = CreateHttpClient(clientConfiguration);
}

AWSHttpResourceClient::AWSHttpResourceClient(const char* logtag)
: AWSHttpResourceClient(MakeDefaultHttpResourceClientConfiguration(logtag), logtag)
{
}

AWSHttpResourceClient::~AWSHttpResourceClient()
{
}

Aws::String AWSHttpResourceClient::GetResource(const char* endpoint, const char* resource, const char* authToken) const
{
    Aws::StringStream ss;
    ss << endpoint << resource;
    AWS_LOGSTREAM_TRACE(m_logtag.c_str(), "Retrieving credentials from " << ss.str().c_str());

    for (long retries = 0;; retries++)
    {
        std::shared_ptr<HttpRequest> request(CreateHttpRequest(ss.str(), HttpMethod::HTTP_GET,
                    Aws::Utils::Stream::DefaultResponseStreamFactoryMethod));

        request->SetUserAgent(ComputeUserAgentString());

        if (authToken)
        {
            request->SetHeaderValue(Aws::Http::AWS_AUTHORIZATION_HEADER, authToken);
        }

        std::shared_ptr<HttpResponse> response(m_httpClient->MakeRequest(request));

        if (response && response->GetResponseCode() == HttpResponseCode::OK)
        {
            Aws::IStreamBufIterator eos;
            return Aws::String(Aws::IStreamBufIterator(response->GetResponseBody()), eos);
        }

        const Aws::Client::AWSError<Aws::Client::CoreErrors> error = [this, &response]() {
            if (!response)
            {
                AWS_LOGSTREAM_ERROR(m_logtag.c_str(), "Http request to retrieve credentials failed");
                return AWSError<CoreErrors>(CoreErrors::NETWORK_CONNECTION, true);  // Retriable
            }
            else
            {
                const auto responseCode = response->GetResponseCode();

                AWS_LOGSTREAM_ERROR(m_logtag.c_str(), "Http request to retrieve credentials failed with error code "
                                    << static_cast<int>(responseCode));
                return CoreErrorsMapper::GetErrorForHttpResponseCode(responseCode);
            }
        } ();

        if (!m_retryStrategy->ShouldRetry(error, retries))
        {
            AWS_LOGSTREAM_ERROR(m_logtag.c_str(), "Can not retrive resource " << resource);
            return {};
        }

        auto sleepMillis = m_retryStrategy->CalculateDelayBeforeNextRetry(error, retries);
        AWS_LOGSTREAM_WARN(m_logtag.c_str(), "Request failed, now waiting " << sleepMillis << " ms before attempting again.");
        m_httpClient->RetryRequestSleep(std::chrono::milliseconds(sleepMillis));
    }
}

EC2MetadataClient::EC2MetadataClient(const char* endpoint)
    : AWSHttpResourceClient(EC2_METADATA_CLIENT_LOG_TAG), m_endpoint(endpoint)
{
}

EC2MetadataClient::EC2MetadataClient(const Aws::Client::ClientConfiguration& clientConfiguration, const char* endpoint)
    : AWSHttpResourceClient(clientConfiguration, EC2_METADATA_CLIENT_LOG_TAG), m_endpoint(endpoint)
{
}

EC2MetadataClient::~EC2MetadataClient()
{

}

Aws::String EC2MetadataClient::GetResource(const char* resourcePath) const
{
    return GetResource(m_endpoint.c_str(), resourcePath, ""/*authToken*/);
}

Aws::String EC2MetadataClient::GetDefaultCredentials() const
{
    AWS_LOGSTREAM_TRACE(m_logtag.c_str(), 
            "Getting default credentials for ec2 instance");
    Aws::String credentialsString = GetResource(EC2_SECURITY_CREDENTIALS_RESOURCE);

    if (credentialsString.empty()) return {};
    
    Aws::String trimmedCredentialsString = StringUtils::Trim(credentialsString.c_str());
    Aws::Vector<Aws::String> securityCredentials = StringUtils::Split(trimmedCredentialsString, '\n');

    AWS_LOGSTREAM_DEBUG(m_logtag.c_str(),
            "Calling EC2MetatadaService resource, " << EC2_SECURITY_CREDENTIALS_RESOURCE
            << " returned credential string " << trimmedCredentialsString);

    if (securityCredentials.size() == 0)
    {
        AWS_LOGSTREAM_WARN(m_logtag.c_str(), 
                "Initial call to ec2Metadataservice to get credentials failed");
        return {};
    }

    Aws::StringStream ss;
    ss << EC2_SECURITY_CREDENTIALS_RESOURCE << "/" << securityCredentials[0];
    AWS_LOGSTREAM_DEBUG(m_logtag.c_str(), 
            "Calling EC2MetatadaService resource " << ss.str());
    return GetResource(ss.str().c_str());
}

Aws::String EC2MetadataClient::GetCurrentRegion() const
{
    AWS_LOGSTREAM_TRACE(m_logtag.c_str(), "Getting current region for ec2 instance");
    Aws::String azString = GetResource(EC2_REGION_RESOURCE);

    if (azString.empty())
    {
        AWS_LOGSTREAM_INFO(m_logtag.c_str() ,
                "Unable to pull region from instance metadata service ");
        return {};
    }
    
    Aws::String trimmedAZString = StringUtils::Trim(azString.c_str());
    AWS_LOGSTREAM_DEBUG(m_logtag.c_str(), "Calling EC2MetatadaService resource " 
            << EC2_REGION_RESOURCE << " , returned credential string " << trimmedAZString);

    Aws::String region;
    region.reserve(trimmedAZString.length());

    bool digitFound = false;
    for (auto character : trimmedAZString)
    {
        if(digitFound && !isdigit(character))
        {
            break;
        }
        if (isdigit(character))
        {
            digitFound = true;
        }

        region.append(1, character);
    }

    AWS_LOGSTREAM_INFO(m_logtag.c_str(), "Detected current region as " << region);
    return region;
}

ECSCredentialsClient::ECSCredentialsClient(const char* resourcePath, const char* endpoint, const char* token)
    : AWSHttpResourceClient(ECS_CREDENTIALS_CLIENT_LOG_TAG),
    m_resourcePath(resourcePath), m_endpoint(endpoint), m_token(token)
{
}

ECSCredentialsClient::ECSCredentialsClient(const Aws::Client::ClientConfiguration& clientConfiguration, const char* resourcePath, const char* endpoint, const char* token)
    : AWSHttpResourceClient(clientConfiguration, ECS_CREDENTIALS_CLIENT_LOG_TAG),
    m_resourcePath(resourcePath), m_endpoint(endpoint), m_token(token)
{
}

