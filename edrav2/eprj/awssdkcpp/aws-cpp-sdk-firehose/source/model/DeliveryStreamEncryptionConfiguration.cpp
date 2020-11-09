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

#include <aws/firehose/model/DeliveryStreamEncryptionConfiguration.h>
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

DeliveryStreamEncryptionConfiguration::DeliveryStreamEncryptionConfiguration() : 
    m_status(DeliveryStreamEncryptionStatus::NOT_SET),
    m_statusHasBeenSet(false)
{
}

DeliveryStreamEncryptionConfiguration::DeliveryStreamEncryptionConfiguration(JsonView jsonValue) : 
    m_status(DeliveryStreamEncryptionStatus::NOT_SET),
    m_statusHasBeenSet(false)
{
  *this = jsonValue;
}

DeliveryStreamEncryptionConfiguration& DeliveryStreamEncryptionConfiguration::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("Status"))
  {
    m_status = DeliveryStreamEncryptionStatusMapper::GetDeliveryStreamEncryptionStatusForName(jsonValue.GetString("Status"));

    m_statusHasBeenSet = true;
  }

  return *this;
}

JsonValue DeliveryStreamEncryptionConfiguration::Jsonize() const
{
  JsonValue payload;

  if(m_statusHasBeenSet)
  {
   payload.WithString("Status", DeliveryStreamEncryptionStatusMapper::GetNameForDeliveryStreamEncryptionStatus(m_status));
  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
