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

#include <aws/firehose/model/CloudWatchLoggingOptions.h>
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

CloudWatchLoggingOptions::CloudWatchLoggingOptions() : 
    m_enabled(false),
    m_enabledHasBeenSet(false),
    m_logGroupNameHasBeenSet(false),
    m_logStreamNameHasBeenSet(false)
{
}

CloudWatchLoggingOptions::CloudWatchLoggingOptions(JsonView jsonValue) : 
    m_enabled(false),
    m_enabledHasBeenSet(false),
    m_logGroupNameHasBeenSet(false),
    m_logStreamNameHasBeenSet(false)
{
  *this = jsonValue;
}

CloudWatchLoggingOptions& CloudWatchLoggingOptions::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("Enabled"))
  {
    m_enabled = jsonValue.GetBool("Enabled");

    m_enabledHasBeenSet = true;
  }

  if(jsonValue.ValueExists("LogGroupName"))
  {
    m_logGroupName = jsonValue.GetString("LogGroupName");

    m_logGroupNameHasBeenSet = true;
  }

  if(jsonValue.ValueExists("LogStreamName"))
  {
    m_logStreamName = jsonValue.GetString("LogStreamName");

    m_logStreamNameHasBeenSet = true;
  }

  return *this;
}

JsonValue CloudWatchLoggingOptions::Jsonize() const
{
  JsonValue payload;

  if(m_enabledHasBeenSet)
  {
   payload.WithBool("Enabled", m_enabled);

  }

  if(m_logGroupNameHasBeenSet)
  {
   payload.WithString("LogGroupName", m_logGroupName);

  }

  if(m_logStreamNameHasBeenSet)
  {
   payload.WithString("LogStreamName", m_logStreamName);

  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
