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

#include <aws/firehose/model/KMSEncryptionConfig.h>
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

KMSEncryptionConfig::KMSEncryptionConfig() : 
    m_aWSKMSKeyARNHasBeenSet(false)
{
}

KMSEncryptionConfig::KMSEncryptionConfig(JsonView jsonValue) : 
    m_aWSKMSKeyARNHasBeenSet(false)
{
  *this = jsonValue;
}

KMSEncryptionConfig& KMSEncryptionConfig::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("AWSKMSKeyARN"))
  {
    m_aWSKMSKeyARN = jsonValue.GetString("AWSKMSKeyARN");

    m_aWSKMSKeyARNHasBeenSet = true;
  }

  return *this;
}

JsonValue KMSEncryptionConfig::Jsonize() const
{
  JsonValue payload;

  if(m_aWSKMSKeyARNHasBeenSet)
  {
   payload.WithString("AWSKMSKeyARN", m_aWSKMSKeyARN);

  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
