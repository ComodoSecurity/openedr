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
#include <aws/firehose/model/ParquetSerDe.h>
#include <aws/firehose/model/OrcSerDe.h>
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
   * <p>The serializer that you want Kinesis Data Firehose to use to convert data to
   * the target format before writing it to Amazon S3. Kinesis Data Firehose supports
   * two types of serializers: the <a
   * href="https://hive.apache.org/javadocs/r1.2.2/api/org/apache/hadoop/hive/ql/io/orc/OrcSerde.html">ORC
   * SerDe</a> and the <a
   * href="https://hive.apache.org/javadocs/r1.2.2/api/org/apache/hadoop/hive/ql/io/parquet/serde/ParquetHiveSerDe.html">Parquet
   * SerDe</a>.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/Serializer">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API Serializer
  {
  public:
    Serializer();
    Serializer(Aws::Utils::Json::JsonView jsonValue);
    Serializer& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>A serializer to use for converting data to the Parquet format before storing
     * it in Amazon S3. For more information, see <a
     * href="https://parquet.apache.org/documentation/latest/">Apache Parquet</a>.</p>
     */
    inline const ParquetSerDe& GetParquetSerDe() const{ return m_parquetSerDe; }

    /**
     * <p>A serializer to use for converting data to the Parquet format before storing
     * it in Amazon S3. For more information, see <a
     * href="https://parquet.apache.org/documentation/latest/">Apache Parquet</a>.</p>
     */
    inline void SetParquetSerDe(const ParquetSerDe& value) { m_parquetSerDeHasBeenSet = true; m_parquetSerDe = value; }

    /**
     * <p>A serializer to use for converting data to the Parquet format before storing
     * it in Amazon S3. For more information, see <a
     * href="https://parquet.apache.org/documentation/latest/">Apache Parquet</a>.</p>
     */
    inline void SetParquetSerDe(ParquetSerDe&& value) { m_parquetSerDeHasBeenSet = true; m_parquetSerDe = std::move(value); }

    /**
     * <p>A serializer to use for converting data to the Parquet format before storing
     * it in Amazon S3. For more information, see <a
     * href="https://parquet.apache.org/documentation/latest/">Apache Parquet</a>.</p>
     */
    inline Serializer& WithParquetSerDe(const ParquetSerDe& value) { SetParquetSerDe(value); return *this;}

    /**
     * <p>A serializer to use for converting data to the Parquet format before storing
     * it in Amazon S3. For more information, see <a
     * href="https://parquet.apache.org/documentation/latest/">Apache Parquet</a>.</p>
     */
    inline Serializer& WithParquetSerDe(ParquetSerDe&& value) { SetParquetSerDe(std::move(value)); return *this;}


    /**
     * <p>A serializer to use for converting data to the ORC format before storing it
     * in Amazon S3. For more information, see <a
     * href="https://orc.apache.org/docs/">Apache ORC</a>.</p>
     */
    inline const OrcSerDe& GetOrcSerDe() const{ return m_orcSerDe; }

    /**
     * <p>A serializer to use for converting data to the ORC format before storing it
     * in Amazon S3. For more information, see <a
     * href="https://orc.apache.org/docs/">Apache ORC</a>.</p>
     */
    inline void SetOrcSerDe(const OrcSerDe& value) { m_orcSerDeHasBeenSet = true; m_orcSerDe = value; }

    /**
     * <p>A serializer to use for converting data to the ORC format before storing it
     * in Amazon S3. For more information, see <a
     * href="https://orc.apache.org/docs/">Apache ORC</a>.</p>
     */
    inline void SetOrcSerDe(OrcSerDe&& value) { m_orcSerDeHasBeenSet = true; m_orcSerDe = std::move(value); }

    /**
     * <p>A serializer to use for converting data to the ORC format before storing it
     * in Amazon S3. For more information, see <a
     * href="https://orc.apache.org/docs/">Apache ORC</a>.</p>
     */
    inline Serializer& WithOrcSerDe(const OrcSerDe& value) { SetOrcSerDe(value); return *this;}

    /**
     * <p>A serializer to use for converting data to the ORC format before storing it
     * in Amazon S3. For more information, see <a
     * href="https://orc.apache.org/docs/">Apache ORC</a>.</p>
     */
    inline Serializer& WithOrcSerDe(OrcSerDe&& value) { SetOrcSerDe(std::move(value)); return *this;}

  private:

    ParquetSerDe m_parquetSerDe;
    bool m_parquetSerDeHasBeenSet;

    OrcSerDe m_orcSerDe;
    bool m_orcSerDeHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
