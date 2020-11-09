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


#include <aws/core/auth/AWSCredentialsProvider.h>

#include <aws/core/config/AWSProfileConfigLoader.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/platform/FileSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/FileSystemUtils.h>

#include <cstdlib>
#include <fstream>
#include <string.h>
#include <climits>


using namespace Aws::Utils;
using namespace Aws::Utils::Logging;
using namespace Aws::Auth;
using namespace Aws::Internal;
using Aws::Utils::Threading::ReaderLockGuard;
using Aws::Utils::Threading::WriterLockGuard;

static const char ACCESS_KEY_ENV_VAR[] = "AWS_ACCESS_KEY_ID";
static const char SECRET_KEY_ENV_VAR[] = "AWS_SECRET_ACCESS_KEY";
static const char SESSION_TOKEN_ENV_VAR[] = "AWS_SESSION_TOKEN";
static const char DEFAULT_PROFILE[] = "default";
static const char AWS_PROFILE_ENV_VAR[] = "AWS_PROFILE";
static const char AWS_PROFILE_DEFAULT_ENV_VAR[] = "AWS_DEFAULT_PROFILE";

static const char AWS_CREDENTIAL_PROFILES_FILE[] = "AWS_SHARED_CREDENTIALS_FILE";

static const char PROFILE_DEFAULT_FILENAME[] = "credentials";
static const char CONFIG_FILENAME[] = "config";

#ifndef _WIN32
static const char PROFILE_DIRECTORY[] = "/.aws";
static const char DIRECTORY_JOIN[] = "/";

#else
    static const char PROFILE_DIRECTORY[] = "\\.aws";
    static const char DIRECTORY_JOIN[] = "\\";
#endif // _WIN32


static const int EXPIRATION_GRACE_PERIOD = 5 * 1000;

void AWSCredentialsProvider::Reload()
{
    m_lastLoadedMs = DateTime::Now().Millis();
}

bool AWSCredentialsProvider::IsTimeToRefresh(long reloadFrequency)
{
    if (DateTime::Now().Millis() - m_lastLoadedMs > reloadFrequency)
    {
        return true;
    }
    return false;
}


static const char* ENVIRONMENT_LOG_TAG = "EnvironmentAWSCredentialsProvider";


AWSCredentials EnvironmentAWSCredentialsProvider::GetAWSCredentials()
{
    auto accessKey = Aws::Environment::GetEnv(ACCESS_KEY_ENV_VAR);
    AWSCredentials credentials("", "", "");

    if (!accessKey.empty())
    {
        credentials.SetAWSAccessKeyId(accessKey);

        AWS_LOGSTREAM_DEBUG(ENVIRONMENT_LOG_TAG, "Found credential in environment with access key id " << accessKey);
        auto secretKey = Aws::Environment::GetEnv(SECRET_KEY_ENV_VAR);

        if (!secretKey.empty())
        {
            credentials.SetAWSSecretKey(secretKey);
            AWS_LOGSTREAM_INFO(ENVIRONMENT_LOG_TAG, "Found secret key");
        }

        auto sessionToken = Aws::Environment::GetEnv(SESSION_TOKEN_ENV_VAR);

        if(!sessionToken.empty())
        {
            credentials.SetSessionToken(sessionToken);
            AWS_LOGSTREAM_INFO(ENVIRONMENT_LOG_TAG, "Found sessionToken");
        }
    }

    return credentials;
}

static Aws::String GetBaseDirectory()
{
    return Aws::FileSystem::GetHomeDirectory();
}

Aws::String ProfileConfigFileAWSCredentialsProvider::GetConfigProfileFilename()
{
    return GetBaseDirectory() + PROFILE_DIRECTORY + DIRECTORY_JOIN + CONFIG_FILENAME;
}

Aws::String ProfileConfigFileAWSCredentialsProvider::GetCredentialsProfileFilename()
{
    auto profileFileNameFromVar = Aws::Environment::GetEnv(AWS_CREDENTIAL_PROFILES_FILE);

    if (!profileFileNameFromVar.empty())
    {
        return profileFileNameFromVar;
    }
    else
    {
        return GetBaseDirectory() + PROFILE_DIRECTORY + DIRECTORY_JOIN + PROFILE_DEFAULT_FILENAME;
    }
}

Aws::String ProfileConfigFileAWSCredentialsProvider::GetProfileDirectory()
{
    Aws::String profileFileName = GetCredentialsProfileFilename();
    auto lastSeparator = profileFileName.find_last_of(DIRECTORY_JOIN);
    if (lastSeparator != std::string::npos)
    {
        return profileFileName.substr(0, lastSeparator);
    }
    else
    {
        return {};
    }
}

static const char* PROFILE_LOG_TAG = "ProfileConfigFileAWSCredentialsProvider";


ProfileConfigFileAWSCredentialsProvider::ProfileConfigFileAWSCredentialsProvider(long refreshRateMs) :
        m_configFileLoader(Aws::MakeShared<Aws::Config::AWSConfigFileProfileConfigLoader>(PROFILE_LOG_TAG, GetConfigProfileFilename(), true)),
        m_credentialsFileLoader(Aws::MakeShared<Aws::Config::AWSConfigFileProfileConfigLoader>(PROFILE_LOG_TAG, GetCredentialsProfileFilename())),
        m_loadFrequencyMs(refreshRateMs)
{
    auto profileFromVar = Aws::Environment::GetEnv(AWS_PROFILE_DEFAULT_ENV_VAR);
    if (profileFromVar.empty())
    {
        profileFromVar = Aws::Environment::GetEnv(AWS_PROFILE_ENV_VAR);
    }

    if (!profileFromVar.empty())
    {
        m_profileToUse = profileFromVar;
    }
    else
    {
        m_profileToUse = DEFAULT_PROFILE;
    }

    AWS_LOGSTREAM_INFO(PROFILE_LOG_TAG, "Setting provider to read credentials from " <<  GetCredentialsProfileFilename() << " for credentials file"
                                      << " and " <<  GetConfigProfileFilename() << " for the config file "
                                      << ", for use with profile " << m_profileToUse);
}

ProfileConfigFileAWSCredentialsProvider::ProfileConfigFileAWSCredentialsProvider(const char* profile, long refreshRateMs) :
        m_profileToUse(profile),
        m_configFileLoader(Aws::MakeShared<Aws::Config::AWSConfigFileProfileConfigLoader>(PROFILE_LOG_TAG, GetConfigProfileFilename(), true)),
        m_credentialsFileLoader(Aws::MakeShared<Aws::Config::AWSConfigFileProfileConfigLoader>(PROFILE_LOG_TAG, GetCredentialsProfileFilename())),
        m_loadFrequencyMs(refreshRateMs)
{
    AWS_LOGSTREAM_INFO(PROFILE_LOG_TAG, "Setting provider to read credentials from " <<  GetCredentialsProfileFilename() << " for credentials file"
                                      << " and " <<  GetConfigProfileFilename() << " for the config file "
                                      << ", for use with profile " << m_profileToUse);
}

AWSCredentials ProfileConfigFileAWSCredentialsProvider::GetAWSCredentials()
{
    RefreshIfExpired();
    ReaderLockGuard guard(m_reloadLock);
    auto credsFileProfileIter = m_credentialsFileLoader->GetProfiles().find(m_profileToUse);

    if(credsFileProfileIter != m_credentialsFileLoader->GetProfiles().end())
    {
        return credsFileProfileIter->second.GetCredentials();
    }

    auto configFileProfileIter = m_configFileLoader->GetProfiles().find(m_profileToUse);
    if(configFileProfileIter != m_configFileLoader->GetProfiles().end())
    {
        return configFileProfileIter->second.GetCredentials();
    }

    return AWSCredentials();
}


void ProfileConfigFileAWSCredentialsProvider::Reload()
{
    if (!m_credentialsFileLoader->Load())
    {
        m_configFileLoader->Load();
    }
    AWSCredentialsProvider::Reload();
}

void ProfileConfigFileAWSCredentialsProvider::RefreshIfExpired()
{
    ReaderLockGuard guard(m_reloadLock);
    if (!IsTimeToRefresh(m_loadFrequencyMs))
    {
       return;
    }

    //fall-back to config file.
    guard.UpgradeToWriterLock();
    if (!IsTimeToRefresh(m_loadFrequencyMs)) // double-checked lock to avoid refreshing twice
    {
        return;
    }

    Reload();
}

static const char* INSTANCE_LOG_TAG = "InstanceProfileCredentialsProvider";

InstanceProfileCredentialsProvider::InstanceProfileCredentialsProvider(long refreshRateMs) :
        m_ec2MetadataConfigLoader(Aws::MakeShared<Aws::Config::EC2InstanceProfileConfigLoader>(INSTANCE_LOG_TAG)),
        m_loadFrequencyMs(refreshRateMs)
{
    AWS_LOGSTREAM_INFO(INSTANCE_LOG_TAG, "Creating Instance with default EC2MetadataClient and refresh rate " << refreshRateMs);
}


InstanceProfileCredentialsProvider::InstanceProfileCredentialsProvider(const std::shared_ptr<Aws::Config::EC2InstanceProfileConfigLoader>& loader,
                                                                       long refreshRateMs) :
        m_ec2MetadataConfigLoader(loader),
        m_loadFrequencyMs(refreshRateMs)
{
    AWS_LOGSTREAM_INFO(INSTANCE_LOG_TAG, "Creating Instance with injected EC2MetadataClient and refresh rate " << refreshRateMs);
}


AWSCredentials InstanceProfileCredentialsProvider::GetAWSCredentials()
{
    RefreshIfExpired();
    ReaderLockGuard guard(m_reloadLock);
    auto profileIter = m_ec2MetadataConfigLoader->GetProfiles().find(Aws::Config::INSTANCE_PROFILE_KEY);

    if(profileIter != m_ec2MetadataConfigLoader->GetProfiles().end())
    {
        return profileIter->second.GetCredentials();
    }

    return AWSCredentials();
}

void InstanceProfileCredentialsProvider::Reload()
{
    AWS_LOGSTREAM_INFO(INSTANCE_LOG_TAG, "Credentials have expired attempting to repull from EC2 Metadata Service.");
    m_ec2MetadataConfigLoader->Load();
    AWSCredentialsProvider::Reload();
}

void InstanceProfileCredentialsProvider::RefreshIfExpired()
{
    AWS_LOGSTREAM_DEBUG(INSTANCE_LOG_TAG, "Checking if latest credential pull has expired.");
    ReaderLockGuard guard(m_reloadLock);
    if (!IsTimeToRefresh(m_loadFrequencyMs))
    {
        return;
    }

    guard.UpgradeToWriterLock();
    if (!IsTimeToRefresh(m_loadFrequencyMs)) // double-checked lock to avoid refreshing twice
    {
        return; 
    }
    Reload();
}

static const char TASK_ROLE_LOG_TAG[] = "TaskRoleCredentialsProvider";

TaskRoleCredentialsProvider::TaskRoleCredentialsProvider(const char* URI, long refreshRateMs) :
    m_ecsCredentialsClient(Aws::MakeShared<Aws::Internal::ECSCredentialsClient>(TASK_ROLE_LOG_TAG, URI)),
    m_loadFrequencyMs(refreshRateMs),
    m_expirationDate(DateTime::Now()),
    m_credentials(Aws::Auth::AWSCredentials())
{
    AWS_LOGSTREAM_INFO(TASK_ROLE_LOG_TAG, "Creating TaskRole with default ECSCredentialsClient and refresh rate " << refreshRateMs);
}

TaskRoleCredentialsProvider::TaskRoleCredentialsProvider(const char* endpoint, const char* token, long refreshRateMs) :
    m_ecsCredentialsClient(Aws::MakeShared<Aws::Internal::ECSCredentialsClient>(TASK_ROLE_LOG_TAG, ""/*resourcePath*/,
                endpoint, token)),
    m_loadFrequencyMs(refreshRateMs),
    m_expirationDate(DateTime::Now()),
    m_credentials(Aws::Auth::AWSCredentials())
{
    AWS_LOGSTREAM_INFO(TASK_ROLE_LOG_TAG, "Creating TaskRole with default ECSCredentialsClient and refresh rate " << refreshRateMs);
}

TaskRoleCredentialsProvider::TaskRoleCredentialsProvider(
        const std::shared_ptr<Aws::Internal::ECSCredentialsClient>& client, long refreshRateMs) :
    m_ecsCredentialsClient(client),
    m_loadFrequencyMs(refreshRateMs),
    m_expirationDate(DateTime::Now()),
    m_credentials(Aws::Auth::AWSCredentials())
{
    AWS_LOGSTREAM_INFO(TASK_ROLE_LOG_TAG, "Creating TaskRole with default ECSCredentialsClient and refresh rate " << refreshRateMs);
}

AWSCredentials TaskRoleCredentialsProvider::GetAWSCredentials()
{
    RefreshIfExpired();
    ReaderLockGuard guard(m_reloadLock);
    return m_credentials;
}

bool TaskRoleCredentialsProvider::ExpiresSoon() const
{
    return (m_expirationDate.Millis() - Aws::Utils::DateTime::Now().Millis() < EXPIRATION_GRACE_PERIOD);
}

void TaskRoleCredentialsProvider::Reload()
{
    AWS_LOGSTREAM_INFO(TASK_ROLE_LOG_TAG, "Credentials have expired or will expire, attempting to repull from ECS IAM Service.");

    auto credentialsStr = m_ecsCredentialsClient->GetECSCredentials();
    if (credentialsStr.empty()) return;

    Json::JsonValue credentialsDoc(credentialsStr);
    if (!credentialsDoc.WasParseSuccessful()) 
    {
        AWS_LOGSTREAM_ERROR(TASK_ROLE_LOG_TAG, "Failed to parse output from ECSCredentialService with error " << credentialsDoc.GetErrorMessage());
        return;
    }

    Aws::String accessKey, secretKey, token;
    Utils::Json::JsonView credentialsView(credentialsDoc);
    accessKey = credentialsView.GetString("AccessKeyId");
    secretKey = credentialsView.GetString("SecretAccessKey");
    token = credentialsView.GetString("Token");
    AWS_LOGSTREAM_DEBUG(TASK_ROLE_LOG_TAG, "Successfully pulled credentials from metadata service with access key " << accessKey);

    m_credentials.SetAWSAccessKeyId(accessKey);
    m_credentials.SetAWSSecretKey(secretKey);
    m_credentials.SetSessionToken(token);
    m_expirationDate = Aws::Utils::DateTime(credentialsView.GetString("Expiration"), DateFormat::ISO_8601);
    AWSCredentialsProvider::Reload();
}

void TaskRoleCredentialsProvider::RefreshIfExpired()
{
    AWS_LOGSTREAM_DEBUG(TASK_ROLE_LOG_TAG, "Checking if latest credential pull has expired.");
    ReaderLockGuard guard(m_reloadLock);
    if (!IsTimeToRefresh(m_loadFrequencyMs) && !ExpiresSoon())
    {
        return;
    }

    guard.UpgradeToWriterLock();

    if (!IsTimeToRefresh(m_loadFrequencyMs) && !ExpiresSoon())
    {
        return;
    }

    Reload();
}
