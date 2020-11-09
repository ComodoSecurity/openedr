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
   * <p>Describes an update for a destination in Amazon S3.</p><p><h3>See Also:</h3> 
   * <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/S3DestinationUpdate">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API S3DestinationUpdate
  {
  public:
    S3DestinationUpdate();
    S3DestinationUpdate(Aws::Utils::Json::JsonView jsonValue);
    S3DestinationUpdate& operator=(Aws::Utils::Json::JsonView jsonValue);
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
    inline S3DestinationUpdate& WithRoleARN(const Aws::String& value) { SetRoleARN(value); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline S3DestinationUpdate& WithRoleARN(Aws::String&& value) { SetRoleARN(std::move(value)); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline S3DestinationUpdate& WithRoleARN(const char* value) { SetRoleARN(value); return *this;}


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
    inline S3DestinationUpdate& WithBucketARN(const Aws::String& value) { SetBucketARN(value); return *this;}

    /**
     * <p>The ARN of the S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline S3DestinationUpdate& WithBucketARN(Aws::String&& value) { SetBucketARN(std::move(value)); return *this;}

    /**
     * <p>The ARN of the S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline S3DestinationUpdate& WithBucketARN(const char* value) { SetBucketARN(value); return *this;}


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
    inline S3DestinationUpdate& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>The "YYYY/MM/DD/HH" time format prefix is automatically used for delivered
     * Amazon S3 files. You can specify an extra prefix to be added in front of the
     * time format prefix. If the prefix ends with a slash, it appears as a folder in
     * the S3 bucket. For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/basic-deliver.html#s3-object-name">Amazon
     * S3 Object Name Format</a> in the <i>Amazon Kinesis Data Firehose Developer
     * Guide</i>.</p>
     */
    inline S3DestinationUpdate& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>The "YYYY/MM/DD/HH" time format prefix is automatically used for delivered
     * Amazon S3 files. You can specify an extra prefix to be added in front of the
     * time format prefix. If the prefix ends with a slash, it appears as a folder in
     * the S3 bucket. For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/basic-deliver.html#s3-object-name">Amazon
     * S3 Object Name Format</a> in the <i>Amazon Kinesis Data Firehose Developer
     * Guide</i>.</p>
     */
    inline S3DestinationUpdate& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>A prefix that Kinesis Data Firehose evaluates and adds to failed records
     * before writing them to S3. This prefix appears immediately following the bucket
     * name.</p>
     */
    inline const Aws::String& GetErrorOutputPrefix() const{ return m_errorOutputPrefix; }

    /**
     * <p>A prefix that Kinesis Data Firehose evaluates and adds to failed records
     * before writing them to S3. This prefix appears immediately following the bucket
     * name.</p>
     */
    inline void SetErrorOutputPrefix(const Aws::String& value) { m_errorOutputPrefixHasBeenSet = true; m_errorOutputPrefix = value; }

    /**
     * <p>A prefix that Kinesis Data Firehose evaluates and adds to failed records
     * before writing them to S3. This prefix appears immediately following the bucket
     * name.</p>
     */
    inline void SetErrorOutputPrefix(Aws::String&& value) { m_errorOutputPrefixHasBeenSet = true; m_errorOutputPrefix = std::move(value); }

    /**
     * <p>A prefix that Kinesis Data Firehose evaluates and adds to failed records
     * before writing them to S3. This prefix appears immediately following the bucket
     * name.</p>
     */
    inline void SetErrorOutputPrefix(const char* value) { m_errorOutputPrefixHasBeenSet = true; m_errorOutputPrefix.assign(value); }

    /**
     * <p>A prefix that Kinesis Data Firehose evaluates and adds to failed records
     * before writing them to S3. This prefix appears immediately following the bucket
     * name.</p>
     */
    inline S3DestinationUpdate& WithErrorOutputPrefix(const Aws::String& value) { SetErrorOutputPrefix(value); return *this;}

    /**
     * <p>A prefix that Kinesis Data Firehose evaluates and adds to failed records
     * before writing them to S3. This prefix appears immediately following the bucket
     * name.</p>
     */
    inline S3DestinationUpdate& WithErrorOutputPrefix(Aws::String&& value) { SetErrorOutputPrefix(std::move(value)); return *this;}

    /**
     * <p>A prefix that Kinesis Data Firehose evaluates and adds to failed records
     * before writing them to S3. This prefix appears immediately following the bucket
     * name.</p>
     */
    inline S3DestinationUpdate& WithErrorOutputPrefix(const char* value) { SetErrorOutputPrefix(value); return *this;}


    /**
     * <p>The buffering option. If no value is specified, <code>BufferingHints</code>
     * object default values are used.</p>
     */
    inline const BufferingHints& GetBufferingHints() const{ return m_bufferingHints; }

    /**
     * <p>The buffering option. If no value is specified, <code>BufferingHints</code>
     * object default values are used.</p>
     */
    inline void SetBufferingHints(const BufferingHints& value) { m_bufferingHintsHasBeenSet = true; m_bufferingHints = value; }

    /**
     * <p>The buffering option. If no value is specified, <code>BufferingHints</code>
     * object default values are used.</p>
     */
    inline void SetBufferingHints(BufferingHints&& value) { m_bufferingHintsHasBeenSet = true; m_bufferingHints = std::move(value); }

    /**
     * <p>The buffering option. If no value is specified, <code>BufferingHints</code>
     * object default values are used.</p>
     */
    inline S3DestinationUpdate& WithBufferingHints(const BufferingHints& value) { SetBufferingHints(value); return *this;}

    /**
     * <p>The buffering option. If no value is specified, <code>BufferingHints</code>
     * object default values are used.</p>
     */
    inline S3DestinationUpdate& WithBufferingHints(BufferingHints&& value) { SetBufferingHints(std::move(value)); return *this;}


    /**
     * <p>The compression format. If no value is specified, the default is
     * <code>UNCOMPRESSED</code>.</p> <p>The compression formats <code>SNAPPY</code> or
     * <code>ZIP</code> cannot be specified for Amazon Redshift destinations because
     * they are not supported by the Amazon Redshift <code>COPY</code> operation that
     * reads from the S3 bucket.</p>
     */
    inline const CompressionFormat& GetCompressionFormat() const{ return m_compressionFormat; }

    /**
     * <p>The compression format. If no value is specified, the default is
     * <code>UNCOMPRESSED</code>.</p> <p>The compression formats <code>SNAPPY</code> or
     * <code>ZIP</code> cannot be specified for Amazon Redshift destinations because
     * they are not supported by the Amazon Redshift <code>COPY</code> operation that
     * reads from the S3 bucket.</p>
     */
    inline void SetCompressionFormat(const CompressionFormat& value) { m_compressionFormatHasBeenSet = true; m_compressionFormat = value; }

    /**
     * <p>The compression format. If no value is specified, the default is
     * <code>UNCOMPRESSED</code>.</p> <p>The compression formats <code>SNAPPY</code> or
     * <code>ZIP</code> cannot be specified for Amazon Redshift destinations because
     * they are not supported by the Amazon Redshift <code>COPY</code> operation that
     * reads from the S3 bucket.</p>
     */
    inline void SetCompressionFormat(CompressionFormat&& value) { m_compressionFormatHasBeenSet = true; m_compressionFormat = std::move(value); }

    /**
     * <p>The compression format. If no value is specified, the default is
     * <code>UNCOMPRESSED</code>.</p> <p>The compression formats <code>SNAPPY</code> or
     * <code>ZIP</code> cannot be specified for Amazon Redshift destinations because
     * they are not supported by the Amazon Redshift <code>COPY</code> operation that
     * reads from the S3 bucket.</p>
     */
    inline S3DestinationUpdate& WithCompressionFormat(const CompressionFormat& value) { SetCompressionFormat(value); return *this;}

    /**
     * <p>The compression format. If no value is specified, the default is
     * <code>UNCOMPRESSED</code>.</p> <p>The compression formats <code>SNAPPY</code> or
     * <code>ZIP</code> cannot be specified for Amazon Redshift destinations because
     * they are not supported by the Amazon Redshift <code>COPY</code> operation that
     * reads from the S3 bucket.</p>
     */
    inline S3DestinationUpdate& WithCompressionFormat(CompressionFormat&& value) { SetCompressionFormat(std::move(value)); return *this;}


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
    inline S3DestinationUpdate& WithEncryptionConfiguration(const EncryptionConfiguration& value) { SetEncryptionConfiguration(value); return *this;}

    /**
     * <p>The encryption configuration. If no value is specified, the default is no
     * encryption.</p>
     */
    inline S3DestinationUpdate& WithEncryptionConfiguration(EncryptionConfiguration&& value) { SetEncryptionConfiguration(std::move(value)); return *this;}


    /**
     * <p>The CloudWatch logging options for your delivery stream.</p>
     */
    inline const CloudWatchLoggingOptions& GetCloudWatchLoggingOptions() const{ return m_cloudWatchLoggingOptions; }

    /**
     * <p>The CloudWatch logging options for your delivery stream.</p>
     */
    inline void SetCloudWatchLoggingOptions(const CloudWatchLoggingOptions& value) { m_cloudWatchLoggingOptionsHasBeenSet = true; m_cloudWatchLoggingOptions = value; }

    /**
     * <p>The CloudWatch logging options for your delivery stream.</p>
     */
    inline void SetCloudWatchLoggingOptions(CloudWatchLoggingOptions&& value) { m_cloudWatchLoggingOptionsHasBeenSet = true; m_cloudWatchLoggingOptions = std::move(value); }

    /**
     * <p>The CloudWatch logging options for your delivery stream.</p>
     */
    inline S3DestinationUpdate& WithCloudWatchLoggingOptions(const CloudWatchLoggingOptions& value) { SetCloudWatchLoggingOptions(value); return *this;}

    /**
     * <p>The CloudWatch logging options for your delivery stream.</p>
     */
    inline S3DestinationUpdate& WithCloudWatchLoggingOptions(CloudWatchLoggingOptions&& value) { SetCloudWatchLoggingOptions(std::move(value)); return *this;}

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
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
