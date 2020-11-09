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

#pragma once
#include <aws/firehose/Firehose_EXPORTS.h>
#include <aws/firehose/model/ParquetCompression.h>
#include <aws/firehose/model/ParquetWriterVersion.h>
#include <utility>

namespace Aws
{
namespace Utils
{
namespace Json
{
  class JsonValue;
  class JsonView;
} // namespace Json
} // namespace Utils
namespace Firehose
{
namespace Model
{

  /**
   * <p>A serializer to use for converting data to the Parquet format before storing
   * it in Amazon S3. For more information, see <a
   * href="https://parquet.apache.org/documentation/latest/">Apache
   * Parquet</a>.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/ParquetSerDe">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API ParquetSerDe
  {
  public:
    ParquetSerDe();
    ParquetSerDe(Aws::Utils::Json::JsonView jsonValue);
    ParquetSerDe& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>The Hadoop Distributed File System (HDFS) block size. This is useful if you
     * intend to copy the data from Amazon S3 to HDFS before querying. The default is
     * 256 MiB and the minimum is 64 MiB. Kinesis Data Firehose uses this value for
     * padding calculations.</p>
     */
    inline int GetBlockSizeBytes() const{ return m_blockSizeBytes; }

    /**
     * <p>The Hadoop Distributed File System (HDFS) block size. This is useful if you
     * intend to copy the data from Amazon S3 to HDFS before querying. The default is
     * 256 MiB and the minimum is 64 MiB. Kinesis Data Firehose uses this value for
     * padding calculations.</p>
     */
    inline void SetBlockSizeBytes(int value) { m_blockSizeBytesHasBeenSet = true; m_blockSizeBytes = value; }

    /**
     * <p>The Hadoop Distributed File System (HDFS) block size. This is useful if you
     * intend to copy the data from Amazon S3 to HDFS before querying. The default is
     * 256 MiB and the minimum is 64 MiB. Kinesis Data Firehose uses this value for
     * padding calculations.</p>
     */
    inline ParquetSerDe& WithBlockSizeBytes(int value) { SetBlockSizeBytes(value); return *this;}


    /**
     * <p>The Parquet page size. Column chunks are divided into pages. A page is
     * conceptually an indivisible unit (in terms of compression and encoding). The
     * minimum value is 64 KiB and the default is 1 MiB.</p>
     */
    inline int GetPageSizeBytes() const{ return m_pageSizeBytes; }

    /**
     * <p>The Parquet page size. Column chunks are divided into pages. A page is
     * conceptually an indivisible unit (in terms of compression and encoding). The
     * minimum value is 64 KiB and the default is 1 MiB.</p>
     */
    inline void SetPageSizeBytes(int value) { m_pageSizeBytesHasBeenSet = true; m_pageSizeBytes = value; }

    /**
     * <p>The Parquet page size. Column chunks are divided into pages. A page is
     * conceptually an indivisible unit (in terms of compression and encoding). The
     * minimum value is 64 KiB and the default is 1 MiB.</p>
     */
    inline ParquetSerDe& WithPageSizeBytes(int value) { SetPageSizeBytes(value); return *this;}


    /**
     * <p>The compression code to use over data blocks. The possible values are
     * <code>UNCOMPRESSED</code>, <code>SNAPPY</code>, and <code>GZIP</code>, with the
     * default being <code>SNAPPY</code>. Use <code>SNAPPY</code> for higher
     * decompression speed. Use <code>GZIP</code> if the compression ration is more
     * important than speed.</p>
     */
    inline const ParquetCompression& GetCompression() const{ return m_compression; }

    /**
     * <p>The compression code to use over data blocks. The possible values are
     * <code>UNCOMPRESSED</code>, <code>SNAPPY</code>, and <code>GZIP</code>, with the
     * default being <code>SNAPPY</code>. Use <code>SNAPPY</code> for higher
     * decompression speed. Use <code>GZIP</code> if the compression ration is more
     * important than speed.</p>
     */
    inline void SetCompression(const ParquetCompression& value) { m_compressionHasBeenSet = true; m_compression = value; }

    /**
     * <p>The compression code to use over data blocks. The possible values are
     * <code>UNCOMPRESSED</code>, <code>SNAPPY</code>, and <code>GZIP</code>, with the
     * default being <code>SNAPPY</code>. Use <code>SNAPPY</code> for higher
     * decompression speed. Use <code>GZIP</code> if the compression ration is more
     * important than speed.</p>
     */
    inline void SetCompression(ParquetCompression&& value) { m_compressionHasBeenSet = true; m_compression = std::move(value); }

    /**
     * <p>The compression code to use over data blocks. The possible values are
     * <code>UNCOMPRESSED</code>, <code>SNAPPY</code>, and <code>GZIP</code>, with the
     * default being <code>SNAPPY</code>. Use <code>SNAPPY</code> for higher
     * decompression speed. Use <code>GZIP</code> if the compression ration is more
     * important than speed.</p>
     */
    inline ParquetSerDe& WithCompression(const ParquetCompression& value) { SetCompression(value); return *this;}

    /**
     * <p>The compression code to use over data blocks. The possible values are
     * <code>UNCOMPRESSED</code>, <code>SNAPPY</code>, and <code>GZIP</code>, with the
     * default being <code>SNAPPY</code>. Use <code>SNAPPY</code> for higher
     * decompression speed. Use <code>GZIP</code> if the compression ration is more
     * important than speed.</p>
     */
    inline ParquetSerDe& WithCompression(ParquetCompression&& value) { SetCompression(std::move(value)); return *this;}


    /**
     * <p>Indicates whether to enable dictionary compression.</p>
     */
    inline bool GetEnableDictionaryCompression() const{ return m_enableDictionaryCompression; }

    /**
     * <p>Indicates whether to enable dictionary compression.</p>
     */
    inline void SetEnableDictionaryCompression(bool value) { m_enableDictionaryCompressionHasBeenSet = true; m_enableDictionaryCompression = value; }

    /**
     * <p>Indicates whether to enable dictionary compression.</p>
     */
    inline ParquetSerDe& WithEnableDictionaryCompression(bool value) { SetEnableDictionaryCompression(value); return *this;}


    /**
     * <p>The maximum amount of padding to apply. This is useful if you intend to copy
     * the data from Amazon S3 to HDFS before querying. The default is 0.</p>
     */
    inline int GetMaxPaddingBytes() const{ return m_maxPaddingBytes; }

    /**
     * <p>The maximum amount of padding to apply. This is useful if you intend to copy
     * the data from Amazon S3 to HDFS before querying. The default is 0.</p>
     */
    inline void SetMaxPaddingBytes(int value) { m_maxPaddingBytesHasBeenSet = true; m_maxPaddingBytes = value; }

    /**
     * <p>The maximum amount of padding to apply. This is useful if you intend to copy
     * the data from Amazon S3 to HDFS before querying. The default is 0.</p>
     */
    inline ParquetSerDe& WithMaxPaddingBytes(int value) { SetMaxPaddingBytes(value); return *this;}


    /**
     * <p>Indicates the version of row format to output. The possible values are
     * <code>V1</code> and <code>V2</code>. The default is <code>V1</code>.</p>
     */
    inline const ParquetWriterVersion& GetWriterVersion() const{ return m_writerVersion; }

    /**
     * <p>Indicates the version of row format to output. The possible values are
     * <code>V1</code> and <code>V2</code>. The default is <code>V1</code>.</p>
     */
    inline void SetWriterVersion(const ParquetWriterVersion& value) { m_writerVersionHasBeenSet = true; m_writerVersion = value; }

    /**
     * <p>Indicates the version of row format to output. The possible values are
     * <code>V1</code> and <code>V2</code>. The default is <code>V1</code>.</p>
     */
    inline void SetWriterVersion(ParquetWriterVersion&& value) { m_writerVersionHasBeenSet = true; m_writerVersion = std::move(value); }

    /**
     * <p>Indicates the version of row format to output. The possible values are
     * <code>V1</code> and <code>V2</code>. The default is <code>V1</code>.</p>
     */
    inline ParquetSerDe& WithWriterVersion(const ParquetWriterVersion& value) { SetWriterVersion(value); return *this;}

    /**
     * <p>Indicates the version of row format to output. The possible values are
     * <code>V1</code> and <code>V2</code>. The default is <code>V1</code>.</p>
     */
    inline ParquetSerDe& WithWriterVersion(ParquetWriterVersion&& value) { SetWriterVersion(std::move(value)); return *this;}

  private:

    int m_blockSizeBytes;
    bool m_blockSizeBytesHasBeenSet;

    int m_pageSizeBytes;
    bool m_pageSizeBytesHasBeenSet;

    ParquetCompression m_compression;
    bool m_compressionHasBeenSet;

    bool m_enableDictionaryCompression;
    bool m_enableDictionaryCompressionHasBeenSet;

    int m_maxPaddingBytes;
    bool m_maxPaddingBytesHasBeenSet;

    ParquetWriterVersion m_writerVersion;
    bool m_writerVersionHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
