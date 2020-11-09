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

#include <aws/firehose/model/ListDeliveryStreamsResult.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/UnreferencedParam.h>

#include <utility>

using namespace Aws::Firehose::Model;
using namespace Aws::Utils::Json;
using namespace Aws::Utils;
using namespace Aws;

ListDeliveryStreamsResult::ListDeliveryStreamsResult() : 
    m_hasMoreDeliveryStreams(false)
{
}

ListDeliveryStreamsResult::ListDeliveryStreamsResult(const Aws::AmazonWebServiceResult<JsonValue>& result) : 
    m_hasMoreDeliveryStreams(false)
{
  *this = result;
}

ListDeliveryStreamsResult& ListDeliveryStreamsResult::operator =(const Aws::AmazonWebServiceResult<JsonValue>& result)
{
  JsonView jsonValue = result.GetPayload().View();
  if(jsonValue.ValueExists("DeliveryStreamNames"))
  {
    Array<JsonView> deliveryStreamNamesJsonList = jsonValue.GetArray("DeliveryStreamNames");
    for(unsigned deliveryStreamNamesIndex = 0; deliveryStreamNamesIndex < deliveryStreamNamesJsonList.GetLength(); ++deliveryStreamNamesIndex)
    {
      m_deliveryStreamNames.push_back(deliveryStreamNamesJsonList[deliveryStreamNamesIndex].AsString());
    }
  }

  if(jsonValue.ValueExists("HasMoreDeliveryStreams"))
  {
    m_hasMoreDeliveryStreams = jsonValue.GetBool("HasMoreDeliveryStreams");

  }



  return *this;
}
