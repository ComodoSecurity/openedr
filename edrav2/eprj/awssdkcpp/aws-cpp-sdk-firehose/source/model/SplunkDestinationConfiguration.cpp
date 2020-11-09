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

#include <aws/firehose/model/SplunkDestinationConfiguration.h>
#include <aws/core/utils/json/JsonSerializer.h>

#include <utility>

using namespace Aws::Utils::Json;
using namespace Aws::Utils;

namespace Aws
{
namespace Firehose
{
namespace Model
{

SplunkDestinationConfiguration::SplunkDestinationConfiguration() : 
    m_hECEndpointHasBeenSet(false),
    m_hECEndpointType(HECEndpointType::NOT_SET),
    m_hECEndpointTypeHasBeenSet(false),
    m_hECTokenHasBeenSet(false),
    m_hECAcknowledgmentTimeoutInSeconds(0),
    m_hECAcknowledgmentTimeoutInSecondsHasBeenSet(false),
    m_retryOptionsHasBeenSet(false),
    m_s3BackupMode(SplunkS3BackupMode::NOT_SET),
    m_s3BackupModeHasBeenSet(false),
    m_s3ConfigurationHasBeenSet(false),
    m_processingConfigurationHasBeenSet(false),
    m_cloudWatchLoggingOptionsHasBeenSet(false)
{
}

SplunkDestinationConfiguration::SplunkDestinationConfiguration(JsonView jsonValue) : 
    m_hECEndpointHasBeenSet(false),
    m_hECEndpointType(HECEndpointType::NOT_SET),
    m_hECEndpointTypeHasBeenSet(false),
    m_hECTokenHasBeenSet(false),
    m_hECAcknowledgmentTimeoutInSeconds(0),
    m_hECAcknowledgmentTimeoutInSecondsHasBeenSet(false),
    m_retryOptionsHasBeenSet(false),
    m_s3BackupMode(SplunkS3BackupMode::NOT_SET),
    m_s3BackupModeHasBeenSet(false),
    m_s3ConfigurationHasBeenSet(false),
    m_processingConfigurationHasBeenSet(false),
    m_cloudWatchLoggingOptionsHasBeenSet(false)
{
  *this = jsonValue;
}

SplunkDestinationConfiguration& SplunkDestinationConfiguration::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("HECEndpoint"))
  {
    m_hECEndpoint = jsonValue.GetString("HECEndpoint");

    m_hECEndpointHasBeenSet = true;
  }

  if(jsonValue.ValueExists("HECEndpointType"))
  {
    m_hECEndpointType = HECEndpointTypeMapper::GetHECEndpointTypeForName(jsonValue.GetString("HECEndpointType"));

    m_hECEndpointTypeHasBeenSet = true;
  }

  if(jsonValue.ValueExists("HECToken"))
  {
    m_hECToken = jsonValue.GetString("HECToken");

    m_hECTokenHasBeenSet = true;
  }

  if(jsonValue.ValueExists("HECAcknowledgmentTimeoutInSeconds"))
  {
    m_hECAcknowledgmentTimeoutInSeconds = jsonValue.GetInteger("HECAcknowledgmentTimeoutInSeconds");

    m_hECAcknowledgmentTimeoutInSecondsHasBeenSet = true;
  }

  if(jsonValue.ValueExists("RetryOptions"))
  {
    m_retryOptions = jsonValue.GetObject("RetryOptions");

    m_retryOptionsHasBeenSet = true;
  }

  if(jsonValue.ValueExists("S3BackupMode"))
  {
    m_s3BackupMode = SplunkS3BackupModeMapper::GetSplunkS3BackupModeForName(jsonValue.GetString("S3BackupMode"));

    m_s3BackupModeHasBeenSet = true;
  }

  if(jsonValue.ValueExists("S3Configuration"))
  {
    m_s3Configuration = jsonValue.GetObject("S3Configuration");

    m_s3ConfigurationHasBeenSet = true;
  }

  if(jsonValue.ValueExists("ProcessingConfiguration"))
  {
    m_processingConfiguration = jsonValue.GetObject("ProcessingConfiguration");

    m_processingConfigurationHasBeenSet = true;
  }

  if(jsonValue.ValueExists("CloudWatchLoggingOptions"))
  {
    m_cloudWatchLoggingOptions = jsonValue.GetObject("CloudWatchLoggingOptions");

    m_cloudWatchLoggingOptionsHasBeenSet = true;
  }

  return *this;
}

JsonValue SplunkDestinationConfiguration::Jsonize() const
{
  JsonValue payload;

  if(m_hECEndpointHasBeenSet)
  {
   payload.WithString("HECEndpoint", m_hECEndpoint);

  }

  if(m_hECEndpointTypeHasBeenSet)
  {
   payload.WithString("HECEndpointType", HECEndpointTypeMapper::GetNameForHECEndpointType(m_hECEndpointType));
  }

  if(m_hECTokenHasBeenSet)
  {
   payload.WithString("HECToken", m_hECToken);

  }

  if(m_hECAcknowledgmentTimeoutInSecondsHasBeenSet)
  {
   payload.WithInteger("HECAcknowledgmentTimeoutInSeconds", m_hECAcknowledgmentTimeoutInSeconds);

  }

  if(m_retryOptionsHasBeenSet)
  {
   payload.WithObject("RetryOptions", m_retryOptions.Jsonize());

  }

  if(m_s3BackupModeHasBeenSet)
  {
   payload.WithString("S3BackupMode", SplunkS3BackupModeMapper::GetNameForSplunkS3BackupMode(m_s3BackupMode));
  }

  if(m_s3ConfigurationHasBeenSet)
  {
   payload.WithObject("S3Configuration", m_s3Configuration.Jsonize());

  }

  if(m_processingConfigurationHasBeenSet)
  {
   payload.WithObject("ProcessingConfiguration", m_processingConfiguration.Jsonize());

  }

  if(m_cloudWatchLoggingOptionsHasBeenSet)
  {
   payload.WithObject("CloudWatchLoggingOptions", m_cloudWatchLoggingOptions.Jsonize());

  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
