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
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/DateTime.h>
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
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/KinesisStreamSourceDescription">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API KinesisStreamSourceDescription
  {
  public:
    KinesisStreamSourceDescription();
    KinesisStreamSourceDescription(Aws::Utils::Json::JsonView jsonValue);
    KinesisStreamSourceDescription& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>The Amazon Resource Name (ARN) of the source Kinesis data stream. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html#arn-syntax-kinesis-streams">Amazon
     * Kinesis Data Streams ARN Format</a>.</p>
     */
    inline const Aws::String& GetKinesisStreamARN() const{ return m_kinesisStreamARN; }

    /**
     * <p>The Amazon Resource Name (ARN) of the source Kinesis data stream. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html#arn-syntax-kinesis-streams">Amazon
     * Kinesis Data Streams ARN Format</a>.</p>
     */
    inline void SetKinesisStreamARN(const Aws::String& value) { m_kinesisStreamARNHasBeenSet = true; m_kinesisStreamARN = value; }

    /**
     * <p>The Amazon Resource Name (ARN) of the source Kinesis data stream. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html#arn-syntax-kinesis-streams">Amazon
     * Kinesis Data Streams ARN Format</a>.</p>
     */
    inline void SetKinesisStreamARN(Aws::String&& value) { m_kinesisStreamARNHasBeenSet = true; m_kinesisStreamARN = std::move(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the source Kinesis data stream. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html#arn-syntax-kinesis-streams">Amazon
     * Kinesis Data Streams ARN Format</a>.</p>
     */
    inline void SetKinesisStreamARN(const char* value) { m_kinesisStreamARNHasBeenSet = true; m_kinesisStreamARN.assign(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the source Kinesis data stream. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html#arn-syntax-kinesis-streams">Amazon
     * Kinesis Data Streams ARN Format</a>.</p>
     */
    inline KinesisStreamSourceDescription& WithKinesisStreamARN(const Aws::String& value) { SetKinesisStreamARN(value); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the source Kinesis data stream. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html#arn-syntax-kinesis-streams">Amazon
     * Kinesis Data Streams ARN Format</a>.</p>
     */
    inline KinesisStreamSourceDescription& WithKinesisStreamARN(Aws::String&& value) { SetKinesisStreamARN(std::move(value)); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the source Kinesis data stream. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html#arn-syntax-kinesis-streams">Amazon
     * Kinesis Data Streams ARN Format</a>.</p>
     */
    inline KinesisStreamSourceDescription& WithKinesisStreamARN(const char* value) { SetKinesisStreamARN(value); return *this;}


    /**
     * <p>The ARN of the role used by the source Kinesis data stream. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html#arn-syntax-iam">AWS
     * Identity and Access Management (IAM) ARN Format</a>.</p>
     */
    inline const Aws::String& GetRoleARN() const{ return m_roleARN; }

    /**
     * <p>The ARN of the role used by the source Kinesis data stream. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html#arn-syntax-iam">AWS
     * Identity and Access Management (IAM) ARN Format</a>.</p>
     */
    inline void SetRoleARN(const Aws::String& value) { m_roleARNHasBeenSet = true; m_roleARN = value; }

    /**
     * <p>The ARN of the role used by the source Kinesis data stream. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html#arn-syntax-iam">AWS
     * Identity and Access Management (IAM) ARN Format</a>.</p>
     */
    inline void SetRoleARN(Aws::String&& value) { m_roleARNHasBeenSet = true; m_roleARN = std::move(value); }

    /**
     * <p>The ARN of the role used by the source Kinesis data stream. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html#arn-syntax-iam">AWS
     * Identity and Access Management (IAM) ARN Format</a>.</p>
     */
    inline void SetRoleARN(const char* value) { m_roleARNHasBeenSet = true; m_roleARN.assign(value); }

    /**
     * <p>The ARN of the role used by the source Kinesis data stream. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html#arn-syntax-iam">AWS
     * Identity and Access Management (IAM) ARN Format</a>.</p>
     */
    inline KinesisStreamSourceDescription& WithRoleARN(const Aws::String& value) { SetRoleARN(value); return *this;}

    /**
     * <p>The ARN of the role used by the source Kinesis data stream. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html#arn-syntax-iam">AWS
     * Identity and Access Management (IAM) ARN Format</a>.</p>
     */
    inline KinesisStreamSourceDescription& WithRoleARN(Aws::String&& value) { SetRoleARN(std::move(value)); return *this;}

    /**
     * <p>The ARN of the role used by the source Kinesis data stream. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html#arn-syntax-iam">AWS
     * Identity and Access Management (IAM) ARN Format</a>.</p>
     */
    inline KinesisStreamSourceDescription& WithRoleARN(const char* value) { SetRoleARN(value); return *this;}


    /**
     * <p>Kinesis Data Firehose starts retrieving records from the Kinesis data stream
     * starting with this timestamp.</p>
     */
    inline const Aws::Utils::DateTime& GetDeliveryStartTimestamp() const{ return m_deliveryStartTimestamp; }

    /**
     * <p>Kinesis Data Firehose starts retrieving records from the Kinesis data stream
     * starting with this timestamp.</p>
     */
    inline void SetDeliveryStartTimestamp(const Aws::Utils::DateTime& value) { m_deliveryStartTimestampHasBeenSet = true; m_deliveryStartTimestamp = value; }

    /**
     * <p>Kinesis Data Firehose starts retrieving records from the Kinesis data stream
     * starting with this timestamp.</p>
     */
    inline void SetDeliveryStartTimestamp(Aws::Utils::DateTime&& value) { m_deliveryStartTimestampHasBeenSet = true; m_deliveryStartTimestamp = std::move(value); }

    /**
     * <p>Kinesis Data Firehose starts retrieving records from the Kinesis data stream
     * starting with this timestamp.</p>
     */
    inline KinesisStreamSourceDescription& WithDeliveryStartTimestamp(const Aws::Utils::DateTime& value) { SetDeliveryStartTimestamp(value); return *this;}

    /**
     * <p>Kinesis Data Firehose starts retrieving records from the Kinesis data stream
     * starting with this timestamp.</p>
     */
    inline KinesisStreamSourceDescription& WithDeliveryStartTimestamp(Aws::Utils::DateTime&& value) { SetDeliveryStartTimestamp(std::move(value)); return *this;}

  private:

    Aws::String m_kinesisStreamARN;
    bool m_kinesisStreamARNHasBeenSet;

    Aws::String m_roleARN;
    bool m_roleARNHasBeenSet;

    Aws::Utils::DateTime m_deliveryStartTimestamp;
    bool m_deliveryStartTimestampHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
