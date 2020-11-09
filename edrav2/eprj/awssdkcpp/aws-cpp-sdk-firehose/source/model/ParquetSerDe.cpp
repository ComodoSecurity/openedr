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

#include <aws/firehose/model/ParquetSerDe.h>
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

ParquetSerDe::ParquetSerDe() : 
    m_blockSizeBytes(0),
    m_blockSizeBytesHasBeenSet(false),
    m_pageSizeBytes(0),
    m_pageSizeBytesHasBeenSet(false),
    m_compression(ParquetCompression::NOT_SET),
    m_compressionHasBeenSet(false),
    m_enableDictionaryCompression(false),
    m_enableDictionaryCompressionHasBeenSet(false),
    m_maxPaddingBytes(0),
    m_maxPaddingBytesHasBeenSet(false),
    m_writerVersion(ParquetWriterVersion::NOT_SET),
    m_writerVersionHasBeenSet(false)
{
}

ParquetSerDe::ParquetSerDe(JsonView jsonValue) : 
    m_blockSizeBytes(0),
    m_blockSizeBytesHasBeenSet(false),
    m_pageSizeBytes(0),
    m_pageSizeBytesHasBeenSet(false),
    m_compression(ParquetCompression::NOT_SET),
    m_compressionHasBeenSet(false),
    m_enableDictionaryCompression(false),
    m_enableDictionaryCompressionHasBeenSet(false),
    m_maxPaddingBytes(0),
    m_maxPaddingBytesHasBeenSet(false),
    m_writerVersion(ParquetWriterVersion::NOT_SET),
    m_writerVersionHasBeenSet(false)
{
  *this = jsonValue;
}

ParquetSerDe& ParquetSerDe::operator =(JsonView jsonValue)
{
  if(jsonValue.ValueExists("BlockSizeBytes"))
  {
    m_blockSizeBytes = jsonValue.GetInteger("BlockSizeBytes");

    m_blockSizeBytesHasBeenSet = true;
  }

  if(jsonValue.ValueExists("PageSizeBytes"))
  {
    m_pageSizeBytes = jsonValue.GetInteger("PageSizeBytes");

    m_pageSizeBytesHasBeenSet = true;
  }

  if(jsonValue.ValueExists("Compression"))
  {
    m_compression = ParquetCompressionMapper::GetParquetCompressionForName(jsonValue.GetString("Compression"));

    m_compressionHasBeenSet = true;
  }

  if(jsonValue.ValueExists("EnableDictionaryCompression"))
  {
    m_enableDictionaryCompression = jsonValue.GetBool("EnableDictionaryCompression");

    m_enableDictionaryCompressionHasBeenSet = true;
  }

  if(jsonValue.ValueExists("MaxPaddingBytes"))
  {
    m_maxPaddingBytes = jsonValue.GetInteger("MaxPaddingBytes");

    m_maxPaddingBytesHasBeenSet = true;
  }

  if(jsonValue.ValueExists("WriterVersion"))
  {
    m_writerVersion = ParquetWriterVersionMapper::GetParquetWriterVersionForName(jsonValue.GetString("WriterVersion"));

    m_writerVersionHasBeenSet = true;
  }

  return *this;
}

JsonValue ParquetSerDe::Jsonize() const
{
  JsonValue payload;

  if(m_blockSizeBytesHasBeenSet)
  {
   payload.WithInteger("BlockSizeBytes", m_blockSizeBytes);

  }

  if(m_pageSizeBytesHasBeenSet)
  {
   payload.WithInteger("PageSizeBytes", m_pageSizeBytes);

  }

  if(m_compressionHasBeenSet)
  {
   payload.WithString("Compression", ParquetCompressionMapper::GetNameForParquetCompression(m_compression));
  }

  if(m_enableDictionaryCompressionHasBeenSet)
  {
   payload.WithBool("EnableDictionaryCompression", m_enableDictionaryCompression);

  }

  if(m_maxPaddingBytesHasBeenSet)
  {
   payload.WithInteger("MaxPaddingBytes", m_maxPaddingBytes);

  }

  if(m_writerVersionHasBeenSet)
  {
   payload.WithString("WriterVersion", ParquetWriterVersionMapper::GetNameForParquetWriterVersion(m_writerVersion));
  }

  return payload;
}

} // namespace Model
} // namespace Firehose
} // namespace Aws
