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

#include <aws/firehose/model/ProcessorParameter.h>
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

ProcessorParameter::ProcessorParameter() : 
    m_parameterName(ProcessorParameterName::NOT_SET),
    m_parameterNameHasBeenSet(false),
    m_parameterValueHasBeenSet(false)
{
}

ProcessorParameter::ProcessorParameter(JsonView jsonValue) : 
    m_parameterName(ProcessorParameterName::NOT_SET),
    m_parameterNameHasBeenSet(false),
    m_parameterValueHasBeenSet(false)
{
  *this = jsonValue;
}

ProcessorParameter& ProcessorParameter::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("ParameterName"))
  {
    m_parameterName = ProcessorParameterNameMapper::GetProcessorParameterNameForName(jsonValue.GetString("ParameterName"));

    m_parameterNameHasBeenSet = true;
  }

  if(jsonValue.ValueExists("ParameterValue"))
  {
    m_parameterValue = jsonValue.GetString("ParameterValue");

    m_parameterValueHasBeenSet = true;
  }

  return *this;
}

JsonValue ProcessorParameter::Jsonize() const
{
  JsonValue payload;

  if(m_parameterNameHasBeenSet)
  {
   payload.WithString("ParameterName", ProcessorParameterNameMapper::GetNameForProcessorParameterName(m_parameterName));
  }

  if(m_parameterValueHasBeenSet)
  {
   payload.WithString("ParameterValue", m_parameterValue);

  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
