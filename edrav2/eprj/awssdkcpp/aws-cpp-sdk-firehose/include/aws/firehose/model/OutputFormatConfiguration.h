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
#include <aws/firehose/model/Serializer.h>
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
   * <p>Specifies the serializer that you want Kinesis Data Firehose to use to
   * convert the format of your data before it writes it to Amazon S3.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/OutputFormatConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API OutputFormatConfiguration
  {
  public:
    OutputFormatConfiguration();
    OutputFormatConfiguration(Aws::Utils::Json::JsonView jsonValue);
    OutputFormatConfiguration& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>Specifies which serializer to use. You can choose either the ORC SerDe or the
     * Parquet SerDe. If both are non-null, the server rejects the request.</p>
     */
    inline const Serializer& GetSerializer() const{ return m_serializer; }

    /**
     * <p>Specifies which serializer to use. You can choose either the ORC SerDe or the
     * Parquet SerDe. If both are non-null, the server rejects the request.</p>
     */
    inline void SetSerializer(const Serializer& value) { m_serializerHasBeenSet = true; m_serializer = value; }

    /**
     * <p>Specifies which serializer to use. You can choose either the ORC SerDe or the
     * Parquet SerDe. If both are non-null, the server rejects the request.</p>
     */
    inline void SetSerializer(Serializer&& value) { m_serializerHasBeenSet = true; m_serializer = std::move(value); }

    /**
     * <p>Specifies which serializer to use. You can choose either the ORC SerDe or the
     * Parquet SerDe. If both are non-null, the server rejects the request.</p>
     */
    inline OutputFormatConfiguration& WithSerializer(const Serializer& value) { SetSerializer(value); return *this;}

    /**
     * <p>Specifies which serializer to use. You can choose either the ORC SerDe or the
     * Parquet SerDe. If both are non-null, the server rejects the request.</p>
     */
    inline OutputFormatConfiguration& WithSerializer(Serializer&& value) { SetSerializer(std::move(value)); return *this;}

  private:

    Serializer m_serializer;
    bool m_serializerHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
