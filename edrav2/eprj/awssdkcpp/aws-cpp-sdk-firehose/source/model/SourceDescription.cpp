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

#include <aws/firehose/model/SourceDescription.h>
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

SourceDescription::SourceDescription() : 
    m_kinesisStreamSourceDescriptionHasBeenSet(false)
{
}

SourceDescription::SourceDescription(JsonView jsonValue) : 
    m_kinesisStreamSourceDescriptionHasBeenSet(false)
{
  *this = jsonValue;
}

SourceDescription& SourceDescription::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("KinesisStreamSourceDescription"))
  {
    m_kinesisStreamSourceDescription = jsonValue.GetObject("KinesisStreamSourceDescription");

    m_kinesisStreamSourceDescriptionHasBeenSet = true;
  }

  return *this;
}

JsonValue SourceDescription::Jsonize() const
{
  JsonValue payload;

  if(m_kinesisStreamSourceDescriptionHasBeenSet)
  {
   payload.WithObject("KinesisStreamSourceDescription", m_kinesisStreamSourceDescription.Jsonize());

  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
