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
   * <p>Describes hints for the buffering to perform before delivering data to the
   * destination. These options are treated as hints, and therefore Kinesis Data
   * Firehose might choose to use different values when it is optimal.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/BufferingHints">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API BufferingHints
  {
  public:
    BufferingHints();
    BufferingHints(Aws::Utils::Json::JsonView jsonValue);
    BufferingHints& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>Buffer incoming data to the specified size, in MBs, before delivering it to
     * the destination. The default value is 5.</p> <p>We recommend setting this
     * parameter to a value greater than the amount of data you typically ingest into
     * the delivery stream in 10 seconds. For example, if you typically ingest data at
     * 1 MB/sec, the value should be 10 MB or higher.</p>
     */
    inline int GetSizeInMBs() const{ return m_sizeInMBs; }

    /**
     * <p>Buffer incoming data to the specified size, in MBs, before delivering it to
     * the destination. The default value is 5.</p> <p>We recommend setting this
     * parameter to a value greater than the amount of data you typically ingest into
     * the delivery stream in 10 seconds. For example, if you typically ingest data at
     * 1 MB/sec, the value should be 10 MB or higher.</p>
     */
    inline void SetSizeInMBs(int value) { m_sizeInMBsHasBeenSet = true; m_sizeInMBs = value; }

    /**
     * <p>Buffer incoming data to the specified size, in MBs, before delivering it to
     * the destination. The default value is 5.</p> <p>We recommend setting this
     * parameter to a value greater than the amount of data you typically ingest into
     * the delivery stream in 10 seconds. For example, if you typically ingest data at
     * 1 MB/sec, the value should be 10 MB or higher.</p>
     */
    inline BufferingHints& WithSizeInMBs(int value) { SetSizeInMBs(value); return *this;}


    /**
     * <p>Buffer incoming data for the specified period of time, in seconds, before
     * delivering it to the destination. The default value is 300.</p>
     */
    inline int GetIntervalInSeconds() const{ return m_intervalInSeconds; }

    /**
     * <p>Buffer incoming data for the specified period of time, in seconds, before
     * delivering it to the destination. The default value is 300.</p>
     */
    inline void SetIntervalInSeconds(int value) { m_intervalInSecondsHasBeenSet = true; m_intervalInSeconds = value; }

    /**
     * <p>Buffer incoming data for the specified period of time, in seconds, before
     * delivering it to the destination. The default value is 300.</p>
     */
    inline BufferingHints& WithIntervalInSeconds(int value) { SetIntervalInSeconds(value); return *this;}

  private:

    int m_sizeInMBs;
    bool m_sizeInMBsHasBeenSet;

    int m_intervalInSeconds;
    bool m_intervalInSecondsHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
