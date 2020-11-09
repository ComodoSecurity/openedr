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

#include <aws/firehose/model/ExtendedS3DestinationDescription.h>
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

ExtendedS3DestinationDescription::ExtendedS3DestinationDescription() : 
    m_roleARNHasBeenSet(false),
    m_bucketARNHasBeenSet(false),
    m_prefixHasBeenSet(false),
    m_errorOutputPrefixHasBeenSet(false),
    m_bufferingHintsHasBeenSet(false),
    m_compressionFormat(CompressionFormat::NOT_SET),
    m_compressionFormatHasBeenSet(false),
    m_encryptionConfigurationHasBeenSet(false),
    m_cloudWatchLoggingOptionsHasBeenSet(false),
    m_processingConfigurationHasBeenSet(false),
    m_s3BackupMode(S3BackupMode::NOT_SET),
    m_s3BackupModeHasBeenSet(false),
    m_s3BackupDescriptionHasBeenSet(false),
    m_dataFormatConversionConfigurationHasBeenSet(false)
{
}

ExtendedS3DestinationDescription::ExtendedS3DestinationDescription(JsonView jsonValue) : 
    m_roleARNHasBeenSet(false),
    m_bucketARNHasBeenSet(false),
    m_prefixHasBeenSet(false),
    m_errorOutputPrefixHasBeenSet(false),
    m_bufferingHintsHasBeenSet(false),
    m_compressionFormat(CompressionFormat::NOT_SET),
    m_compressionFormatHasBeenSet(false),
    m_encryptionConfigurationHasBeenSet(false),
    m_cloudWatchLoggingOptionsHasBeenSet(false),
    m_processingConfigurationHasBeenSet(false),
    m_s3BackupMode(S3BackupMode::NOT_SET),
    m_s3BackupModeHasBeenSet(false),
    m_s3BackupDescriptionHasBeenSet(false),
    m_dataFormatConversionConfigurationHasBeenSet(false)
{
  *this = jsonValue;
}

ExtendedS3DestinationDescription& ExtendedS3DestinationDescription::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("RoleARN"))
  {
    m_roleARN = jsonValue.GetString("RoleARN");

    m_roleARNHasBeenSet = true;
  }

  if(jsonValue.ValueExists("BucketARN"))
  {
    m_bucketARN = jsonValue.GetString("BucketARN");

    m_bucketARNHasBeenSet = true;
  }

  if(jsonValue.ValueExists("Prefix"))
  {
    m_prefix = jsonValue.GetString("Prefix");

    m_prefixHasBeenSet = true;
  }

  if(jsonValue.ValueExists("ErrorOutputPrefix"))
  {
    m_errorOutputPrefix = jsonValue.GetString("ErrorOutputPrefix");

    m_errorOutputPrefixHasBeenSet = true;
  }

  if(jsonValue.ValueExists("BufferingHints"))
  {
    m_bufferingHints = jsonValue.GetObject("BufferingHints");

    m_bufferingHintsHasBeenSet = true;
  }

  if(jsonValue.ValueExists("CompressionFormat"))
  {
    m_compressionFormat = CompressionFormatMapper::GetCompressionFormatForName(jsonValue.GetString("CompressionFormat"));

    m_compressionFormatHasBeenSet = true;
  }

  if(jsonValue.ValueExists("EncryptionConfiguration"))
  {
    m_encryptionConfiguration = jsonValue.GetObject("EncryptionConfiguration");

    m_encryptionConfigurationHasBeenSet = true;
  }

  if(jsonValue.ValueExists("CloudWatchLoggingOptions"))
  {
    m_cloudWatchLoggingOptions = jsonValue.GetObject("CloudWatchLoggingOptions");

    m_cloudWatchLoggingOptionsHasBeenSet = true;
  }

  if(jsonValue.ValueExists("ProcessingConfiguration"))
  {
    m_processingConfiguration = jsonValue.GetObject("ProcessingConfiguration");

    m_processingConfigurationHasBeenSet = true;
  }

  if(jsonValue.ValueExists("S3BackupMode"))
  {
    m_s3BackupMode = S3BackupModeMapper::GetS3BackupModeForName(jsonValue.GetString("S3BackupMode"));

    m_s3BackupModeHasBeenSet = true;
  }

  if(jsonValue.ValueExists("S3BackupDescription"))
  {
    m_s3BackupDescription = jsonValue.GetObject("S3BackupDescription");

    m_s3BackupDescriptionHasBeenSet = true;
  }

  if(jsonValue.ValueExists("DataFormatConversionConfiguration"))
  {
    m_dataFormatConversionConfiguration = jsonValue.GetObject("DataFormatConversionConfiguration");

    m_dataFormatConversionConfigurationHasBeenSet = true;
  }

  return *this;
}

JsonValue ExtendedS3DestinationDescription::Jsonize() const
{
  JsonValue payload;

  if(m_roleARNHasBeenSet)
  {
   payload.WithString("RoleARN", m_roleARN);

  }

  if(m_bucketARNHasBeenSet)
  {
   payload.WithString("BucketARN", m_bucketARN);

  }

  if(m_prefixHasBeenSet)
  {
   payload.WithString("Prefix", m_prefix);

  }

  if(m_errorOutputPrefixHasBeenSet)
  {
   payload.WithString("ErrorOutputPrefix", m_errorOutputPrefix);

  }

  if(m_bufferingHintsHasBeenSet)
  {
   payload.WithObject("BufferingHints", m_bufferingHints.Jsonize());

  }

  if(m_compressionFormatHasBeenSet)
  {
   payload.WithString("CompressionFormat", CompressionFormatMapper::GetNameForCompressionFormat(m_compressionFormat));
  }

  if(m_encryptionConfigurationHasBeenSet)
  {
   payload.WithObject("EncryptionConfiguration", m_encryptionConfiguration.Jsonize());

  }

  if(m_cloudWatchLoggingOptionsHasBeenSet)
  {
   payload.WithObject("CloudWatchLoggingOptions", m_cloudWatchLoggingOptions.Jsonize());

  }

  if(m_processingConfigurationHasBeenSet)
  {
   payload.WithObject("ProcessingConfiguration", m_processingConfiguration.Jsonize());

  }

  if(m_s3BackupModeHasBeenSet)
  {
   payload.WithString("S3BackupMode", S3BackupModeMapper::GetNameForS3BackupMode(m_s3BackupMode));
  }

  if(m_s3BackupDescriptionHasBeenSet)
  {
   payload.WithObject("S3BackupDescription", m_s3BackupDescription.Jsonize());

  }

  if(m_dataFormatConversionConfigurationHasBeenSet)
  {
   payload.WithObject("DataFormatConversionConfiguration", m_dataFormatConversionConfiguration.Jsonize());

  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
