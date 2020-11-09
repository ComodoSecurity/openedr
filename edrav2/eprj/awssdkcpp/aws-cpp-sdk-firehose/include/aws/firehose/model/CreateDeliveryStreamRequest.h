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
#include <aws/firehose/FirehoseRequest.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/firehose/model/DeliveryStreamType.h>
#include <aws/firehose/model/KinesisStreamSourceConfiguration.h>
#include <aws/firehose/model/ExtendedS3DestinationConfiguration.h>
#include <aws/firehose/model/RedshiftDestinationConfiguration.h>
#include <aws/firehose/model/ElasticsearchDestinationConfiguration.h>
#include <aws/firehose/model/SplunkDestinationConfiguration.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/firehose/model/Tag.h>
#include <utility>

namespace Aws
{
namespace Firehose
{
namespace Model
{

  /**
   */
  class AWS_FIREHOSE_API CreateDeliveryStreamRequest : public FirehoseRequest
  {
  public:
    CreateDeliveryStreamRequest();
    
    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "CreateDeliveryStream"; }

    Aws::String SerializePayload() const override;

    Aws::Http::HeaderValueCollection GetRequestSpecificHeaders() const override;


    /**
     * <p>The name of the delivery stream. This name must be unique per AWS account in
     * the same AWS Region. If the delivery streams are in different accounts or
     * different Regions, you can have multiple delivery streams with the same
     * name.</p>
     */
    inline const Aws::String& GetDeliveryStreamName() const{ return m_deliveryStreamName; }

    /**
     * <p>The name of the delivery stream. This name must be unique per AWS account in
     * the same AWS Region. If the delivery streams are in different accounts or
     * different Regions, you can have multiple delivery streams with the same
     * name.</p>
     */
    inline void SetDeliveryStreamName(const Aws::String& value) { m_deliveryStreamNameHasBeenSet = true; m_deliveryStreamName = value; }

    /**
     * <p>The name of the delivery stream. This name must be unique per AWS account in
     * the same AWS Region. If the delivery streams are in different accounts or
     * different Regions, you can have multiple delivery streams with the same
     * name.</p>
     */
    inline void SetDeliveryStreamName(Aws::String&& value) { m_deliveryStreamNameHasBeenSet = true; m_deliveryStreamName = std::move(value); }

    /**
     * <p>The name of the delivery stream. This name must be unique per AWS account in
     * the same AWS Region. If the delivery streams are in different accounts or
     * different Regions, you can have multiple delivery streams with the same
     * name.</p>
     */
    inline void SetDeliveryStreamName(const char* value) { m_deliveryStreamNameHasBeenSet = true; m_deliveryStreamName.assign(value); }

    /**
     * <p>The name of the delivery stream. This name must be unique per AWS account in
     * the same AWS Region. If the delivery streams are in different accounts or
     * different Regions, you can have multiple delivery streams with the same
     * name.</p>
     */
    inline CreateDeliveryStreamRequest& WithDeliveryStreamName(const Aws::String& value) { SetDeliveryStreamName(value); return *this;}

    /**
     * <p>The name of the delivery stream. This name must be unique per AWS account in
     * the same AWS Region. If the delivery streams are in different accounts or
     * different Regions, you can have multiple delivery streams with the same
     * name.</p>
     */
    inline CreateDeliveryStreamRequest& WithDeliveryStreamName(Aws::String&& value) { SetDeliveryStreamName(std::move(value)); return *this;}

    /**
     * <p>The name of the delivery stream. This name must be unique per AWS account in
     * the same AWS Region. If the delivery streams are in different accounts or
     * different Regions, you can have multiple delivery streams with the same
     * name.</p>
     */
    inline CreateDeliveryStreamRequest& WithDeliveryStreamName(const char* value) { SetDeliveryStreamName(value); return *this;}


    /**
     * <p>The delivery stream type. This parameter can be one of the following
     * values:</p> <ul> <li> <p> <code>DirectPut</code>: Provider applications access
     * the delivery stream directly.</p> </li> <li> <p>
     * <code>KinesisStreamAsSource</code>: The delivery stream uses a Kinesis data
     * stream as a source.</p> </li> </ul>
     */
    inline const DeliveryStreamType& GetDeliveryStreamType() const{ return m_deliveryStreamType; }

    /**
     * <p>The delivery stream type. This parameter can be one of the following
     * values:</p> <ul> <li> <p> <code>DirectPut</code>: Provider applications access
     * the delivery stream directly.</p> </li> <li> <p>
     * <code>KinesisStreamAsSource</code>: The delivery stream uses a Kinesis data
     * stream as a source.</p> </li> </ul>
     */
    inline void SetDeliveryStreamType(const DeliveryStreamType& value) { m_deliveryStreamTypeHasBeenSet = true; m_deliveryStreamType = value; }

    /**
     * <p>The delivery stream type. This parameter can be one of the following
     * values:</p> <ul> <li> <p> <code>DirectPut</code>: Provider applications access
     * the delivery stream directly.</p> </li> <li> <p>
     * <code>KinesisStreamAsSource</code>: The delivery stream uses a Kinesis data
     * stream as a source.</p> </li> </ul>
     */
    inline void SetDeliveryStreamType(DeliveryStreamType&& value) { m_deliveryStreamTypeHasBeenSet = true; m_deliveryStreamType = std::move(value); }

    /**
     * <p>The delivery stream type. This parameter can be one of the following
     * values:</p> <ul> <li> <p> <code>DirectPut</code>: Provider applications access
     * the delivery stream directly.</p> </li> <li> <p>
     * <code>KinesisStreamAsSource</code>: The delivery stream uses a Kinesis data
     * stream as a source.</p> </li> </ul>
     */
    inline CreateDeliveryStreamRequest& WithDeliveryStreamType(const DeliveryStreamType& value) { SetDeliveryStreamType(value); return *this;}

    /**
     * <p>The delivery stream type. This parameter can be one of the following
     * values:</p> <ul> <li> <p> <code>DirectPut</code>: Provider applications access
     * the delivery stream directly.</p> </li> <li> <p>
     * <code>KinesisStreamAsSource</code>: The delivery stream uses a Kinesis data
     * stream as a source.</p> </li> </ul>
     */
    inline CreateDeliveryStreamRequest& WithDeliveryStreamType(DeliveryStreamType&& value) { SetDeliveryStreamType(std::move(value)); return *this;}


    /**
     * <p>When a Kinesis data stream is used as the source for the delivery stream, a
     * <a>KinesisStreamSourceConfiguration</a> containing the Kinesis data stream
     * Amazon Resource Name (ARN) and the role ARN for the source stream.</p>
     */
    inline const KinesisStreamSourceConfiguration& GetKinesisStreamSourceConfiguration() const{ return m_kinesisStreamSourceConfiguration; }

    /**
     * <p>When a Kinesis data stream is used as the source for the delivery stream, a
     * <a>KinesisStreamSourceConfiguration</a> containing the Kinesis data stream
     * Amazon Resource Name (ARN) and the role ARN for the source stream.</p>
     */
    inline void SetKinesisStreamSourceConfiguration(const KinesisStreamSourceConfiguration& value) { m_kinesisStreamSourceConfigurationHasBeenSet = true; m_kinesisStreamSourceConfiguration = value; }

    /**
     * <p>When a Kinesis data stream is used as the source for the delivery stream, a
     * <a>KinesisStreamSourceConfiguration</a> containing the Kinesis data stream
     * Amazon Resource Name (ARN) and the role ARN for the source stream.</p>
     */
    inline void SetKinesisStreamSourceConfiguration(KinesisStreamSourceConfiguration&& value) { m_kinesisStreamSourceConfigurationHasBeenSet = true; m_kinesisStreamSourceConfiguration = std::move(value); }

    /**
     * <p>When a Kinesis data stream is used as the source for the delivery stream, a
     * <a>KinesisStreamSourceConfiguration</a> containing the Kinesis data stream
     * Amazon Resource Name (ARN) and the role ARN for the source stream.</p>
     */
    inline CreateDeliveryStreamRequest& WithKinesisStreamSourceConfiguration(const KinesisStreamSourceConfiguration& value) { SetKinesisStreamSourceConfiguration(value); return *this;}

    /**
     * <p>When a Kinesis data stream is used as the source for the delivery stream, a
     * <a>KinesisStreamSourceConfiguration</a> containing the Kinesis data stream
     * Amazon Resource Name (ARN) and the role ARN for the source stream.</p>
     */
    inline CreateDeliveryStreamRequest& WithKinesisStreamSourceConfiguration(KinesisStreamSourceConfiguration&& value) { SetKinesisStreamSourceConfiguration(std::move(value)); return *this;}


    /**
     * <p>The destination in Amazon S3. You can specify only one destination.</p>
     */
    inline const ExtendedS3DestinationConfiguration& GetExtendedS3DestinationConfiguration() const{ return m_extendedS3DestinationConfiguration; }

    /**
     * <p>The destination in Amazon S3. You can specify only one destination.</p>
     */
    inline void SetExtendedS3DestinationConfiguration(const ExtendedS3DestinationConfiguration& value) { m_extendedS3DestinationConfigurationHasBeenSet = true; m_extendedS3DestinationConfiguration = value; }

    /**
     * <p>The destination in Amazon S3. You can specify only one destination.</p>
     */
    inline void SetExtendedS3DestinationConfiguration(ExtendedS3DestinationConfiguration&& value) { m_extendedS3DestinationConfigurationHasBeenSet = true; m_extendedS3DestinationConfiguration = std::move(value); }

    /**
     * <p>The destination in Amazon S3. You can specify only one destination.</p>
     */
    inline CreateDeliveryStreamRequest& WithExtendedS3DestinationConfiguration(const ExtendedS3DestinationConfiguration& value) { SetExtendedS3DestinationConfiguration(value); return *this;}

    /**
     * <p>The destination in Amazon S3. You can specify only one destination.</p>
     */
    inline CreateDeliveryStreamRequest& WithExtendedS3DestinationConfiguration(ExtendedS3DestinationConfiguration&& value) { SetExtendedS3DestinationConfiguration(std::move(value)); return *this;}


    /**
     * <p>The destination in Amazon Redshift. You can specify only one destination.</p>
     */
    inline const RedshiftDestinationConfiguration& GetRedshiftDestinationConfiguration() const{ return m_redshiftDestinationConfiguration; }

    /**
     * <p>The destination in Amazon Redshift. You can specify only one destination.</p>
     */
    inline void SetRedshiftDestinationConfiguration(const RedshiftDestinationConfiguration& value) { m_redshiftDestinationConfigurationHasBeenSet = true; m_redshiftDestinationConfiguration = value; }

    /**
     * <p>The destination in Amazon Redshift. You can specify only one destination.</p>
     */
    inline void SetRedshiftDestinationConfiguration(RedshiftDestinationConfiguration&& value) { m_redshiftDestinationConfigurationHasBeenSet = true; m_redshiftDestinationConfiguration = std::move(value); }

    /**
     * <p>The destination in Amazon Redshift. You can specify only one destination.</p>
     */
    inline CreateDeliveryStreamRequest& WithRedshiftDestinationConfiguration(const RedshiftDestinationConfiguration& value) { SetRedshiftDestinationConfiguration(value); return *this;}

    /**
     * <p>The destination in Amazon Redshift. You can specify only one destination.</p>
     */
    inline CreateDeliveryStreamRequest& WithRedshiftDestinationConfiguration(RedshiftDestinationConfiguration&& value) { SetRedshiftDestinationConfiguration(std::move(value)); return *this;}


    /**
     * <p>The destination in Amazon ES. You can specify only one destination.</p>
     */
    inline const ElasticsearchDestinationConfiguration& GetElasticsearchDestinationConfiguration() const{ return m_elasticsearchDestinationConfiguration; }

    /**
     * <p>The destination in Amazon ES. You can specify only one destination.</p>
     */
    inline void SetElasticsearchDestinationConfiguration(const ElasticsearchDestinationConfiguration& value) { m_elasticsearchDestinationConfigurationHasBeenSet = true; m_elasticsearchDestinationConfiguration = value; }

    /**
     * <p>The destination in Amazon ES. You can specify only one destination.</p>
     */
    inline void SetElasticsearchDestinationConfiguration(ElasticsearchDestinationConfiguration&& value) { m_elasticsearchDestinationConfigurationHasBeenSet = true; m_elasticsearchDestinationConfiguration = std::move(value); }

    /**
     * <p>The destination in Amazon ES. You can specify only one destination.</p>
     */
    inline CreateDeliveryStreamRequest& WithElasticsearchDestinationConfiguration(const ElasticsearchDestinationConfiguration& value) { SetElasticsearchDestinationConfiguration(value); return *this;}

    /**
     * <p>The destination in Amazon ES. You can specify only one destination.</p>
     */
    inline CreateDeliveryStreamRequest& WithElasticsearchDestinationConfiguration(ElasticsearchDestinationConfiguration&& value) { SetElasticsearchDestinationConfiguration(std::move(value)); return *this;}


    /**
     * <p>The destination in Splunk. You can specify only one destination.</p>
     */
    inline const SplunkDestinationConfiguration& GetSplunkDestinationConfiguration() const{ return m_splunkDestinationConfiguration; }

    /**
     * <p>The destination in Splunk. You can specify only one destination.</p>
     */
    inline void SetSplunkDestinationConfiguration(const SplunkDestinationConfiguration& value) { m_splunkDestinationConfigurationHasBeenSet = true; m_splunkDestinationConfiguration = value; }

    /**
     * <p>The destination in Splunk. You can specify only one destination.</p>
     */
    inline void SetSplunkDestinationConfiguration(SplunkDestinationConfiguration&& value) { m_splunkDestinationConfigurationHasBeenSet = true; m_splunkDestinationConfiguration = std::move(value); }

    /**
     * <p>The destination in Splunk. You can specify only one destination.</p>
     */
    inline CreateDeliveryStreamRequest& WithSplunkDestinationConfiguration(const SplunkDestinationConfiguration& value) { SetSplunkDestinationConfiguration(value); return *this;}

    /**
     * <p>The destination in Splunk. You can specify only one destination.</p>
     */
    inline CreateDeliveryStreamRequest& WithSplunkDestinationConfiguration(SplunkDestinationConfiguration&& value) { SetSplunkDestinationConfiguration(std::move(value)); return *this;}


    /**
     * <p>A set of tags to assign to the delivery stream. A tag is a key-value pair
     * that you can define and assign to AWS resources. Tags are metadata. For example,
     * you can add friendly names and descriptions or other types of information that
     * can help you distinguish the delivery stream. For more information about tags,
     * see <a
     * href="https://docs.aws.amazon.com/awsaccountbilling/latest/aboutv2/cost-alloc-tags.html">Using
     * Cost Allocation Tags</a> in the AWS Billing and Cost Management User Guide.</p>
     * <p>You can specify up to 50 tags when creating a delivery stream.</p>
     */
    inline const Aws::Vector<Tag>& GetTags() const{ return m_tags; }

    /**
     * <p>A set of tags to assign to the delivery stream. A tag is a key-value pair
     * that you can define and assign to AWS resources. Tags are metadata. For example,
     * you can add friendly names and descriptions or other types of information that
     * can help you distinguish the delivery stream. For more information about tags,
     * see <a
     * href="https://docs.aws.amazon.com/awsaccountbilling/latest/aboutv2/cost-alloc-tags.html">Using
     * Cost Allocation Tags</a> in the AWS Billing and Cost Management User Guide.</p>
     * <p>You can specify up to 50 tags when creating a delivery stream.</p>
     */
    inline void SetTags(const Aws::Vector<Tag>& value) { m_tagsHasBeenSet = true; m_tags = value; }

    /**
     * <p>A set of tags to assign to the delivery stream. A tag is a key-value pair
     * that you can define and assign to AWS resources. Tags are metadata. For example,
     * you can add friendly names and descriptions or other types of information that
     * can help you distinguish the delivery stream. For more information about tags,
     * see <a
     * href="https://docs.aws.amazon.com/awsaccountbilling/latest/aboutv2/cost-alloc-tags.html">Using
     * Cost Allocation Tags</a> in the AWS Billing and Cost Management User Guide.</p>
     * <p>You can specify up to 50 tags when creating a delivery stream.</p>
     */
    inline void SetTags(Aws::Vector<Tag>&& value) { m_tagsHasBeenSet = true; m_tags = std::move(value); }

    /**
     * <p>A set of tags to assign to the delivery stream. A tag is a key-value pair
     * that you can define and assign to AWS resources. Tags are metadata. For example,
     * you can add friendly names and descriptions or other types of information that
     * can help you distinguish the delivery stream. For more information about tags,
     * see <a
     * href="https://docs.aws.amazon.com/awsaccountbilling/latest/aboutv2/cost-alloc-tags.html">Using
     * Cost Allocation Tags</a> in the AWS Billing and Cost Management User Guide.</p>
     * <p>You can specify up to 50 tags when creating a delivery stream.</p>
     */
    inline CreateDeliveryStreamRequest& WithTags(const Aws::Vector<Tag>& value) { SetTags(value); return *this;}

    /**
     * <p>A set of tags to assign to the delivery stream. A tag is a key-value pair
     * that you can define and assign to AWS resources. Tags are metadata. For example,
     * you can add friendly names and descriptions or other types of information that
     * can help you distinguish the delivery stream. For more information about tags,
     * see <a
     * href="https://docs.aws.amazon.com/awsaccountbilling/latest/aboutv2/cost-alloc-tags.html">Using
     * Cost Allocation Tags</a> in the AWS Billing and Cost Management User Guide.</p>
     * <p>You can specify up to 50 tags when creating a delivery stream.</p>
     */
    inline CreateDeliveryStreamRequest& WithTags(Aws::Vector<Tag>&& value) { SetTags(std::move(value)); return *this;}

    /**
     * <p>A set of tags to assign to the delivery stream. A tag is a key-value pair
     * that you can define and assign to AWS resources. Tags are metadata. For example,
     * you can add friendly names and descriptions or other types of information that
     * can help you distinguish the delivery stream. For more information about tags,
     * see <a
     * href="https://docs.aws.amazon.com/awsaccountbilling/latest/aboutv2/cost-alloc-tags.html">Using
     * Cost Allocation Tags</a> in the AWS Billing and Cost Management User Guide.</p>
     * <p>You can specify up to 50 tags when creating a delivery stream.</p>
     */
    inline CreateDeliveryStreamRequest& AddTags(const Tag& value) { m_tagsHasBeenSet = true; m_tags.push_back(value); return *this; }

    /**
     * <p>A set of tags to assign to the delivery stream. A tag is a key-value pair
     * that you can define and assign to AWS resources. Tags are metadata. For example,
     * you can add friendly names and descriptions or other types of information that
     * can help you distinguish the delivery stream. For more information about tags,
     * see <a
     * href="https://docs.aws.amazon.com/awsaccountbilling/latest/aboutv2/cost-alloc-tags.html">Using
     * Cost Allocation Tags</a> in the AWS Billing and Cost Management User Guide.</p>
     * <p>You can specify up to 50 tags when creating a delivery stream.</p>
     */
    inline CreateDeliveryStreamRequest& AddTags(Tag&& value) { m_tagsHasBeenSet = true; m_tags.push_back(std::move(value)); return *this; }

  private:

    Aws::String m_deliveryStreamName;
    bool m_deliveryStreamNameHasBeenSet;

    DeliveryStreamType m_deliveryStreamType;
    bool m_deliveryStreamTypeHasBeenSet;

    KinesisStreamSourceConfiguration m_kinesisStreamSourceConfiguration;
    bool m_kinesisStreamSourceConfigurationHasBeenSet;

    ExtendedS3DestinationConfiguration m_extendedS3DestinationConfiguration;
    bool m_extendedS3DestinationConfigurationHasBeenSet;

    RedshiftDestinationConfiguration m_redshiftDestinationConfiguration;
    bool m_redshiftDestinationConfigurationHasBeenSet;

    ElasticsearchDestinationConfiguration m_elasticsearchDestinationConfiguration;
    bool m_elasticsearchDestinationConfigurationHasBeenSet;

    SplunkDestinationConfiguration m_splunkDestinationConfiguration;
    bool m_splunkDestinationConfigurationHasBeenSet;

    Aws::Vector<Tag> m_tags;
    bool m_tagsHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
