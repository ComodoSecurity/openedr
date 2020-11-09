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

#include <aws/firehose/model/UpdateDestinationRequest.h>
#include <aws/core/utils/json/JsonSerializer.h>

#include <utility>

using namespace Aws::Firehose::Model;
using namespace Aws::Utils::Json;
using namespace Aws::Utils;

UpdateDestinationRequest::UpdateDestinationRequest() : 
    m_deliveryStreamNameHasBeenSet(false),
    m_currentDeliveryStreamVersionIdHasBeenSet(false),
    m_destinationIdHasBeenSet(false),
    m_extendedS3DestinationUpdateHasBeenSet(false),
    m_redshiftDestinationUpdateHasBeenSet(false),
    m_elasticsearchDestinationUpdateHasBeenSet(false),
    m_splunkDestinationUpdateHasBeenSet(false)
{
}

Aws::String UpdateDestinationRequest::SerializePayload() const
{
  JsonValue payload;

  if(m_deliveryStreamNameHasBeenSet)
  {
   payload.WithString("DeliveryStreamName", m_deliveryStreamName);

  }

  if(m_currentDeliveryStreamVersionIdHasBeenSet)
  {
   payload.WithString("CurrentDeliveryStreamVersionId", m_currentDeliveryStreamVersionId);

  }

  if(m_destinationIdHasBeenSet)
  {
   payload.WithString("DestinationId", m_destinationId);

  }

  if(m_extendedS3DestinationUpdateHasBeenSet)
  {
   payload.WithObject("ExtendedS3DestinationUpdate", m_extendedS3DestinationUpdate.Jsonize());

  }

  if(m_redshiftDestinationUpdateHasBeenSet)
  {
   payload.WithObject("RedshiftDestinationUpdate", m_redshiftDestinationUpdate.Jsonize());

  }

  if(m_elasticsearchDestinationUpdateHasBeenSet)
  {
   payload.WithObject("ElasticsearchDestinationUpdate", m_elasticsearchDestinationUpdate.Jsonize());

  }

  if(m_splunkDestinationUpdateHasBeenSet)
  {
   payload.WithObject("SplunkDestinationUpdate", m_splunkDestinationUpdate.Jsonize());

  }

  return payload.View().WriteReadable();
}

Aws::Http::HeaderValueCollection UpdateDestinationRequest::GetRequestSpecificHeaders() const
{
  Aws::Http::HeaderValueCollection headers;
  headers.insert(Aws::Http::HeaderValuePair("X-Amz-Target", "Firehose_20150804.UpdateDestination"));
  return headers;

}




