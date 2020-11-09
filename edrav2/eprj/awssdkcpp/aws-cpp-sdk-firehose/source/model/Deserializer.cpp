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

#include <aws/firehose/model/Deserializer.h>
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

Deserializer::Deserializer() : 
    m_openXJsonSerDeHasBeenSet(false),
    m_hiveJsonSerDeHasBeenSet(false)
{
}

Deserializer::Deserializer(JsonView jsonValue) : 
    m_openXJsonSerDeHasBeenSet(false),
    m_hiveJsonSerDeHasBeenSet(false)
{
  *this = jsonValue;
}

Deserializer& Deserializer::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("OpenXJsonSerDe"))
  {
    m_openXJsonSerDe = jsonValue.GetObject("OpenXJsonSerDe");

    m_openXJsonSerDeHasBeenSet = true;
  }

  if(jsonValue.ValueExists("HiveJsonSerDe"))
  {
    m_hiveJsonSerDe = jsonValue.GetObject("HiveJsonSerDe");

    m_hiveJsonSerDeHasBeenSet = true;
  }

  return *this;
}

JsonValue Deserializer::Jsonize() const
{
  JsonValue payload;

  if(m_openXJsonSerDeHasBeenSet)
  {
   payload.WithObject("OpenXJsonSerDe", m_openXJsonSerDe.Jsonize());

  }

  if(m_hiveJsonSerDeHasBeenSet)
  {
   payload.WithObject("HiveJsonSerDe", m_hiveJsonSerDe.Jsonize());

  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
