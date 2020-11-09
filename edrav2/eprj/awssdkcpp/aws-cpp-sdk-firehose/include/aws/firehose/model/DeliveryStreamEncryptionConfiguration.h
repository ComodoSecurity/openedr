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
#include <aws/firehose/model/DeliveryStreamEncryptionStatus.h>
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
   * <p>Indicates the server-side encryption (SSE) status for the delivery
   * stream.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/DeliveryStreamEncryptionConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API DeliveryStreamEncryptionConfiguration
  {
  public:
    DeliveryStreamEncryptionConfiguration();
    DeliveryStreamEncryptionConfiguration(Aws::Utils::Json::JsonView jsonValue);
    DeliveryStreamEncryptionConfiguration& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>For a full description of the different values of this status, see
     * <a>StartDeliveryStreamEncryption</a> and
     * <a>StopDeliveryStreamEncryption</a>.</p>
     */
    inline const DeliveryStreamEncryptionStatus& GetStatus() const{ return m_status; }

    /**
     * <p>For a full description of the different values of this status, see
     * <a>StartDeliveryStreamEncryption</a> and
     * <a>StopDeliveryStreamEncryption</a>.</p>
     */
    inline void SetStatus(const DeliveryStreamEncryptionStatus& value) { m_statusHasBeenSet = true; m_status = value; }

    /**
     * <p>For a full description of the different values of this status, see
     * <a>StartDeliveryStreamEncryption</a> and
     * <a>StopDeliveryStreamEncryption</a>.</p>
     */
    inline void SetStatus(DeliveryStreamEncryptionStatus&& value) { m_statusHasBeenSet = true; m_status = std::move(value); }

    /**
     * <p>For a full description of the different values of this status, see
     * <a>StartDeliveryStreamEncryption</a> and
     * <a>StopDeliveryStreamEncryption</a>.</p>
     */
    inline DeliveryStreamEncryptionConfiguration& WithStatus(const DeliveryStreamEncryptionStatus& value) { SetStatus(value); return *this;}

    /**
     * <p>For a full description of the different values of this status, see
     * <a>StartDeliveryStreamEncryption</a> and
     * <a>StopDeliveryStreamEncryption</a>.</p>
     */
    inline DeliveryStreamEncryptionConfiguration& WithStatus(DeliveryStreamEncryptionStatus&& value) { SetStatus(std::move(value)); return *this;}

  private:

    DeliveryStreamEncryptionStatus m_status;
    bool m_statusHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
