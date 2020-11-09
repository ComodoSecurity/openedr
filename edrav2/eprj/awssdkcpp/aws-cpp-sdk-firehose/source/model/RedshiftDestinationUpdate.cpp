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

#include <aws/firehose/model/RedshiftDestinationUpdate.h>
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

RedshiftDestinationUpdate::RedshiftDestinationUpdate() : 
    m_roleARNHasBeenSet(false),
    m_clusterJDBCURLHasBeenSet(false),
    m_copyCommandHasBeenSet(false),
    m_usernameHasBeenSet(false),
    m_passwordHasBeenSet(false),
    m_retryOptionsHasBeenSet(false),
    m_s3UpdateHasBeenSet(false),
    m_processingConfigurationHasBeenSet(false),
    m_s3BackupMode(RedshiftS3BackupMode::NOT_SET),
    m_s3BackupModeHasBeenSet(false),
    m_s3BackupUpdateHasBeenSet(false),
    m_cloudWatchLoggingOptionsHasBeenSet(false)
{
}

RedshiftDestinationUpdate::RedshiftDestinationUpdate(JsonView jsonValue) : 
    m_roleARNHasBeenSet(false),
    m_clusterJDBCURLHasBeenSet(false),
    m_copyCommandHasBeenSet(false),
    m_usernameHasBeenSet(false),
    m_passwordHasBeenSet(false),
    m_retryOptionsHasBeenSet(false),
    m_s3UpdateHasBeenSet(false),
    m_processingConfigurationHasBeenSet(false),
    m_s3BackupMode(RedshiftS3BackupMode::NOT_SET),
    m_s3BackupModeHasBeenSet(false),
    m_s3BackupUpdateHasBeenSet(false),
    m_cloudWatchLoggingOptionsHasBeenSet(false)
{
  *this = jsonValue;
}

RedshiftDestinationUpdate& RedshiftDestinationUpdate::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("RoleARN"))
  {
    m_roleARN = jsonValue.GetString("RoleARN");

    m_roleARNHasBeenSet = true;
  }

  if(jsonValue.ValueExists("ClusterJDBCURL"))
  {
    m_clusterJDBCURL = jsonValue.GetString("ClusterJDBCURL");

    m_clusterJDBCURLHasBeenSet = true;
  }

  if(jsonValue.ValueExists("CopyCommand"))
  {
    m_copyCommand = jsonValue.GetObject("CopyCommand");

    m_copyCommandHasBeenSet = true;
  }

  if(jsonValue.ValueExists("Username"))
  {
    m_username = jsonValue.GetString("Username");

    m_usernameHasBeenSet = true;
  }

  if(jsonValue.ValueExists("Password"))
  {
    m_password = jsonValue.GetString("Password");

    m_passwordHasBeenSet = true;
  }

  if(jsonValue.ValueExists("RetryOptions"))
  {
    m_retryOptions = jsonValue.GetObject("RetryOptions");

    m_retryOptionsHasBeenSet = true;
  }

  if(jsonValue.ValueExists("S3Update"))
  {
    m_s3Update = jsonValue.GetObject("S3Update");

    m_s3UpdateHasBeenSet = true;
  }

  if(jsonValue.ValueExists("ProcessingConfiguration"))
  {
    m_processingConfiguration = jsonValue.GetObject("ProcessingConfiguration");

    m_processingConfigurationHasBeenSet = true;
  }

  if(jsonValue.ValueExists("S3BackupMode"))
  {
    m_s3BackupMode = RedshiftS3BackupModeMapper::GetRedshiftS3BackupModeForName(jsonValue.GetString("S3BackupMode"));

    m_s3BackupModeHasBeenSet = true;
  }

  if(jsonValue.ValueExists("S3BackupUpdate"))
  {
    m_s3BackupUpdate = jsonValue.GetObject("S3BackupUpdate");

    m_s3BackupUpdateHasBeenSet = true;
  }

  if(jsonValue.ValueExists("CloudWatchLoggingOptions"))
  {
    m_cloudWatchLoggingOptions = jsonValue.GetObject("CloudWatchLoggingOptions");

    m_cloudWatchLoggingOptionsHasBeenSet = true;
  }

  return *this;
}

JsonValue RedshiftDestinationUpdate::Jsonize() const
{
  JsonValue payload;

  if(m_roleARNHasBeenSet)
  {
   payload.WithString("RoleARN", m_roleARN);

  }

  if(m_clusterJDBCURLHasBeenSet)
  {
   payload.WithString("ClusterJDBCURL", m_clusterJDBCURL);

  }

  if(m_copyCommandHasBeenSet)
  {
   payload.WithObject("CopyCommand", m_copyCommand.Jsonize());

  }

  if(m_usernameHasBeenSet)
  {
   payload.WithString("Username", m_username);

  }

  if(m_passwordHasBeenSet)
  {
   payload.WithString("Password", m_password);

  }

  if(m_retryOptionsHasBeenSet)
  {
   payload.WithObject("RetryOptions", m_retryOptions.Jsonize());

  }

  if(m_s3UpdateHasBeenSet)
  {
   payload.WithObject("S3Update", m_s3Update.Jsonize());

  }

  if(m_processingConfigurationHasBeenSet)
  {
   payload.WithObject("ProcessingConfiguration", m_processingConfiguration.Jsonize());

  }

  if(m_s3BackupModeHasBeenSet)
  {
   payload.WithString("S3BackupMode", RedshiftS3BackupModeMapper::GetNameForRedshiftS3BackupMode(m_s3BackupMode));
  }

  if(m_s3BackupUpdateHasBeenSet)
  {
   payload.WithObject("S3BackupUpdate", m_s3BackupUpdate.Jsonize());

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
