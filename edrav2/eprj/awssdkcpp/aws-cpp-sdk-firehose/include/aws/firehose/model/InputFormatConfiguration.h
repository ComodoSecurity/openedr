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
#include <aws/firehose/model/Deserializer.h>
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
   * <p>Specifies the deserializer you want to use to convert the format of the input
   * data.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/InputFormatConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API InputFormatConfiguration
  {
  public:
    InputFormatConfiguration();
    InputFormatConfiguration(Aws::Utils::Json::JsonView jsonValue);
    InputFormatConfiguration& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>Specifies which deserializer to use. You can choose either the Apache Hive
     * JSON SerDe or the OpenX JSON SerDe. If both are non-null, the server rejects the
     * request.</p>
     */
    inline const Deserializer& GetDeserializer() const{ return m_deserializer; }

    /**
     * <p>Specifies which deserializer to use. You can choose either the Apache Hive
     * JSON SerDe or the OpenX JSON SerDe. If both are non-null, the server rejects the
     * request.</p>
     */
    inline void SetDeserializer(const Deserializer& value) { m_deserializerHasBeenSet = true; m_deserializer = value; }

    /**
     * <p>Specifies which deserializer to use. You can choose either the Apache Hive
     * JSON SerDe or the OpenX JSON SerDe. If both are non-null, the server rejects the
     * request.</p>
     */
    inline void SetDeserializer(Deserializer&& value) { m_deserializerHasBeenSet = true; m_deserializer = std::move(value); }

    /**
     * <p>Specifies which deserializer to use. You can choose either the Apache Hive
     * JSON SerDe or the OpenX JSON SerDe. If both are non-null, the server rejects the
     * request.</p>
     */
    inline InputFormatConfiguration& WithDeserializer(const Deserializer& value) { SetDeserializer(value); return *this;}

    /**
     * <p>Specifies which deserializer to use. You can choose either the Apache Hive
     * JSON SerDe or the OpenX JSON SerDe. If both are non-null, the server rejects the
     * request.</p>
     */
    inline InputFormatConfiguration& WithDeserializer(Deserializer&& value) { SetDeserializer(std::move(value)); return *this;}

  private:

    Deserializer m_deserializer;
    bool m_deserializerHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
