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
#include <aws/firehose/model/KinesisStreamSourceDescription.h>
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
   * <p>Details about a Kinesis data stream used as the source for a Kinesis Data
   * Firehose delivery stream.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/SourceDescription">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API SourceDescription
  {
  public:
    SourceDescription();
    SourceDescription(Aws::Utils::Json::JsonView jsonValue);
    SourceDescription& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>The <a>KinesisStreamSourceDescription</a> value for the source Kinesis data
     * stream.</p>
     */
    inline const KinesisStreamSourceDescription& GetKinesisStreamSourceDescription() const{ return m_kinesisStreamSourceDescription; }

    /**
     * <p>The <a>KinesisStreamSourceDescription</a> value for the source Kinesis data
     * stream.</p>
     */
    inline void SetKinesisStreamSourceDescription(const KinesisStreamSourceDescription& value) { m_kinesisStreamSourceDescriptionHasBeenSet = true; m_kinesisStreamSourceDescription = value; }

    /**
     * <p>The <a>KinesisStreamSourceDescription</a> value for the source Kinesis data
     * stream.</p>
     */
    inline void SetKinesisStreamSourceDescription(KinesisStreamSourceDescription&& value) { m_kinesisStreamSourceDescriptionHasBeenSet = true; m_kinesisStreamSourceDescription = std::move(value); }

    /**
     * <p>The <a>KinesisStreamSourceDescription</a> value for the source Kinesis data
     * stream.</p>
     */
    inline SourceDescription& WithKinesisStreamSourceDescription(const KinesisStreamSourceDescription& value) { SetKinesisStreamSourceDescription(value); return *this;}

    /**
     * <p>The <a>KinesisStreamSourceDescription</a> value for the source Kinesis data
     * stream.</p>
     */
    inline SourceDescription& WithKinesisStreamSourceDescription(KinesisStreamSourceDescription&& value) { SetKinesisStreamSourceDescription(std::move(value)); return *this;}

  private:

    KinesisStreamSourceDescription m_kinesisStreamSourceDescription;
    bool m_kinesisStreamSourceDescriptionHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
