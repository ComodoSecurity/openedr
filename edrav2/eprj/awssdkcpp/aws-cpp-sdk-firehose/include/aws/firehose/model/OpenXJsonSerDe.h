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
#include <aws/core/utils/memory/stl/AWSMap.h>
#include <aws/core/utils/memory/stl/AWSString.h>
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
   * <p>The OpenX SerDe. Used by Kinesis Data Firehose for deserializing data, which
   * means converting it from the JSON format in preparation for serializing it to
   * the Parquet or ORC format. This is one of two deserializers you can choose,
   * depending on which one offers the functionality you need. The other option is
   * the native Hive / HCatalog JsonSerDe.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/OpenXJsonSerDe">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API OpenXJsonSerDe
  {
  public:
    OpenXJsonSerDe();
    OpenXJsonSerDe(Aws::Utils::Json::JsonView jsonValue);
    OpenXJsonSerDe& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>When set to <code>true</code>, specifies that the names of the keys include
     * dots and that you want Kinesis Data Firehose to replace them with underscores.
     * This is useful because Apache Hive does not allow dots in column names. For
     * example, if the JSON contains a key whose name is "a.b", you can define the
     * column name to be "a_b" when using this option.</p> <p>The default is
     * <code>false</code>.</p>
     */
    inline bool GetConvertDotsInJsonKeysToUnderscores() const{ return m_convertDotsInJsonKeysToUnderscores; }

    /**
     * <p>When set to <code>true</code>, specifies that the names of the keys include
     * dots and that you want Kinesis Data Firehose to replace them with underscores.
     * This is useful because Apache Hive does not allow dots in column names. For
     * example, if the JSON contains a key whose name is "a.b", you can define the
     * column name to be "a_b" when using this option.</p> <p>The default is
     * <code>false</code>.</p>
     */
    inline void SetConvertDotsInJsonKeysToUnderscores(bool value) { m_convertDotsInJsonKeysToUnderscoresHasBeenSet = true; m_convertDotsInJsonKeysToUnderscores = value; }

    /**
     * <p>When set to <code>true</code>, specifies that the names of the keys include
     * dots and that you want Kinesis Data Firehose to replace them with underscores.
     * This is useful because Apache Hive does not allow dots in column names. For
     * example, if the JSON contains a key whose name is "a.b", you can define the
     * column name to be "a_b" when using this option.</p> <p>The default is
     * <code>false</code>.</p>
     */
    inline OpenXJsonSerDe& WithConvertDotsInJsonKeysToUnderscores(bool value) { SetConvertDotsInJsonKeysToUnderscores(value); return *this;}


    /**
     * <p>When set to <code>true</code>, which is the default, Kinesis Data Firehose
     * converts JSON keys to lowercase before deserializing them.</p>
     */
    inline bool GetCaseInsensitive() const{ return m_caseInsensitive; }

    /**
     * <p>When set to <code>true</code>, which is the default, Kinesis Data Firehose
     * converts JSON keys to lowercase before deserializing them.</p>
     */
    inline void SetCaseInsensitive(bool value) { m_caseInsensitiveHasBeenSet = true; m_caseInsensitive = value; }

    /**
     * <p>When set to <code>true</code>, which is the default, Kinesis Data Firehose
     * converts JSON keys to lowercase before deserializing them.</p>
     */
    inline OpenXJsonSerDe& WithCaseInsensitive(bool value) { SetCaseInsensitive(value); return *this;}


    /**
     * <p>Maps column names to JSON keys that aren't identical to the column names.
     * This is useful when the JSON contains keys that are Hive keywords. For example,
     * <code>timestamp</code> is a Hive keyword. If you have a JSON key named
     * <code>timestamp</code>, set this parameter to <code>{"ts": "timestamp"}</code>
     * to map this key to a column named <code>ts</code>.</p>
     */
    inline const Aws::Map<Aws::String, Aws::String>& GetColumnToJsonKeyMappings() const{ return m_columnToJsonKeyMappings; }

    /**
     * <p>Maps column names to JSON keys that aren't identical to the column names.
     * This is useful when the JSON contains keys that are Hive keywords. For example,
     * <code>timestamp</code> is a Hive keyword. If you have a JSON key named
     * <code>timestamp</code>, set this parameter to <code>{"ts": "timestamp"}</code>
     * to map this key to a column named <code>ts</code>.</p>
     */
    inline void SetColumnToJsonKeyMappings(const Aws::Map<Aws::String, Aws::String>& value) { m_columnToJsonKeyMappingsHasBeenSet = true; m_columnToJsonKeyMappings = value; }

    /**
     * <p>Maps column names to JSON keys that aren't identical to the column names.
     * This is useful when the JSON contains keys that are Hive keywords. For example,
     * <code>timestamp</code> is a Hive keyword. If you have a JSON key named
     * <code>timestamp</code>, set this parameter to <code>{"ts": "timestamp"}</code>
     * to map this key to a column named <code>ts</code>.</p>
     */
    inline void SetColumnToJsonKeyMappings(Aws::Map<Aws::String, Aws::String>&& value) { m_columnToJsonKeyMappingsHasBeenSet = true; m_columnToJsonKeyMappings = std::move(value); }

    /**
     * <p>Maps column names to JSON keys that aren't identical to the column names.
     * This is useful when the JSON contains keys that are Hive keywords. For example,
     * <code>timestamp</code> is a Hive keyword. If you have a JSON key named
     * <code>timestamp</code>, set this parameter to <code>{"ts": "timestamp"}</code>
     * to map this key to a column named <code>ts</code>.</p>
     */
    inline OpenXJsonSerDe& WithColumnToJsonKeyMappings(const Aws::Map<Aws::String, Aws::String>& value) { SetColumnToJsonKeyMappings(value); return *this;}

    /**
     * <p>Maps column names to JSON keys that aren't identical to the column names.
     * This is useful when the JSON contains keys that are Hive keywords. For example,
     * <code>timestamp</code> is a Hive keyword. If you have a JSON key named
     * <code>timestamp</code>, set this parameter to <code>{"ts": "timestamp"}</code>
     * to map this key to a column named <code>ts</code>.</p>
     */
    inline OpenXJsonSerDe& WithColumnToJsonKeyMappings(Aws::Map<Aws::String, Aws::String>&& value) { SetColumnToJsonKeyMappings(std::move(value)); return *this;}

    /**
     * <p>Maps column names to JSON keys that aren't identical to the column names.
     * This is useful when the JSON contains keys that are Hive keywords. For example,
     * <code>timestamp</code> is a Hive keyword. If you have a JSON key named
     * <code>timestamp</code>, set this parameter to <code>{"ts": "timestamp"}</code>
     * to map this key to a column named <code>ts</code>.</p>
     */
    inline OpenXJsonSerDe& AddColumnToJsonKeyMappings(const Aws::String& key, const Aws::String& value) { m_columnToJsonKeyMappingsHasBeenSet = true; m_columnToJsonKeyMappings.emplace(key, value); return *this; }

    /**
     * <p>Maps column names to JSON keys that aren't identical to the column names.
     * This is useful when the JSON contains keys that are Hive keywords. For example,
     * <code>timestamp</code> is a Hive keyword. If you have a JSON key named
     * <code>timestamp</code>, set this parameter to <code>{"ts": "timestamp"}</code>
     * to map this key to a column named <code>ts</code>.</p>
     */
    inline OpenXJsonSerDe& AddColumnToJsonKeyMappings(Aws::String&& key, const Aws::String& value) { m_columnToJsonKeyMappingsHasBeenSet = true; m_columnToJsonKeyMappings.emplace(std::move(key), value); return *this; }

    /**
     * <p>Maps column names to JSON keys that aren't identical to the column names.
     * This is useful when the JSON contains keys that are Hive keywords. For example,
     * <code>timestamp</code> is a Hive keyword. If you have a JSON key named
     * <code>timestamp</code>, set this parameter to <code>{"ts": "timestamp"}</code>
     * to map this key to a column named <code>ts</code>.</p>
     */
    inline OpenXJsonSerDe& AddColumnToJsonKeyMappings(const Aws::String& key, Aws::String&& value) { m_columnToJsonKeyMappingsHasBeenSet = true; m_columnToJsonKeyMappings.emplace(key, std::move(value)); return *this; }

    /**
     * <p>Maps column names to JSON keys that aren't identical to the column names.
     * This is useful when the JSON contains keys that are Hive keywords. For example,
     * <code>timestamp</code> is a Hive keyword. If you have a JSON key named
     * <code>timestamp</code>, set this parameter to <code>{"ts": "timestamp"}</code>
     * to map this key to a column named <code>ts</code>.</p>
     */
    inline OpenXJsonSerDe& AddColumnToJsonKeyMappings(Aws::String&& key, Aws::String&& value) { m_columnToJsonKeyMappingsHasBeenSet = true; m_columnToJsonKeyMappings.emplace(std::move(key), std::move(value)); return *this; }

    /**
     * <p>Maps column names to JSON keys that aren't identical to the column names.
     * This is useful when the JSON contains keys that are Hive keywords. For example,
     * <code>timestamp</code> is a Hive keyword. If you have a JSON key named
     * <code>timestamp</code>, set this parameter to <code>{"ts": "timestamp"}</code>
     * to map this key to a column named <code>ts</code>.</p>
     */
    inline OpenXJsonSerDe& AddColumnToJsonKeyMappings(const char* key, Aws::String&& value) { m_columnToJsonKeyMappingsHasBeenSet = true; m_columnToJsonKeyMappings.emplace(key, std::move(value)); return *this; }

    /**
     * <p>Maps column names to JSON keys that aren't identical to the column names.
     * This is useful when the JSON contains keys that are Hive keywords. For example,
     * <code>timestamp</code> is a Hive keyword. If you have a JSON key named
     * <code>timestamp</code>, set this parameter to <code>{"ts": "timestamp"}</code>
     * to map this key to a column named <code>ts</code>.</p>
     */
    inline OpenXJsonSerDe& AddColumnToJsonKeyMappings(Aws::String&& key, const char* value) { m_columnToJsonKeyMappingsHasBeenSet = true; m_columnToJsonKeyMappings.emplace(std::move(key), value); return *this; }

    /**
     * <p>Maps column names to JSON keys that aren't identical to the column names.
     * This is useful when the JSON contains keys that are Hive keywords. For example,
     * <code>timestamp</code> is a Hive keyword. If you have a JSON key named
     * <code>timestamp</code>, set this parameter to <code>{"ts": "timestamp"}</code>
     * to map this key to a column named <code>ts</code>.</p>
     */
    inline OpenXJsonSerDe& AddColumnToJsonKeyMappings(const char* key, const char* value) { m_columnToJsonKeyMappingsHasBeenSet = true; m_columnToJsonKeyMappings.emplace(key, value); return *this; }

  private:

    bool m_convertDotsInJsonKeysToUnderscores;
    bool m_convertDotsInJsonKeysToUnderscoresHasBeenSet;

    bool m_caseInsensitive;
    bool m_caseInsensitiveHasBeenSet;

    Aws::Map<Aws::String, Aws::String> m_columnToJsonKeyMappings;
    bool m_columnToJsonKeyMappingsHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
