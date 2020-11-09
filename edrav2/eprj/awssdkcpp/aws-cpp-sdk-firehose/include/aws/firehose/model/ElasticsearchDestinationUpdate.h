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
#include <aws/firehose/model/ElasticsearchIndexRotationPeriod.h>
#include <aws/firehose/model/ElasticsearchBufferingHints.h>
#include <aws/firehose/model/ElasticsearchRetryOptions.h>
#include <aws/firehose/model/S3DestinationUpdate.h>
#include <aws/firehose/model/ProcessingConfiguration.h>
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
   * <p>Describes an update for a destination in Amazon ES.</p><p><h3>See Also:</h3> 
   * <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/ElasticsearchDestinationUpdate">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API ElasticsearchDestinationUpdate
  {
  public:
    ElasticsearchDestinationUpdate();
    ElasticsearchDestinationUpdate(Aws::Utils::Json::JsonView jsonValue);
    ElasticsearchDestinationUpdate& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>The Amazon Resource Name (ARN) of the IAM role to be assumed by Kinesis Data
     * Firehose for calling the Amazon ES Configuration API and for indexing documents.
     * For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/controlling-access.html#using-iam-s3">Grant
     * Kinesis Data Firehose Access to an Amazon S3 Destination</a> and <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline const Aws::String& GetRoleARN() const{ return m_roleARN; }

    /**
     * <p>The Amazon Resource Name (ARN) of the IAM role to be assumed by Kinesis Data
     * Firehose for calling the Amazon ES Configuration API and for indexing documents.
     * For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/controlling-access.html#using-iam-s3">Grant
     * Kinesis Data Firehose Access to an Amazon S3 Destination</a> and <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetRoleARN(const Aws::String& value) { m_roleARNHasBeenSet = true; m_roleARN = value; }

    /**
     * <p>The Amazon Resource Name (ARN) of the IAM role to be assumed by Kinesis Data
     * Firehose for calling the Amazon ES Configuration API and for indexing documents.
     * For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/controlling-access.html#using-iam-s3">Grant
     * Kinesis Data Firehose Access to an Amazon S3 Destination</a> and <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetRoleARN(Aws::String&& value) { m_roleARNHasBeenSet = true; m_roleARN = std::move(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the IAM role to be assumed by Kinesis Data
     * Firehose for calling the Amazon ES Configuration API and for indexing documents.
     * For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/controlling-access.html#using-iam-s3">Grant
     * Kinesis Data Firehose Access to an Amazon S3 Destination</a> and <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetRoleARN(const char* value) { m_roleARNHasBeenSet = true; m_roleARN.assign(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the IAM role to be assumed by Kinesis Data
     * Firehose for calling the Amazon ES Configuration API and for indexing documents.
     * For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/controlling-access.html#using-iam-s3">Grant
     * Kinesis Data Firehose Access to an Amazon S3 Destination</a> and <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline ElasticsearchDestinationUpdate& WithRoleARN(const Aws::String& value) { SetRoleARN(value); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the IAM role to be assumed by Kinesis Data
     * Firehose for calling the Amazon ES Configuration API and for indexing documents.
     * For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/controlling-access.html#using-iam-s3">Grant
     * Kinesis Data Firehose Access to an Amazon S3 Destination</a> and <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline ElasticsearchDestinationUpdate& WithRoleARN(Aws::String&& value) { SetRoleARN(std::move(value)); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the IAM role to be assumed by Kinesis Data
     * Firehose for calling the Amazon ES Configuration API and for indexing documents.
     * For more information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/controlling-access.html#using-iam-s3">Grant
     * Kinesis Data Firehose Access to an Amazon S3 Destination</a> and <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline ElasticsearchDestinationUpdate& WithRoleARN(const char* value) { SetRoleARN(value); return *this;}


    /**
     * <p>The ARN of the Amazon ES domain. The IAM role must have permissions
     * for <code>DescribeElasticsearchDomain</code>,
     * <code>DescribeElasticsearchDomains</code>, and
     * <code>DescribeElasticsearchDomainConfig</code> after assuming the IAM role
     * specified in <code>RoleARN</code>. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline const Aws::String& GetDomainARN() const{ return m_domainARN; }

    /**
     * <p>The ARN of the Amazon ES domain. The IAM role must have permissions
     * for <code>DescribeElasticsearchDomain</code>,
     * <code>DescribeElasticsearchDomains</code>, and
     * <code>DescribeElasticsearchDomainConfig</code> after assuming the IAM role
     * specified in <code>RoleARN</code>. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetDomainARN(const Aws::String& value) { m_domainARNHasBeenSet = true; m_domainARN = value; }

    /**
     * <p>The ARN of the Amazon ES domain. The IAM role must have permissions
     * for <code>DescribeElasticsearchDomain</code>,
     * <code>DescribeElasticsearchDomains</code>, and
     * <code>DescribeElasticsearchDomainConfig</code> after assuming the IAM role
     * specified in <code>RoleARN</code>. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetDomainARN(Aws::String&& value) { m_domainARNHasBeenSet = true; m_domainARN = std::move(value); }

    /**
     * <p>The ARN of the Amazon ES domain. The IAM role must have permissions
     * for <code>DescribeElasticsearchDomain</code>,
     * <code>DescribeElasticsearchDomains</code>, and
     * <code>DescribeElasticsearchDomainConfig</code> after assuming the IAM role
     * specified in <code>RoleARN</code>. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetDomainARN(const char* value) { m_domainARNHasBeenSet = true; m_domainARN.assign(value); }

    /**
     * <p>The ARN of the Amazon ES domain. The IAM role must have permissions
     * for <code>DescribeElasticsearchDomain</code>,
     * <code>DescribeElasticsearchDomains</code>, and
     * <code>DescribeElasticsearchDomainConfig</code> after assuming the IAM role
     * specified in <code>RoleARN</code>. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline ElasticsearchDestinationUpdate& WithDomainARN(const Aws::String& value) { SetDomainARN(value); return *this;}

    /**
     * <p>The ARN of the Amazon ES domain. The IAM role must have permissions
     * for <code>DescribeElasticsearchDomain</code>,
     * <code>DescribeElasticsearchDomains</code>, and
     * <code>DescribeElasticsearchDomainConfig</code> after assuming the IAM role
     * specified in <code>RoleARN</code>. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline ElasticsearchDestinationUpdate& WithDomainARN(Aws::String&& value) { SetDomainARN(std::move(value)); return *this;}

    /**
     * <p>The ARN of the Amazon ES domain. The IAM role must have permissions
     * for <code>DescribeElasticsearchDomain</code>,
     * <code>DescribeElasticsearchDomains</code>, and
     * <code>DescribeElasticsearchDomainConfig</code> after assuming the IAM role
     * specified in <code>RoleARN</code>. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline ElasticsearchDestinationUpdate& WithDomainARN(const char* value) { SetDomainARN(value); return *this;}


    /**
     * <p>The Elasticsearch index name.</p>
     */
    inline const Aws::String& GetIndexName() const{ return m_indexName; }

    /**
     * <p>The Elasticsearch index name.</p>
     */
    inline void SetIndexName(const Aws::String& value) { m_indexNameHasBeenSet = true; m_indexName = value; }

    /**
     * <p>The Elasticsearch index name.</p>
     */
    inline void SetIndexName(Aws::String&& value) { m_indexNameHasBeenSet = true; m_indexName = std::move(value); }

    /**
     * <p>The Elasticsearch index name.</p>
     */
    inline void SetIndexName(const char* value) { m_indexNameHasBeenSet = true; m_indexName.assign(value); }

    /**
     * <p>The Elasticsearch index name.</p>
     */
    inline ElasticsearchDestinationUpdate& WithIndexName(const Aws::String& value) { SetIndexName(value); return *this;}

    /**
     * <p>The Elasticsearch index name.</p>
     */
    inline ElasticsearchDestinationUpdate& WithIndexName(Aws::String&& value) { SetIndexName(std::move(value)); return *this;}

    /**
     * <p>The Elasticsearch index name.</p>
     */
    inline ElasticsearchDestinationUpdate& WithIndexName(const char* value) { SetIndexName(value); return *this;}


    /**
     * <p>The Elasticsearch type name. For Elasticsearch 6.x, there can be only one
     * type per index. If you try to specify a new type for an existing index that
     * already has another type, Kinesis Data Firehose returns an error during
     * runtime.</p>
     */
    inline const Aws::String& GetTypeName() const{ return m_typeName; }

    /**
     * <p>The Elasticsearch type name. For Elasticsearch 6.x, there can be only one
     * type per index. If you try to specify a new type for an existing index that
     * already has another type, Kinesis Data Firehose returns an error during
     * runtime.</p>
     */
    inline void SetTypeName(const Aws::String& value) { m_typeNameHasBeenSet = true; m_typeName = value; }

    /**
     * <p>The Elasticsearch type name. For Elasticsearch 6.x, there can be only one
     * type per index. If you try to specify a new type for an existing index that
     * already has another type, Kinesis Data Firehose returns an error during
     * runtime.</p>
     */
    inline void SetTypeName(Aws::String&& value) { m_typeNameHasBeenSet = true; m_typeName = std::move(value); }

    /**
     * <p>The Elasticsearch type name. For Elasticsearch 6.x, there can be only one
     * type per index. If you try to specify a new type for an existing index that
     * already has another type, Kinesis Data Firehose returns an error during
     * runtime.</p>
     */
    inline void SetTypeName(const char* value) { m_typeNameHasBeenSet = true; m_typeName.assign(value); }

    /**
     * <p>The Elasticsearch type name. For Elasticsearch 6.x, there can be only one
     * type per index. If you try to specify a new type for an existing index that
     * already has another type, Kinesis Data Firehose returns an error during
     * runtime.</p>
     */
    inline ElasticsearchDestinationUpdate& WithTypeName(const Aws::String& value) { SetTypeName(value); return *this;}

    /**
     * <p>The Elasticsearch type name. For Elasticsearch 6.x, there can be only one
     * type per index. If you try to specify a new type for an existing index that
     * already has another type, Kinesis Data Firehose returns an error during
     * runtime.</p>
     */
    inline ElasticsearchDestinationUpdate& WithTypeName(Aws::String&& value) { SetTypeName(std::move(value)); return *this;}

    /**
     * <p>The Elasticsearch type name. For Elasticsearch 6.x, there can be only one
     * type per index. If you try to specify a new type for an existing index that
     * already has another type, Kinesis Data Firehose returns an error during
     * runtime.</p>
     */
    inline ElasticsearchDestinationUpdate& WithTypeName(const char* value) { SetTypeName(value); return *this;}


    /**
     * <p>The Elasticsearch index rotation period. Index rotation appends a timestamp
     * to <code>IndexName</code> to facilitate the expiration of old data. For more
     * information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/basic-deliver.html#es-index-rotation">Index
     * Rotation for the Amazon ES Destination</a>. Default value
     * is <code>OneDay</code>.</p>
     */
    inline const ElasticsearchIndexRotationPeriod& GetIndexRotationPeriod() const{ return m_indexRotationPeriod; }

    /**
     * <p>The Elasticsearch index rotation period. Index rotation appends a timestamp
     * to <code>IndexName</code> to facilitate the expiration of old data. For more
     * information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/basic-deliver.html#es-index-rotation">Index
     * Rotation for the Amazon ES Destination</a>. Default value
     * is <code>OneDay</code>.</p>
     */
    inline void SetIndexRotationPeriod(const ElasticsearchIndexRotationPeriod& value) { m_indexRotationPeriodHasBeenSet = true; m_indexRotationPeriod = value; }

    /**
     * <p>The Elasticsearch index rotation period. Index rotation appends a timestamp
     * to <code>IndexName</code> to facilitate the expiration of old data. For more
     * information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/basic-deliver.html#es-index-rotation">Index
     * Rotation for the Amazon ES Destination</a>. Default value
     * is <code>OneDay</code>.</p>
     */
    inline void SetIndexRotationPeriod(ElasticsearchIndexRotationPeriod&& value) { m_indexRotationPeriodHasBeenSet = true; m_indexRotationPeriod = std::move(value); }

    /**
     * <p>The Elasticsearch index rotation period. Index rotation appends a timestamp
     * to <code>IndexName</code> to facilitate the expiration of old data. For more
     * information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/basic-deliver.html#es-index-rotation">Index
     * Rotation for the Amazon ES Destination</a>. Default value
     * is <code>OneDay</code>.</p>
     */
    inline ElasticsearchDestinationUpdate& WithIndexRotationPeriod(const ElasticsearchIndexRotationPeriod& value) { SetIndexRotationPeriod(value); return *this;}

    /**
     * <p>The Elasticsearch index rotation period. Index rotation appends a timestamp
     * to <code>IndexName</code> to facilitate the expiration of old data. For more
     * information, see <a
     * href="http://docs.aws.amazon.com/firehose/latest/dev/basic-deliver.html#es-index-rotation">Index
     * Rotation for the Amazon ES Destination</a>. Default value
     * is <code>OneDay</code>.</p>
     */
    inline ElasticsearchDestinationUpdate& WithIndexRotationPeriod(ElasticsearchIndexRotationPeriod&& value) { SetIndexRotationPeriod(std::move(value)); return *this;}


    /**
     * <p>The buffering options. If no value is specified,
     * <code>ElasticsearchBufferingHints</code> object default values are used. </p>
     */
    inline const ElasticsearchBufferingHints& GetBufferingHints() const{ return m_bufferingHints; }

    /**
     * <p>The buffering options. If no value is specified,
     * <code>ElasticsearchBufferingHints</code> object default values are used. </p>
     */
    inline void SetBufferingHints(const ElasticsearchBufferingHints& value) { m_bufferingHintsHasBeenSet = true; m_bufferingHints = value; }

    /**
     * <p>The buffering options. If no value is specified,
     * <code>ElasticsearchBufferingHints</code> object default values are used. </p>
     */
    inline void SetBufferingHints(ElasticsearchBufferingHints&& value) { m_bufferingHintsHasBeenSet = true; m_bufferingHints = std::move(value); }

    /**
     * <p>The buffering options. If no value is specified,
     * <code>ElasticsearchBufferingHints</code> object default values are used. </p>
     */
    inline ElasticsearchDestinationUpdate& WithBufferingHints(const ElasticsearchBufferingHints& value) { SetBufferingHints(value); return *this;}

    /**
     * <p>The buffering options. If no value is specified,
     * <code>ElasticsearchBufferingHints</code> object default values are used. </p>
     */
    inline ElasticsearchDestinationUpdate& WithBufferingHints(ElasticsearchBufferingHints&& value) { SetBufferingHints(std::move(value)); return *this;}


    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver
     * documents to Amazon ES. The default value is 300 (5 minutes).</p>
     */
    inline const ElasticsearchRetryOptions& GetRetryOptions() const{ return m_retryOptions; }

    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver
     * documents to Amazon ES. The default value is 300 (5 minutes).</p>
     */
    inline void SetRetryOptions(const ElasticsearchRetryOptions& value) { m_retryOptionsHasBeenSet = true; m_retryOptions = value; }

    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver
     * documents to Amazon ES. The default value is 300 (5 minutes).</p>
     */
    inline void SetRetryOptions(ElasticsearchRetryOptions&& value) { m_retryOptionsHasBeenSet = true; m_retryOptions = std::move(value); }

    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver
     * documents to Amazon ES. The default value is 300 (5 minutes).</p>
     */
    inline ElasticsearchDestinationUpdate& WithRetryOptions(const ElasticsearchRetryOptions& value) { SetRetryOptions(value); return *this;}

    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver
     * documents to Amazon ES. The default value is 300 (5 minutes).</p>
     */
    inline ElasticsearchDestinationUpdate& WithRetryOptions(ElasticsearchRetryOptions&& value) { SetRetryOptions(std::move(value)); return *this;}


    /**
     * <p>The Amazon S3 destination.</p>
     */
    inline const S3DestinationUpdate& GetS3Update() const{ return m_s3Update; }

    /**
     * <p>The Amazon S3 destination.</p>
     */
    inline void SetS3Update(const S3DestinationUpdate& value) { m_s3UpdateHasBeenSet = true; m_s3Update = value; }

    /**
     * <p>The Amazon S3 destination.</p>
     */
    inline void SetS3Update(S3DestinationUpdate&& value) { m_s3UpdateHasBeenSet = true; m_s3Update = std::move(value); }

    /**
     * <p>The Amazon S3 destination.</p>
     */
    inline ElasticsearchDestinationUpdate& WithS3Update(const S3DestinationUpdate& value) { SetS3Update(value); return *this;}

    /**
     * <p>The Amazon S3 destination.</p>
     */
    inline ElasticsearchDestinationUpdate& WithS3Update(S3DestinationUpdate&& value) { SetS3Update(std::move(value)); return *this;}


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
    inline ElasticsearchDestinationUpdate& WithProcessingConfiguration(const ProcessingConfiguration& value) { SetProcessingConfiguration(value); return *this;}

    /**
     * <p>The data processing configuration.</p>
     */
    inline ElasticsearchDestinationUpdate& WithProcessingConfiguration(ProcessingConfiguration&& value) { SetProcessingConfiguration(std::move(value)); return *this;}


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
    inline ElasticsearchDestinationUpdate& WithCloudWatchLoggingOptions(const CloudWatchLoggingOptions& value) { SetCloudWatchLoggingOptions(value); return *this;}

    /**
     * <p>The CloudWatch logging options for your delivery stream.</p>
     */
    inline ElasticsearchDestinationUpdate& WithCloudWatchLoggingOptions(CloudWatchLoggingOptions&& value) { SetCloudWatchLoggingOptions(std::move(value)); return *this;}

  private:

    Aws::String m_roleARN;
    bool m_roleARNHasBeenSet;

    Aws::String m_domainARN;
    bool m_domainARNHasBeenSet;

    Aws::String m_indexName;
    bool m_indexNameHasBeenSet;

    Aws::String m_typeName;
    bool m_typeNameHasBeenSet;

    ElasticsearchIndexRotationPeriod m_indexRotationPeriod;
    bool m_indexRotationPeriodHasBeenSet;

    ElasticsearchBufferingHints m_bufferingHints;
    bool m_bufferingHintsHasBeenSet;

    ElasticsearchRetryOptions m_retryOptions;
    bool m_retryOptionsHasBeenSet;

    S3DestinationUpdate m_s3Update;
    bool m_s3UpdateHasBeenSet;

    ProcessingConfiguration m_processingConfiguration;
    bool m_processingConfigurationHasBeenSet;

    CloudWatchLoggingOptions m_cloudWatchLoggingOptions;
    bool m_cloudWatchLoggingOptionsHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
