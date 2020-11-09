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

#include <aws/firehose/model/DataFormatConversionConfiguration.h>
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

DataFormatConversionConfiguration::DataFormatConversionConfiguration() : 
    m_schemaConfigurationHasBeenSet(false),
    m_inputFormatConfigurationHasBeenSet(false),
    m_outputFormatConfigurationHasBeenSet(false),
    m_enabled(false),
    m_enabledHasBeenSet(false)
{
}

DataFormatConversionConfiguration::DataFormatConversionConfiguration(JsonView jsonValue) : 
    m_schemaConfigurationHasBeenSet(false),
    m_inputFormatConfigurationHasBeenSet(false),
    m_outputFormatConfigurationHasBeenSet(false),
    m_enabled(false),
    m_enabledHasBeenSet(false)
{
  *this = jsonValue;
}

DataFormatConversionConfiguration& DataFormatConversionConfiguration::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("SchemaConfiguration"))
  {
    m_schemaConfiguration = jsonValue.GetObject("SchemaConfiguration");

    m_schemaConfigurationHasBeenSet = true;
  }

  if(jsonValue.ValueExists("InputFormatConfiguration"))
  {
    m_inputFormatConfiguration = jsonValue.GetObject("InputFormatConfiguration");

    m_inputFormatConfigurationHasBeenSet = true;
  }

  if(jsonValue.ValueExists("OutputFormatConfiguration"))
  {
    m_outputFormatConfiguration = jsonValue.GetObject("OutputFormatConfiguration");

    m_outputFormatConfigurationHasBeenSet = true;
  }

  if(jsonValue.ValueExists("Enabled"))
  {
    m_enabled = jsonValue.GetBool("Enabled");

    m_enabledHasBeenSet = true;
  }

  return *this;
}

JsonValue DataFormatConversionConfiguration::Jsonize() const
{
  JsonValue payload;

  if(m_schemaConfigurationHasBeenSet)
  {
   payload.WithObject("SchemaConfiguration", m_schemaConfiguration.Jsonize());

  }

  if(m_inputFormatConfigurationHasBeenSet)
  {
   payload.WithObject("InputFormatConfiguration", m_inputFormatConfiguration.Jsonize());

  }

  if(m_outputFormatConfigurationHasBeenSet)
  {
   payload.WithObject("OutputFormatConfiguration", m_outputFormatConfiguration.Jsonize());

  }

  if(m_enabledHasBeenSet)
  {
   payload.WithBool("Enabled", m_enabled);

  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
