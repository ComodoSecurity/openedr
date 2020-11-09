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

#include <aws/firehose/model/KinesisStreamSourceConfiguration.h>
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

KinesisStreamSourceConfiguration::KinesisStreamSourceConfiguration() : 
    m_kinesisStreamARNHasBeenSet(false),
    m_roleARNHasBeenSet(false)
{
}

KinesisStreamSourceConfiguration::KinesisStreamSourceConfiguration(JsonView jsonValue) : 
    m_kinesisStreamARNHasBeenSet(false),
    m_roleARNHasBeenSet(false)
{
  *this = jsonValue;
}

KinesisStreamSourceConfiguration& KinesisStreamSourceConfiguration::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("KinesisStreamARN"))
  {
    m_kinesisStreamARN = jsonValue.GetString("KinesisStreamARN");

    m_kinesisStreamARNHasBeenSet = true;
  }

  if(jsonValue.ValueExists("RoleARN"))
  {
    m_roleARN = jsonValue.GetString("RoleARN");

    m_roleARNHasBeenSet = true;
  }

  return *this;
}

JsonValue KinesisStreamSourceConfiguration::Jsonize() const
{
  JsonValue payload;

  if(m_kinesisStreamARNHasBeenSet)
  {
   payload.WithString("KinesisStreamARN", m_kinesisStreamARN);

  }

  if(m_roleARNHasBeenSet)
  {
   payload.WithString("RoleARN", m_roleARN);

  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
