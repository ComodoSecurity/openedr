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

#include <aws/firehose/model/DeliveryStreamDescription.h>
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

DeliveryStreamDescription::DeliveryStreamDescription() : 
    m_deliveryStreamNameHasBeenSet(false),
    m_deliveryStreamARNHasBeenSet(false),
    m_deliveryStreamStatus(DeliveryStreamStatus::NOT_SET),
    m_deliveryStreamStatusHasBeenSet(false),
    m_deliveryStreamEncryptionConfigurationHasBeenSet(false),
    m_deliveryStreamType(DeliveryStreamType::NOT_SET),
    m_deliveryStreamTypeHasBeenSet(false),
    m_versionIdHasBeenSet(false),
    m_createTimestampHasBeenSet(false),
    m_lastUpdateTimestampHasBeenSet(false),
    m_sourceHasBeenSet(false),
    m_destinationsHasBeenSet(false),
    m_hasMoreDestinations(false),
    m_hasMoreDestinationsHasBeenSet(false)
{
}

DeliveryStreamDescription::DeliveryStreamDescription(JsonView jsonValue) : 
    m_deliveryStreamNameHasBeenSet(false),
    m_deliveryStreamARNHasBeenSet(false),
    m_deliveryStreamStatus(DeliveryStreamStatus::NOT_SET),
    m_deliveryStreamStatusHasBeenSet(false),
    m_deliveryStreamEncryptionConfigurationHasBeenSet(false),
    m_deliveryStreamType(DeliveryStreamType::NOT_SET),
    m_deliveryStreamTypeHasBeenSet(false),
    m_versionIdHasBeenSet(false),
    m_createTimestampHasBeenSet(false),
    m_lastUpdateTimestampHasBeenSet(false),
    m_sourceHasBeenSet(false),
    m_destinationsHasBeenSet(false),
    m_hasMoreDestinations(false),
    m_hasMoreDestinationsHasBeenSet(false)
{
  *this = jsonValue;
}

DeliveryStreamDescription& DeliveryStreamDescription::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("DeliveryStreamName"))
  {
    m_deliveryStreamName = jsonValue.GetString("DeliveryStreamName");

    m_deliveryStreamNameHasBeenSet = true;
  }

  if(jsonValue.ValueExists("DeliveryStreamARN"))
  {
    m_deliveryStreamARN = jsonValue.GetString("DeliveryStreamARN");

    m_deliveryStreamARNHasBeenSet = true;
  }

  if(jsonValue.ValueExists("DeliveryStreamStatus"))
  {
    m_deliveryStreamStatus = DeliveryStreamStatusMapper::GetDeliveryStreamStatusForName(jsonValue.GetString("DeliveryStreamStatus"));

    m_deliveryStreamStatusHasBeenSet = true;
  }

  if(jsonValue.ValueExists("DeliveryStreamEncryptionConfiguration"))
  {
    m_deliveryStreamEncryptionConfiguration = jsonValue.GetObject("DeliveryStreamEncryptionConfiguration");

    m_deliveryStreamEncryptionConfigurationHasBeenSet = true;
  }

  if(jsonValue.ValueExists("DeliveryStreamType"))
  {
    m_deliveryStreamType = DeliveryStreamTypeMapper::GetDeliveryStreamTypeForName(jsonValue.GetString("DeliveryStreamType"));

    m_deliveryStreamTypeHasBeenSet = true;
  }

  if(jsonValue.ValueExists("VersionId"))
  {
    m_versionId = jsonValue.GetString("VersionId");

    m_versionIdHasBeenSet = true;
  }

  if(jsonValue.ValueExists("CreateTimestamp"))
  {
    m_createTimestamp = jsonValue.GetDouble("CreateTimestamp");

    m_createTimestampHasBeenSet = true;
  }

  if(jsonValue.ValueExists("LastUpdateTimestamp"))
  {
    m_lastUpdateTimestamp = jsonValue.GetDouble("LastUpdateTimestamp");

    m_lastUpdateTimestampHasBeenSet = true;
  }

  if(jsonValue.ValueExists("Source"))
  {
    m_source = jsonValue.GetObject("Source");

    m_sourceHasBeenSet = true;
  }

  if(jsonValue.ValueExists("Destinations"))
  {
    Array<JsonView> destinationsJsonList = jsonValue.GetArray("Destinations");
    for(unsigned destinationsIndex = 0; destinationsIndex < destinationsJsonList.GetLength(); ++destinationsIndex)
    {
      m_destinations.push_back(destinationsJsonList[destinationsIndex].AsObject());
    }
    m_destinationsHasBeenSet = true;
  }

  if(jsonValue.ValueExists("HasMoreDestinations"))
  {
    m_hasMoreDestinations = jsonValue.GetBool("HasMoreDestinations");

    m_hasMoreDestinationsHasBeenSet = true;
  }

  return *this;
}

JsonValue DeliveryStreamDescription::Jsonize() const
{
  JsonValue payload;

  if(m_deliveryStreamNameHasBeenSet)
  {
   payload.WithString("DeliveryStreamName", m_deliveryStreamName);

  }

  if(m_deliveryStreamARNHasBeenSet)
  {
   payload.WithString("DeliveryStreamARN", m_deliveryStreamARN);

  }

  if(m_deliveryStreamStatusHasBeenSet)
  {
   payload.WithString("DeliveryStreamStatus", DeliveryStreamStatusMapper::GetNameForDeliveryStreamStatus(m_deliveryStreamStatus));
  }

  if(m_deliveryStreamEncryptionConfigurationHasBeenSet)
  {
   payload.WithObject("DeliveryStreamEncryptionConfiguration", m_deliveryStreamEncryptionConfiguration.Jsonize());

  }

  if(m_deliveryStreamTypeHasBeenSet)
  {
   payload.WithString("DeliveryStreamType", DeliveryStreamTypeMapper::GetNameForDeliveryStreamType(m_deliveryStreamType));
  }

  if(m_versionIdHasBeenSet)
  {
   payload.WithString("VersionId", m_versionId);

  }

  if(m_createTimestampHasBeenSet)
  {
   payload.WithDouble("CreateTimestamp", m_createTimestamp.SecondsWithMSPrecision());
  }

  if(m_lastUpdateTimestampHasBeenSet)
  {
   payload.WithDouble("LastUpdateTimestamp", m_lastUpdateTimestamp.SecondsWithMSPrecision());
  }

  if(m_sourceHasBeenSet)
  {
   payload.WithObject("Source", m_source.Jsonize());

  }

  if(m_destinationsHasBeenSet)
  {
   Array<JsonValue> destinationsJsonList(m_destinations.size());
   for(unsigned destinationsIndex = 0; destinationsIndex < destinationsJsonList.GetLength(); ++destinationsIndex)
   {
     destinationsJsonList[destinationsIndex].AsObject(m_destinations[destinationsIndex].Jsonize());
   }
   payload.WithArray("Destinations", std::move(destinationsJsonList));

  }

  if(m_hasMoreDestinationsHasBeenSet)
  {
   payload.WithBool("HasMoreDestinations", m_hasMoreDestinations);

  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
