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

#include <aws/firehose/model/HiveJsonSerDe.h>
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

HiveJsonSerDe::HiveJsonSerDe() : 
    m_timestampFormatsHasBeenSet(false)
{
}

HiveJsonSerDe::HiveJsonSerDe(JsonView jsonValue) : 
    m_timestampFormatsHasBeenSet(false)
{
  *this = jsonValue;
}

HiveJsonSerDe& HiveJsonSerDe::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("TimestampFormats"))
  {
    Array<JsonView> timestampFormatsJsonList = jsonValue.GetArray("TimestampFormats");
    for(unsigned timestampFormatsIndex = 0; timestampFormatsIndex < timestampFormatsJsonList.GetLength(); ++timestampFormatsIndex)
    {
      m_timestampFormats.push_back(timestampFormatsJsonList[timestampFormatsIndex].AsString());
    }
    m_timestampFormatsHasBeenSet = true;
  }

  return *this;
}

JsonValue HiveJsonSerDe::Jsonize() const
{
  JsonValue payload;

  if(m_timestampFormatsHasBeenSet)
  {
   Array<JsonValue> timestampFormatsJsonList(m_timestampFormats.size());
   for(unsigned timestampFormatsIndex = 0; timestampFormatsIndex < timestampFormatsJsonList.GetLength(); ++timestampFormatsIndex)
   {
     timestampFormatsJsonList[timestampFormatsIndex].AsString(m_timestampFormats[timestampFormatsIndex]);
   }
   payload.WithArray("TimestampFormats", std::move(timestampFormatsJsonList));

  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
