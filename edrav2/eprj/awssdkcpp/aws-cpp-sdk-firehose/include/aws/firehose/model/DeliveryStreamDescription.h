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
#include <aws/firehose/model/DeliveryStreamStatus.h>
#include <aws/firehose/model/DeliveryStreamEncryptionConfiguration.h>
#include <aws/firehose/model/DeliveryStreamType.h>
#include <aws/core/utils/DateTime.h>
#include <aws/firehose/model/SourceDescription.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/firehose/model/DestinationDescription.h>
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
   * <p>Contains information about a delivery stream.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/DeliveryStreamDescription">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API DeliveryStreamDescription
  {
  public:
    DeliveryStreamDescription();
    DeliveryStreamDescription(Aws::Utils::Json::JsonView jsonValue);
    DeliveryStreamDescription& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>The name of the delivery stream.</p>
     */
    inline const Aws::String& GetDeliveryStreamName() const{ return m_deliveryStreamName; }

    /**
     * <p>The name of the delivery stream.</p>
     */
    inline void SetDeliveryStreamName(const Aws::String& value) { m_deliveryStreamNameHasBeenSet = true; m_deliveryStreamName = value; }

    /**
     * <p>The name of the delivery stream.</p>
     */
    inline void SetDeliveryStreamName(Aws::String&& value) { m_deliveryStreamNameHasBeenSet = true; m_deliveryStreamName = std::move(value); }

    /**
     * <p>The name of the delivery stream.</p>
     */
    inline void SetDeliveryStreamName(const char* value) { m_deliveryStreamNameHasBeenSet = true; m_deliveryStreamName.assign(value); }

    /**
     * <p>The name of the delivery stream.</p>
     */
    inline DeliveryStreamDescription& WithDeliveryStreamName(const Aws::String& value) { SetDeliveryStreamName(value); return *this;}

    /**
     * <p>The name of the delivery stream.</p>
     */
    inline DeliveryStreamDescription& WithDeliveryStreamName(Aws::String&& value) { SetDeliveryStreamName(std::move(value)); return *this;}

    /**
     * <p>The name of the delivery stream.</p>
     */
    inline DeliveryStreamDescription& WithDeliveryStreamName(const char* value) { SetDeliveryStreamName(value); return *this;}


    /**
     * <p>The Amazon Resource Name (ARN) of the delivery stream. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline const Aws::String& GetDeliveryStreamARN() const{ return m_deliveryStreamARN; }

    /**
     * <p>The Amazon Resource Name (ARN) of the delivery stream. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetDeliveryStreamARN(const Aws::String& value) { m_deliveryStreamARNHasBeenSet = true; m_deliveryStreamARN = value; }

    /**
     * <p>The Amazon Resource Name (ARN) of the delivery stream. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetDeliveryStreamARN(Aws::String&& value) { m_deliveryStreamARNHasBeenSet = true; m_deliveryStreamARN = std::move(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the delivery stream. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetDeliveryStreamARN(const char* value) { m_deliveryStreamARNHasBeenSet = true; m_deliveryStreamARN.assign(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the delivery stream. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline DeliveryStreamDescription& WithDeliveryStreamARN(const Aws::String& value) { SetDeliveryStreamARN(value); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the delivery stream. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline DeliveryStreamDescription& WithDeliveryStreamARN(Aws::String&& value) { SetDeliveryStreamARN(std::move(value)); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the delivery stream. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline DeliveryStreamDescription& WithDeliveryStreamARN(const char* value) { SetDeliveryStreamARN(value); return *this;}


    /**
     * <p>The status of the delivery stream.</p>
     */
    inline const DeliveryStreamStatus& GetDeliveryStreamStatus() const{ return m_deliveryStreamStatus; }

    /**
     * <p>The status of the delivery stream.</p>
     */
    inline void SetDeliveryStreamStatus(const DeliveryStreamStatus& value) { m_deliveryStreamStatusHasBeenSet = true; m_deliveryStreamStatus = value; }

    /**
     * <p>The status of the delivery stream.</p>
     */
    inline void SetDeliveryStreamStatus(DeliveryStreamStatus&& value) { m_deliveryStreamStatusHasBeenSet = true; m_deliveryStreamStatus = std::move(value); }

    /**
     * <p>The status of the delivery stream.</p>
     */
    inline DeliveryStreamDescription& WithDeliveryStreamStatus(const DeliveryStreamStatus& value) { SetDeliveryStreamStatus(value); return *this;}

    /**
     * <p>The status of the delivery stream.</p>
     */
    inline DeliveryStreamDescription& WithDeliveryStreamStatus(DeliveryStreamStatus&& value) { SetDeliveryStreamStatus(std::move(value)); return *this;}


    /**
     * <p>Indicates the server-side encryption (SSE) status for the delivery
     * stream.</p>
     */
    inline const DeliveryStreamEncryptionConfiguration& GetDeliveryStreamEncryptionConfiguration() const{ return m_deliveryStreamEncryptionConfiguration; }

    /**
     * <p>Indicates the server-side encryption (SSE) status for the delivery
     * stream.</p>
     */
    inline void SetDeliveryStreamEncryptionConfiguration(const DeliveryStreamEncryptionConfiguration& value) { m_deliveryStreamEncryptionConfigurationHasBeenSet = true; m_deliveryStreamEncryptionConfiguration = value; }

    /**
     * <p>Indicates the server-side encryption (SSE) status for the delivery
     * stream.</p>
     */
    inline void SetDeliveryStreamEncryptionConfiguration(DeliveryStreamEncryptionConfiguration&& value) { m_deliveryStreamEncryptionConfigurationHasBeenSet = true; m_deliveryStreamEncryptionConfiguration = std::move(value); }

    /**
     * <p>Indicates the server-side encryption (SSE) status for the delivery
     * stream.</p>
     */
    inline DeliveryStreamDescription& WithDeliveryStreamEncryptionConfiguration(const DeliveryStreamEncryptionConfiguration& value) { SetDeliveryStreamEncryptionConfiguration(value); return *this;}

    /**
     * <p>Indicates the server-side encryption (SSE) status for the delivery
     * stream.</p>
     */
    inline DeliveryStreamDescription& WithDeliveryStreamEncryptionConfiguration(DeliveryStreamEncryptionConfiguration&& value) { SetDeliveryStreamEncryptionConfiguration(std::move(value)); return *this;}


    /**
     * <p>The delivery stream type. This can be one of the following values:</p> <ul>
     * <li> <p> <code>DirectPut</code>: Provider applications access the delivery
     * stream directly.</p> </li> <li> <p> <code>KinesisStreamAsSource</code>: The
     * delivery stream uses a Kinesis data stream as a source.</p> </li> </ul>
     */
    inline const DeliveryStreamType& GetDeliveryStreamType() const{ return m_deliveryStreamType; }

    /**
     * <p>The delivery stream type. This can be one of the following values:</p> <ul>
     * <li> <p> <code>DirectPut</code>: Provider applications access the delivery
     * stream directly.</p> </li> <li> <p> <code>KinesisStreamAsSource</code>: The
     * delivery stream uses a Kinesis data stream as a source.</p> </li> </ul>
     */
    inline void SetDeliveryStreamType(const DeliveryStreamType& value) { m_deliveryStreamTypeHasBeenSet = true; m_deliveryStreamType = value; }

    /**
     * <p>The delivery stream type. This can be one of the following values:</p> <ul>
     * <li> <p> <code>DirectPut</code>: Provider applications access the delivery
     * stream directly.</p> </li> <li> <p> <code>KinesisStreamAsSource</code>: The
     * delivery stream uses a Kinesis data stream as a source.</p> </li> </ul>
     */
    inline void SetDeliveryStreamType(DeliveryStreamType&& value) { m_deliveryStreamTypeHasBeenSet = true; m_deliveryStreamType = std::move(value); }

    /**
     * <p>The delivery stream type. This can be one of the following values:</p> <ul>
     * <li> <p> <code>DirectPut</code>: Provider applications access the delivery
     * stream directly.</p> </li> <li> <p> <code>KinesisStreamAsSource</code>: The
     * delivery stream uses a Kinesis data stream as a source.</p> </li> </ul>
     */
    inline DeliveryStreamDescription& WithDeliveryStreamType(const DeliveryStreamType& value) { SetDeliveryStreamType(value); return *this;}

    /**
     * <p>The delivery stream type. This can be one of the following values:</p> <ul>
     * <li> <p> <code>DirectPut</code>: Provider applications access the delivery
     * stream directly.</p> </li> <li> <p> <code>KinesisStreamAsSource</code>: The
     * delivery stream uses a Kinesis data stream as a source.</p> </li> </ul>
     */
    inline DeliveryStreamDescription& WithDeliveryStreamType(DeliveryStreamType&& value) { SetDeliveryStreamType(std::move(value)); return *this;}


    /**
     * <p>Each time the destination is updated for a delivery stream, the version ID is
     * changed, and the current version ID is required when updating the destination.
     * This is so that the service knows it is applying the changes to the correct
     * version of the delivery stream.</p>
     */
    inline const Aws::String& GetVersionId() const{ return m_versionId; }

    /**
     * <p>Each time the destination is updated for a delivery stream, the version ID is
     * changed, and the current version ID is required when updating the destination.
     * This is so that the service knows it is applying the changes to the correct
     * version of the delivery stream.</p>
     */
    inline void SetVersionId(const Aws::String& value) { m_versionIdHasBeenSet = true; m_versionId = value; }

    /**
     * <p>Each time the destination is updated for a delivery stream, the version ID is
     * changed, and the current version ID is required when updating the destination.
     * This is so that the service knows it is applying the changes to the correct
     * version of the delivery stream.</p>
     */
    inline void SetVersionId(Aws::String&& value) { m_versionIdHasBeenSet = true; m_versionId = std::move(value); }

    /**
     * <p>Each time the destination is updated for a delivery stream, the version ID is
     * changed, and the current version ID is required when updating the destination.
     * This is so that the service knows it is applying the changes to the correct
     * version of the delivery stream.</p>
     */
    inline void SetVersionId(const char* value) { m_versionIdHasBeenSet = true; m_versionId.assign(value); }

    /**
     * <p>Each time the destination is updated for a delivery stream, the version ID is
     * changed, and the current version ID is required when updating the destination.
     * This is so that the service knows it is applying the changes to the correct
     * version of the delivery stream.</p>
     */
    inline DeliveryStreamDescription& WithVersionId(const Aws::String& value) { SetVersionId(value); return *this;}

    /**
     * <p>Each time the destination is updated for a delivery stream, the version ID is
     * changed, and the current version ID is required when updating the destination.
     * This is so that the service knows it is applying the changes to the correct
     * version of the delivery stream.</p>
     */
    inline DeliveryStreamDescription& WithVersionId(Aws::String&& value) { SetVersionId(std::move(value)); return *this;}

    /**
     * <p>Each time the destination is updated for a delivery stream, the version ID is
     * changed, and the current version ID is required when updating the destination.
     * This is so that the service knows it is applying the changes to the correct
     * version of the delivery stream.</p>
     */
    inline DeliveryStreamDescription& WithVersionId(const char* value) { SetVersionId(value); return *this;}


    /**
     * <p>The date and time that the delivery stream was created.</p>
     */
    inline const Aws::Utils::DateTime& GetCreateTimestamp() const{ return m_createTimestamp; }

    /**
     * <p>The date and time that the delivery stream was created.</p>
     */
    inline void SetCreateTimestamp(const Aws::Utils::DateTime& value) { m_createTimestampHasBeenSet = true; m_createTimestamp = value; }

    /**
     * <p>The date and time that the delivery stream was created.</p>
     */
    inline void SetCreateTimestamp(Aws::Utils::DateTime&& value) { m_createTimestampHasBeenSet = true; m_createTimestamp = std::move(value); }

    /**
     * <p>The date and time that the delivery stream was created.</p>
     */
    inline DeliveryStreamDescription& WithCreateTimestamp(const Aws::Utils::DateTime& value) { SetCreateTimestamp(value); return *this;}

    /**
     * <p>The date and time that the delivery stream was created.</p>
     */
    inline DeliveryStreamDescription& WithCreateTimestamp(Aws::Utils::DateTime&& value) { SetCreateTimestamp(std::move(value)); return *this;}


    /**
     * <p>The date and time that the delivery stream was last updated.</p>
     */
    inline const Aws::Utils::DateTime& GetLastUpdateTimestamp() const{ return m_lastUpdateTimestamp; }

    /**
     * <p>The date and time that the delivery stream was last updated.</p>
     */
    inline void SetLastUpdateTimestamp(const Aws::Utils::DateTime& value) { m_lastUpdateTimestampHasBeenSet = true; m_lastUpdateTimestamp = value; }

    /**
     * <p>The date and time that the delivery stream was last updated.</p>
     */
    inline void SetLastUpdateTimestamp(Aws::Utils::DateTime&& value) { m_lastUpdateTimestampHasBeenSet = true; m_lastUpdateTimestamp = std::move(value); }

    /**
     * <p>The date and time that the delivery stream was last updated.</p>
     */
    inline DeliveryStreamDescription& WithLastUpdateTimestamp(const Aws::Utils::DateTime& value) { SetLastUpdateTimestamp(value); return *this;}

    /**
     * <p>The date and time that the delivery stream was last updated.</p>
     */
    inline DeliveryStreamDescription& WithLastUpdateTimestamp(Aws::Utils::DateTime&& value) { SetLastUpdateTimestamp(std::move(value)); return *this;}


    /**
     * <p>If the <code>DeliveryStreamType</code> parameter is
     * <code>KinesisStreamAsSource</code>, a <a>SourceDescription</a> object describing
     * the source Kinesis data stream.</p>
     */
    inline const SourceDescription& GetSource() const{ return m_source; }

    /**
     * <p>If the <code>DeliveryStreamType</code> parameter is
     * <code>KinesisStreamAsSource</code>, a <a>SourceDescription</a> object describing
     * the source Kinesis data stream.</p>
     */
    inline void SetSource(const SourceDescription& value) { m_sourceHasBeenSet = true; m_source = value; }

    /**
     * <p>If the <code>DeliveryStreamType</code> parameter is
     * <code>KinesisStreamAsSource</code>, a <a>SourceDescription</a> object describing
     * the source Kinesis data stream.</p>
     */
    inline void SetSource(SourceDescription&& value) { m_sourceHasBeenSet = true; m_source = std::move(value); }

    /**
     * <p>If the <code>DeliveryStreamType</code> parameter is
     * <code>KinesisStreamAsSource</code>, a <a>SourceDescription</a> object describing
     * the source Kinesis data stream.</p>
     */
    inline DeliveryStreamDescription& WithSource(const SourceDescription& value) { SetSource(value); return *this;}

    /**
     * <p>If the <code>DeliveryStreamType</code> parameter is
     * <code>KinesisStreamAsSource</code>, a <a>SourceDescription</a> object describing
     * the source Kinesis data stream.</p>
     */
    inline DeliveryStreamDescription& WithSource(SourceDescription&& value) { SetSource(std::move(value)); return *this;}


    /**
     * <p>The destinations.</p>
     */
    inline const Aws::Vector<DestinationDescription>& GetDestinations() const{ return m_destinations; }

    /**
     * <p>The destinations.</p>
     */
    inline void SetDestinations(const Aws::Vector<DestinationDescription>& value) { m_destinationsHasBeenSet = true; m_destinations = value; }

    /**
     * <p>The destinations.</p>
     */
    inline void SetDestinations(Aws::Vector<DestinationDescription>&& value) { m_destinationsHasBeenSet = true; m_destinations = std::move(value); }

    /**
     * <p>The destinations.</p>
     */
    inline DeliveryStreamDescription& WithDestinations(const Aws::Vector<DestinationDescription>& value) { SetDestinations(value); return *this;}

    /**
     * <p>The destinations.</p>
     */
    inline DeliveryStreamDescription& WithDestinations(Aws::Vector<DestinationDescription>&& value) { SetDestinations(std::move(value)); return *this;}

    /**
     * <p>The destinations.</p>
     */
    inline DeliveryStreamDescription& AddDestinations(const DestinationDescription& value) { m_destinationsHasBeenSet = true; m_destinations.push_back(value); return *this; }

    /**
     * <p>The destinations.</p>
     */
    inline DeliveryStreamDescription& AddDestinations(DestinationDescription&& value) { m_destinationsHasBeenSet = true; m_destinations.push_back(std::move(value)); return *this; }


    /**
     * <p>Indicates whether there are more destinations available to list.</p>
     */
    inline bool GetHasMoreDestinations() const{ return m_hasMoreDestinations; }

    /**
     * <p>Indicates whether there are more destinations available to list.</p>
     */
    inline void SetHasMoreDestinations(bool value) { m_hasMoreDestinationsHasBeenSet = true; m_hasMoreDestinations = value; }

    /**
     * <p>Indicates whether there are more destinations available to list.</p>
     */
    inline DeliveryStreamDescription& WithHasMoreDestinations(bool value) { SetHasMoreDestinations(value); return *this;}

  private:

    Aws::String m_deliveryStreamName;
    bool m_deliveryStreamNameHasBeenSet;

    Aws::String m_deliveryStreamARN;
    bool m_deliveryStreamARNHasBeenSet;

    DeliveryStreamStatus m_deliveryStreamStatus;
    bool m_deliveryStreamStatusHasBeenSet;

    DeliveryStreamEncryptionConfiguration m_deliveryStreamEncryptionConfiguration;
    bool m_deliveryStreamEncryptionConfigurationHasBeenSet;

    DeliveryStreamType m_deliveryStreamType;
    bool m_deliveryStreamTypeHasBeenSet;

    Aws::String m_versionId;
    bool m_versionIdHasBeenSet;

    Aws::Utils::DateTime m_createTimestamp;
    bool m_createTimestampHasBeenSet;

    Aws::Utils::DateTime m_lastUpdateTimestamp;
    bool m_lastUpdateTimestampHasBeenSet;

    SourceDescription m_source;
    bool m_sourceHasBeenSet;

    Aws::Vector<DestinationDescription> m_destinations;
    bool m_destinationsHasBeenSet;

    bool m_hasMoreDestinations;
    bool m_hasMoreDestinationsHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
