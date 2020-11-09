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

#pragma once
#include <aws/firehose/Firehose_EXPORTS.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/firehose/model/BufferingHints.h>
#include <aws/firehose/model/CompressionFormat.h>
#include <aws/firehose/model/EncryptionConfiguration.h>
#include <aws/firehose/model/CloudWatchLoggingOptions.h>
#include <aws/firehose/model/ProcessingConfiguration.h>
#include <aws/firehose/model/S3BackupMode.h>
#include <aws/firehose/model/S3DestinationConfiguration.h>
#include <aws/firehose/model/DataFormatConversionConfiguration.h>
#include <utility>

namespace Aws
{
namespace Utils
{
namespace Json
{
  class JsonValue;
  class JsonView;
} // namespace Json
} // namespace Utils
namespace Firehose
{
namespace Model
{

  /**
   * <p>Describes the configuration of a destination in Amazon S3.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/ExtendedS3DestinationConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API ExtendedS3DestinationConfiguration
  {
  public:
    ExtendedS3DestinationConfiguration();
    ExtendedS3DestinationConfiguration(Aws::Utils::Json::JsonView jsonValue);
    ExtendedS3DestinationConfiguration& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline const Aws::String& GetRoleARN() const{ return m_roleARN; }

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetRoleARN(const Aws::String& value) { m_roleARNHasBeenSet = true; m_roleARN = value; }

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetRoleARN(Aws::String&& value) { m_roleARNHasBeenSet = true; m_roleARN = std::move(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetRoleARN(const char* value) { m_roleARNHasBeenSet = true; m_roleARN.assign(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithRoleARN(const Aws::String& value) { SetRoleARN(value); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithRoleARN(Aws::String&& value) { SetRoleARN(std::move(value)); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithRoleARN(const char* value) { SetRoleARN(value); return *this;}


    /**
     * <p>The ARN of the S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline const Aws::String& GetBucketARN() const{ return m_bucketARN; }

    /**
     * <p>The ARN of the S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetBucketARN(const Aws::String& value) { m_bucketARNHasBeenSet = true; m_bucketARN = value; }

    /**
     * <p>The ARN of the S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetBucketARN(Aws::String&& value) { m_bucketARNHasBeenSet = true; m_bucketARN = std::move(value); }

    /**
     * <p>The ARN of the S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetBucketARN(const char* value) { m_bucketARNHasBeenSet = true; m_bucketARN.assign(value); }

    /**
     * <p>The ARN of the S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithBucketARN(const Aws::String& value) { SetBucketARN(value); return *this;}

    /**
     * <p>The ARN of the S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithBucketARN(Aws::String&& value) { SetBucketARN(std::move(value)); return *this;}

    /**
     * <p>The ARN of the S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithBucketARN(const char* value) { SetBucketARN(value); return *this;}


    /**
     * <p>The "YYYY/MM/DD/HH" time format prefix is automatically used for delivered
     * Amazon S3 files. You can specify an extra prefix to be added in front of the
     * time format prefix. If the prefix ends with a slash, it appears as a folder in
     * the S3 bucket. For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/basic-deliver.html#s3-object-name">Amazon
     * S3 Object Name Format</a> in the <i>Amazon Kinesis Data Firehose Developer
     * Guide</i>.</p>
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * <p>The "YYYY/MM/DD/HH" time format prefix is automatically used for delivered
     * Amazon S3 files. You can specify an extra prefix to be added in front of the
     * time format prefix. If the prefix ends with a slash, it appears as a folder in
     * the S3 bucket. For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/basic-deliver.html#s3-object-name">Amazon
     * S3 Object Name Format</a> in the <i>Amazon Kinesis Data Firehose Developer
     * Guide</i>.</p>
     */
    inline void SetPrefix(const Aws::String& value) { m_prefixHasBeenSet = true; m_prefix = value; }

    /**
     * <p>The "YYYY/MM/DD/HH" time format prefix is automatically used for delivered
     * Amazon S3 files. You can specify an extra prefix to be added in front of the
     * time format prefix. If the prefix ends with a slash, it appears as a folder in
     * the S3 bucket. For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/basic-deliver.html#s3-object-name">Amazon
     * S3 Object Name Format</a> in the <i>Amazon Kinesis Data Firehose Developer
     * Guide</i>.</p>
     */
    inline void SetPrefix(Aws::String&& value) { m_prefixHasBeenSet = true; m_prefix = std::move(value); }

    /**
     * <p>The "YYYY/MM/DD/HH" time format prefix is automatically used for delivered
     * Amazon S3 files. You can specify an extra prefix to be added in front of the
     * time format prefix. If the prefix ends with a slash, it appears as a folder in
     * the S3 bucket. For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/basic-deliver.html#s3-object-name">Amazon
     * S3 Object Name Format</a> in the <i>Amazon Kinesis Data Firehose Developer
     * Guide</i>.</p>
     */
    inline void SetPrefix(const char* value) { m_prefixHasBeenSet = true; m_prefix.assign(value); }

    /**
     * <p>The "YYYY/MM/DD/HH" time format prefix is automatically used for delivered
     * Amazon S3 files. You can specify an extra prefix to be added in front of the
     * time format prefix. If the prefix ends with a slash, it appears as a folder in
     * the S3 bucket. For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/basic-deliver.html#s3-object-name">Amazon
     * S3 Object Name Format</a> in the <i>Amazon Kinesis Data Firehose Developer
     * Guide</i>.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>The "YYYY/MM/DD/HH" time format prefix is automatically used for delivered
     * Amazon S3 files. You can specify an extra prefix to be added in front of the
     * time format prefix. If the prefix ends with a slash, it appears as a folder in
     * the S3 bucket. For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/basic-deliver.html#s3-object-name">Amazon
     * S3 Object Name Format</a> in the <i>Amazon Kinesis Data Firehose Developer
     * Guide</i>.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>The "YYYY/MM/DD/HH" time format prefix is automatically used for delivered
     * Amazon S3 files. You can specify an extra prefix to be added in front of the
     * time format prefix. If the prefix ends with a slash, it appears as a folder in
     * the S3 bucket. For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/basic-deliver.html#s3-object-name">Amazon
     * S3 Object Name Format</a> in the <i>Amazon Kinesis Data Firehose Developer
     * Guide</i>.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>A prefix that Kinesis Data Firehose evaluates and adds to failed records
     * before writing them to S3. This prefix appears immediately following the bucket
     * name. </p>
     */
    inline const Aws::String& GetErrorOutputPrefix() const{ return m_errorOutputPrefix; }

    /**
     * <p>A prefix that Kinesis Data Firehose evaluates and adds to failed records
     * before writing them to S3. This prefix appears immediately following the bucket
     * name. </p>
     */
    inline void SetErrorOutputPrefix(const Aws::String& value) { m_errorOutputPrefixHasBeenSet = true; m_errorOutputPrefix = value; }

    /**
     * <p>A prefix that Kinesis Data Firehose evaluates and adds to failed records
     * before writing them to S3. This prefix appears immediately following the bucket
     * name. </p>
     */
    inline void SetErrorOutputPrefix(Aws::String&& value) { m_errorOutputPrefixHasBeenSet = true; m_errorOutputPrefix = std::move(value); }

    /**
     * <p>A prefix that Kinesis Data Firehose evaluates and adds to failed records
     * before writing them to S3. This prefix appears immediately following the bucket
     * name. </p>
     */
    inline void SetErrorOutputPrefix(const char* value) { m_errorOutputPrefixHasBeenSet = true; m_errorOutputPrefix.assign(value); }

    /**
     * <p>A prefix that Kinesis Data Firehose evaluates and adds to failed records
     * before writing them to S3. This prefix appears immediately following the bucket
     * name. </p>
     */
    inline ExtendedS3DestinationConfiguration& WithErrorOutputPrefix(const Aws::String& value) { SetErrorOutputPrefix(value); return *this;}

    /**
     * <p>A prefix that Kinesis Data Firehose evaluates and adds to failed records
     * before writing them to S3. This prefix appears immediately following the bucket
     * name. </p>
     */
    inline ExtendedS3DestinationConfiguration& WithErrorOutputPrefix(Aws::String&& value) { SetErrorOutputPrefix(std::move(value)); return *this;}

    /**
     * <p>A prefix that Kinesis Data Firehose evaluates and adds to failed records
     * before writing them to S3. This prefix appears immediately following the bucket
     * name. </p>
     */
    inline ExtendedS3DestinationConfiguration& WithErrorOutputPrefix(const char* value) { SetErrorOutputPrefix(value); return *this;}


    /**
     * <p>The buffering option.</p>
     */
    inline const BufferingHints& GetBufferingHints() const{ return m_bufferingHints; }

    /**
     * <p>The buffering option.</p>
     */
    inline void SetBufferingHints(const BufferingHints& value) { m_bufferingHintsHasBeenSet = true; m_bufferingHints = value; }

    /**
     * <p>The buffering option.</p>
     */
    inline void SetBufferingHints(BufferingHints&& value) { m_bufferingHintsHasBeenSet = true; m_bufferingHints = std::move(value); }

    /**
     * <p>The buffering option.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithBufferingHints(const BufferingHints& value) { SetBufferingHints(value); return *this;}

    /**
     * <p>The buffering option.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithBufferingHints(BufferingHints&& value) { SetBufferingHints(std::move(value)); return *this;}


    /**
     * <p>The compression format. If no value is specified, the default is
     * UNCOMPRESSED.</p>
     */
    inline const CompressionFormat& GetCompressionFormat() const{ return m_compressionFormat; }

    /**
     * <p>The compression format. If no value is specified, the default is
     * UNCOMPRESSED.</p>
     */
    inline void SetCompressionFormat(const CompressionFormat& value) { m_compressionFormatHasBeenSet = true; m_compressionFormat = value; }

    /**
     * <p>The compression format. If no value is specified, the default is
     * UNCOMPRESSED.</p>
     */
    inline void SetCompressionFormat(CompressionFormat&& value) { m_compressionFormatHasBeenSet = true; m_compressionFormat = std::move(value); }

    /**
     * <p>The compression format. If no value is specified, the default is
     * UNCOMPRESSED.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithCompressionFormat(const CompressionFormat& value) { SetCompressionFormat(value); return *this;}

    /**
     * <p>The compression format. If no value is specified, the default is
     * UNCOMPRESSED.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithCompressionFormat(CompressionFormat&& value) { SetCompressionFormat(std::move(value)); return *this;}


    /**
     * <p>The encryption configuration. If no value is specified, the default is no
     * encryption.</p>
     */
    inline const EncryptionConfiguration& GetEncryptionConfiguration() const{ return m_encryptionConfiguration; }

    /**
     * <p>The encryption configuration. If no value is specified, the default is no
     * encryption.</p>
     */
    inline void SetEncryptionConfiguration(const EncryptionConfiguration& value) { m_encryptionConfigurationHasBeenSet = true; m_encryptionConfiguration = value; }

    /**
     * <p>The encryption configuration. If no value is specified, the default is no
     * encryption.</p>
     */
    inline void SetEncryptionConfiguration(EncryptionConfiguration&& value) { m_encryptionConfigurationHasBeenSet = true; m_encryptionConfiguration = std::move(value); }

    /**
     * <p>The encryption configuration. If no value is specified, the default is no
     * encryption.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithEncryptionConfiguration(const EncryptionConfiguration& value) { SetEncryptionConfiguration(value); return *this;}

    /**
     * <p>The encryption configuration. If no value is specified, the default is no
     * encryption.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithEncryptionConfiguration(EncryptionConfiguration&& value) { SetEncryptionConfiguration(std::move(value)); return *this;}


    /**
     * <p>The Amazon CloudWatch logging options for your delivery stream.</p>
     */
    inline const CloudWatchLoggingOptions& GetCloudWatchLoggingOptions() const{ return m_cloudWatchLoggingOptions; }

    /**
     * <p>The Amazon CloudWatch logging options for your delivery stream.</p>
     */
    inline void SetCloudWatchLoggingOptions(const CloudWatchLoggingOptions& value) { m_cloudWatchLoggingOptionsHasBeenSet = true; m_cloudWatchLoggingOptions = value; }

    /**
     * <p>The Amazon CloudWatch logging options for your delivery stream.</p>
     */
    inline void SetCloudWatchLoggingOptions(CloudWatchLoggingOptions&& value) { m_cloudWatchLoggingOptionsHasBeenSet = true; m_cloudWatchLoggingOptions = std::move(value); }

    /**
     * <p>The Amazon CloudWatch logging options for your delivery stream.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithCloudWatchLoggingOptions(const CloudWatchLoggingOptions& value) { SetCloudWatchLoggingOptions(value); return *this;}

    /**
     * <p>The Amazon CloudWatch logging options for your delivery stream.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithCloudWatchLoggingOptions(CloudWatchLoggingOptions&& value) { SetCloudWatchLoggingOptions(std::move(value)); return *this;}


    /**
     * <p>The data processing configuration.</p>
     */
    inline const ProcessingConfiguration& GetProcessingConfiguration() const{ return m_processingConfiguration; }

    /**
     * <p>The data processing configuration.</p>
     */
    inline void SetProcessingConfiguration(const ProcessingConfiguration& value) { m_processingConfigurationHasBeenSet = true; m_processingConfiguration = value; }

    /**
     * <p>The data processing configuration.</p>
     */
    inline void SetProcessingConfiguration(ProcessingConfiguration&& value) { m_processingConfigurationHasBeenSet = true; m_processingConfiguration = std::move(value); }

    /**
     * <p>The data processing configuration.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithProcessingConfiguration(const ProcessingConfiguration& value) { SetProcessingConfiguration(value); return *this;}

    /**
     * <p>The data processing configuration.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithProcessingConfiguration(ProcessingConfiguration&& value) { SetProcessingConfiguration(std::move(value)); return *this;}


    /**
     * <p>The Amazon S3 backup mode.</p>
     */
    inline const S3BackupMode& GetS3BackupMode() const{ return m_s3BackupMode; }

    /**
     * <p>The Amazon S3 backup mode.</p>
     */
    inline void SetS3BackupMode(const S3BackupMode& value) { m_s3BackupModeHasBeenSet = true; m_s3BackupMode = value; }

    /**
     * <p>The Amazon S3 backup mode.</p>
     */
    inline void SetS3BackupMode(S3BackupMode&& value) { m_s3BackupModeHasBeenSet = true; m_s3BackupMode = std::move(value); }

    /**
     * <p>The Amazon S3 backup mode.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithS3BackupMode(const S3BackupMode& value) { SetS3BackupMode(value); return *this;}

    /**
     * <p>The Amazon S3 backup mode.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithS3BackupMode(S3BackupMode&& value) { SetS3BackupMode(std::move(value)); return *this;}


    /**
     * <p>The configuration for backup in Amazon S3.</p>
     */
    inline const S3DestinationConfiguration& GetS3BackupConfiguration() const{ return m_s3BackupConfiguration; }

    /**
     * <p>The configuration for backup in Amazon S3.</p>
     */
    inline void SetS3BackupConfiguration(const S3DestinationConfiguration& value) { m_s3BackupConfigurationHasBeenSet = true; m_s3BackupConfiguration = value; }

    /**
     * <p>The configuration for backup in Amazon S3.</p>
     */
    inline void SetS3BackupConfiguration(S3DestinationConfiguration&& value) { m_s3BackupConfigurationHasBeenSet = true; m_s3BackupConfiguration = std::move(value); }

    /**
     * <p>The configuration for backup in Amazon S3.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithS3BackupConfiguration(const S3DestinationConfiguration& value) { SetS3BackupConfiguration(value); return *this;}

    /**
     * <p>The configuration for backup in Amazon S3.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithS3BackupConfiguration(S3DestinationConfiguration&& value) { SetS3BackupConfiguration(std::move(value)); return *this;}


    /**
     * <p>The serializer, deserializer, and schema for converting data from the JSON
     * format to the Parquet or ORC format before writing it to Amazon S3.</p>
     */
    inline const DataFormatConversionConfiguration& GetDataFormatConversionConfiguration() const{ return m_dataFormatConversionConfiguration; }

    /**
     * <p>The serializer, deserializer, and schema for converting data from the JSON
     * format to the Parquet or ORC format before writing it to Amazon S3.</p>
     */
    inline void SetDataFormatConversionConfiguration(const DataFormatConversionConfiguration& value) { m_dataFormatConversionConfigurationHasBeenSet = true; m_dataFormatConversionConfiguration = value; }

    /**
     * <p>The serializer, deserializer, and schema for converting data from the JSON
     * format to the Parquet or ORC format before writing it to Amazon S3.</p>
     */
    inline void SetDataFormatConversionConfiguration(DataFormatConversionConfiguration&& value) { m_dataFormatConversionConfigurationHasBeenSet = true; m_dataFormatConversionConfiguration = std::move(value); }

    /**
     * <p>The serializer, deserializer, and schema for converting data from the JSON
     * format to the Parquet or ORC format before writing it to Amazon S3.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithDataFormatConversionConfiguration(const DataFormatConversionConfiguration& value) { SetDataFormatConversionConfiguration(value); return *this;}

    /**
     * <p>The serializer, deserializer, and schema for converting data from the JSON
     * format to the Parquet or ORC format before writing it to Amazon S3.</p>
     */
    inline ExtendedS3DestinationConfiguration& WithDataFormatConversionConfiguration(DataFormatConversionConfiguration&& value) { SetDataFormatConversionConfiguration(std::move(value)); return *this;}

  private:

    Aws::String m_roleARN;
    bool m_roleARNHasBeenSet;

    Aws::String m_bucketARN;
    bool m_bucketARNHasBeenSet;

    Aws::String m_prefix;
    bool m_prefixHasBeenSet;

    Aws::String m_errorOutputPrefix;
    bool m_errorOutputPrefixHasBeenSet;

    BufferingHints m_bufferingHints;
    bool m_bufferingHintsHasBeenSet;

    CompressionFormat m_compressionFormat;
    bool m_compressionFormatHasBeenSet;

    EncryptionConfiguration m_encryptionConfiguration;
    bool m_encryptionConfigurationHasBeenSet;

    CloudWatchLoggingOptions m_cloudWatchLoggingOptions;
    bool m_cloudWatchLoggingOptionsHasBeenSet;

    ProcessingConfiguration m_processingConfiguration;
    bool m_processingConfigurationHasBeenSet;

    S3BackupMode m_s3BackupMode;
    bool m_s3BackupModeHasBeenSet;

    S3DestinationConfiguration m_s3BackupConfiguration;
    bool m_s3BackupConfigurationHasBeenSet;

    DataFormatConversionConfiguration m_dataFormatConversionConfiguration;
    bool m_dataFormatConversionConfigurationHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
