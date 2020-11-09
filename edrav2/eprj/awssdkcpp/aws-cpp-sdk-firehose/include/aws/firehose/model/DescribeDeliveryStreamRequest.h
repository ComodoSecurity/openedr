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
#include <aws/firehose/FirehoseRequest.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <utility>

namespace Aws
{
namespace Firehose
{
namespace Model
{

  /**
   */
  class AWS_FIREHOSE_API DescribeDeliveryStreamRequest : public FirehoseRequest
  {
  public:
    DescribeDeliveryStreamRequest();
    
    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "DescribeDeliveryStream"; }

    Aws::String SerializePayload() const override;

    Aws::Http::HeaderValueCollection GetRequestSpecificHeaders() const override;


    /**
     * <p>The name of the delivery stream.</p>
     */
    inline const Aws::String& GetDeliveryStreamName() const{ return m_deliveryStreamName; }

    /**
     * <p>The name of the delivery stream.</p>
     */
    inline void SetDeliveryStreamName(const Aws::String& value) { m_deliveryStreamNameHasBeenSet = true; m_deliveryStreamName = value; }

    /**
     * <p>The name of the delivery stream.</p>
     */
    inline void SetDeliveryStreamName(Aws::String&& value) { m_deliveryStreamNameHasBeenSet = true; m_deliveryStreamName = std::move(value); }

    /**
     * <p>The name of the delivery stream.</p>
     */
    inline void SetDeliveryStreamName(const char* value) { m_deliveryStreamNameHasBeenSet = true; m_deliveryStreamName.assign(value); }

    /**
     * <p>The name of the delivery stream.</p>
     */
    inline DescribeDeliveryStreamRequest& WithDeliveryStreamName(const Aws::String& value) { SetDeliveryStreamName(value); return *this;}

    /**
     * <p>The name of the delivery stream.</p>
     */
    inline DescribeDeliveryStreamRequest& WithDeliveryStreamName(Aws::String&& value) { SetDeliveryStreamName(std::move(value)); return *this;}

    /**
     * <p>The name of the delivery stream.</p>
     */
    inline DescribeDeliveryStreamRequest& WithDeliveryStreamName(const char* value) { SetDeliveryStreamName(value); return *this;}


    /**
     * <p>The limit on the number of destinations to return. You can have one
     * destination per delivery stream.</p>
     */
    inline int GetLimit() const{ return m_limit; }

    /**
     * <p>The limit on the number of destinations to return. You can have one
     * destination per delivery stream.</p>
     */
    inline void SetLimit(int value) { m_limitHasBeenSet = true; m_limit = value; }

    /**
     * <p>The limit on the number of destinations to return. You can have one
     * destination per delivery stream.</p>
     */
    inline DescribeDeliveryStreamRequest& WithLimit(int value) { SetLimit(value); return *this;}


    /**
     * <p>The ID of the destination to start returning the destination information.
     * Kinesis Data Firehose supports one destination per delivery stream.</p>
     */
    inline const Aws::String& GetExclusiveStartDestinationId() const{ return m_exclusiveStartDestinationId; }

    /**
     * <p>The ID of the destination to start returning the destination information.
     * Kinesis Data Firehose supports one destination per delivery stream.</p>
     */
    inline void SetExclusiveStartDestinationId(const Aws::String& value) { m_exclusiveStartDestinationIdHasBeenSet = true; m_exclusiveStartDestinationId = value; }

    /**
     * <p>The ID of the destination to start returning the destination information.
     * Kinesis Data Firehose supports one destination per delivery stream.</p>
     */
    inline void SetExclusiveStartDestinationId(Aws::String&& value) { m_exclusiveStartDestinationIdHasBeenSet = true; m_exclusiveStartDestinationId = std::move(value); }

    /**
     * <p>The ID of the destination to start returning the destination information.
     * Kinesis Data Firehose supports one destination per delivery stream.</p>
     */
    inline void SetExclusiveStartDestinationId(const char* value) { m_exclusiveStartDestinationIdHasBeenSet = true; m_exclusiveStartDestinationId.assign(value); }

    /**
     * <p>The ID of the destination to start returning the destination information.
     * Kinesis Data Firehose supports one destination per delivery stream.</p>
     */
    inline DescribeDeliveryStreamRequest& WithExclusiveStartDestinationId(const Aws::String& value) { SetExclusiveStartDestinationId(value); return *this;}

    /**
     * <p>The ID of the destination to start returning the destination information.
     * Kinesis Data Firehose supports one destination per delivery stream.</p>
     */
    inline DescribeDeliveryStreamRequest& WithExclusiveStartDestinationId(Aws::String&& value) { SetExclusiveStartDestinationId(std::move(value)); return *this;}

    /**
     * <p>The ID of the destination to start returning the destination information.
     * Kinesis Data Firehose supports one destination per delivery stream.</p>
     */
    inline DescribeDeliveryStreamRequest& WithExclusiveStartDestinationId(const char* value) { SetExclusiveStartDestinationId(value); return *this;}

  private:

    Aws::String m_deliveryStreamName;
    bool m_deliveryStreamNameHasBeenSet;

    int m_limit;
    bool m_limitHasBeenSet;

    Aws::String m_exclusiveStartDestinationId;
    bool m_exclusiveStartDestinationIdHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
