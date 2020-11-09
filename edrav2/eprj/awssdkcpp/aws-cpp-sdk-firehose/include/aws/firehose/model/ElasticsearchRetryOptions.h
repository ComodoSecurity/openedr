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
   * documents to Amazon ES.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/ElasticsearchRetryOptions">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API ElasticsearchRetryOptions
  {
  public:
    ElasticsearchRetryOptions();
    ElasticsearchRetryOptions(Aws::Utils::Json::JsonView jsonValue);
    ElasticsearchRetryOptions& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>After an initial failure to deliver to Amazon ES, the total amount of time
     * during which Kinesis Data Firehose retries delivery (including the first
     * attempt). After this time has elapsed, the failed documents are written to
     * Amazon S3. Default value is 300 seconds (5 minutes). A value of 0 (zero) results
     * in no retries.</p>
     */
    inline int GetDurationInSeconds() const{ return m_durationInSeconds; }

    /**
     * <p>After an initial failure to deliver to Amazon ES, the total amount of time
     * during which Kinesis Data Firehose retries delivery (including the first
     * attempt). After this time has elapsed, the failed documents are written to
     * Amazon S3. Default value is 300 seconds (5 minutes). A value of 0 (zero) results
     * in no retries.</p>
     */
    inline void SetDurationInSeconds(int value) { m_durationInSecondsHasBeenSet = true; m_durationInSeconds = value; }

    /**
     * <p>After an initial failure to deliver to Amazon ES, the total amount of time
     * during which Kinesis Data Firehose retries delivery (including the first
     * attempt). After this time has elapsed, the failed documents are written to
     * Amazon S3. Default value is 300 seconds (5 minutes). A value of 0 (zero) results
     * in no retries.</p>
     */
    inline ElasticsearchRetryOptions& WithDurationInSeconds(int value) { SetDurationInSeconds(value); return *this;}

  private:

    int m_durationInSeconds;
    bool m_durationInSecondsHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
