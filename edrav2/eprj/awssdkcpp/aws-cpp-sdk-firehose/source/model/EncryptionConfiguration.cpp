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

#include <aws/firehose/model/EncryptionConfiguration.h>
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

EncryptionConfiguration::EncryptionConfiguration() : 
    m_noEncryptionConfig(NoEncryptionConfig::NOT_SET),
    m_noEncryptionConfigHasBeenSet(false),
    m_kMSEncryptionConfigHasBeenSet(false)
{
}

EncryptionConfiguration::EncryptionConfiguration(JsonView jsonValue) : 
    m_noEncryptionConfig(NoEncryptionConfig::NOT_SET),
    m_noEncryptionConfigHasBeenSet(false),
    m_kMSEncryptionConfigHasBeenSet(false)
{
  *this = jsonValue;
}

EncryptionConfiguration& EncryptionConfiguration::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("NoEncryptionConfig"))
  {
    m_noEncryptionConfig = NoEncryptionConfigMapper::GetNoEncryptionConfigForName(jsonValue.GetString("NoEncryptionConfig"));

    m_noEncryptionConfigHasBeenSet = true;
  }

  if(jsonValue.ValueExists("KMSEncryptionConfig"))
  {
    m_kMSEncryptionConfig = jsonValue.GetObject("KMSEncryptionConfig");

    m_kMSEncryptionConfigHasBeenSet = true;
  }

  return *this;
}

JsonValue EncryptionConfiguration::Jsonize() const
{
  JsonValue payload;

  if(m_noEncryptionConfigHasBeenSet)
  {
   payload.WithString("NoEncryptionConfig", NoEncryptionConfigMapper::GetNameForNoEncryptionConfig(m_noEncryptionConfig));
  }

  if(m_kMSEncryptionConfigHasBeenSet)
  {
   payload.WithObject("KMSEncryptionConfig", m_kMSEncryptionConfig.Jsonize());

  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
