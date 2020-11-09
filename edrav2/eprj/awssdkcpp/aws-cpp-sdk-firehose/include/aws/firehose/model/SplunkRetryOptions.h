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
   * <p>Configures retry behavior in case Kinesis Data Firehose is unable to deliver
   * documents to Splunk, or if it doesn't receive an acknowledgment from
   * Splunk.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/SplunkRetryOptions">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API SplunkRetryOptions
  {
  public:
    SplunkRetryOptions();
    SplunkRetryOptions(Aws::Utils::Json::JsonView jsonValue);
    SplunkRetryOptions& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>The total amount of time that Kinesis Data Firehose spends on retries. This
     * duration starts after the initial attempt to send data to Splunk fails. It
     * doesn't include the periods during which Kinesis Data Firehose waits for
     * acknowledgment from Splunk after each attempt.</p>
     */
    inline int GetDurationInSeconds() const{ return m_durationInSeconds; }

    /**
     * <p>The total amount of time that Kinesis Data Firehose spends on retries. This
     * duration starts after the initial attempt to send data to Splunk fails. It
     * doesn't include the periods during which Kinesis Data Firehose waits for
     * acknowledgment from Splunk after each attempt.</p>
     */
    inline void SetDurationInSeconds(int value) { m_durationInSecondsHasBeenSet = true; m_durationInSeconds = value; }

    /**
     * <p>The total amount of time that Kinesis Data Firehose spends on retries. This
     * duration starts after the initial attempt to send data to Splunk fails. It
     * doesn't include the periods during which Kinesis Data Firehose waits for
     * acknowledgment from Splunk after each attempt.</p>
     */
    inline SplunkRetryOptions& WithDurationInSeconds(int value) { SetDurationInSeconds(value); return *this;}

  private:

    int m_durationInSeconds;
    bool m_durationInSecondsHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
