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

#include <aws/firehose/model/CopyCommand.h>
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

CopyCommand::CopyCommand() : 
    m_dataTableNameHasBeenSet(false),
    m_dataTableColumnsHasBeenSet(false),
    m_copyOptionsHasBeenSet(false)
{
}

CopyCommand::CopyCommand(JsonView jsonValue) : 
    m_dataTableNameHasBeenSet(false),
    m_dataTableColumnsHasBeenSet(false),
    m_copyOptionsHasBeenSet(false)
{
  *this = jsonValue;
}

CopyCommand& CopyCommand::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("DataTableName"))
  {
    m_dataTableName = jsonValue.GetString("DataTableName");

    m_dataTableNameHasBeenSet = true;
  }

  if(jsonValue.ValueExists("DataTableColumns"))
  {
    m_dataTableColumns = jsonValue.GetString("DataTableColumns");

    m_dataTableColumnsHasBeenSet = true;
  }

  if(jsonValue.ValueExists("CopyOptions"))
  {
    m_copyOptions = jsonValue.GetString("CopyOptions");

    m_copyOptionsHasBeenSet = true;
  }

  return *this;
}

JsonValue CopyCommand::Jsonize() const
{
  JsonValue payload;

  if(m_dataTableNameHasBeenSet)
  {
   payload.WithString("DataTableName", m_dataTableName);

  }

  if(m_dataTableColumnsHasBeenSet)
  {
   payload.WithString("DataTableColumns", m_dataTableColumns);

  }

  if(m_copyOptionsHasBeenSet)
  {
   payload.WithString("CopyOptions", m_copyOptions);

  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
