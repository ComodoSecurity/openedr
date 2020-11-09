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
#include <aws/firehose/model/DeliveryStreamType.h>
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
  class AWS_FIREHOSE_API ListDeliveryStreamsRequest : public FirehoseRequest
  {
  public:
    ListDeliveryStreamsRequest();
    
    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "ListDeliveryStreams"; }

    Aws::String SerializePayload() const override;

    Aws::Http::HeaderValueCollection GetRequestSpecificHeaders() const override;


    /**
     * <p>The maximum number of delivery streams to list. The default value is 10.</p>
     */
    inline int GetLimit() const{ return m_limit; }

    /**
     * <p>The maximum number of delivery streams to list. The default value is 10.</p>
     */
    inline void SetLimit(int value) { m_limitHasBeenSet = true; m_limit = value; }

    /**
     * <p>The maximum number of delivery streams to list. The default value is 10.</p>
     */
    inline ListDeliveryStreamsRequest& WithLimit(int value) { SetLimit(value); return *this;}


    /**
     * <p>The delivery stream type. This can be one of the following values:</p> <ul>
     * <li> <p> <code>DirectPut</code>: Provider applications access the delivery
     * stream directly.</p> </li> <li> <p> <code>KinesisStreamAsSource</code>: The
     * delivery stream uses a Kinesis data stream as a source.</p> </li> </ul> <p>This
     * parameter is optional. If this parameter is omitted, delivery streams of all
     * types are returned.</p>
     */
    inline const DeliveryStreamType& GetDeliveryStreamType() const{ return m_deliveryStreamType; }

    /**
     * <p>The delivery stream type. This can be one of the following values:</p> <ul>
     * <li> <p> <code>DirectPut</code>: Provider applications access the delivery
     * stream directly.</p> </li> <li> <p> <code>KinesisStreamAsSource</code>: The
     * delivery stream uses a Kinesis data stream as a source.</p> </li> </ul> <p>This
     * parameter is optional. If this parameter is omitted, delivery streams of all
     * types are returned.</p>
     */
    inline void SetDeliveryStreamType(const DeliveryStreamType& value) { m_deliveryStreamTypeHasBeenSet = true; m_deliveryStreamType = value; }

    /**
     * <p>The delivery stream type. This can be one of the following values:</p> <ul>
     * <li> <p> <code>DirectPut</code>: Provider applications access the delivery
     * stream directly.</p> </li> <li> <p> <code>KinesisStreamAsSource</code>: The
     * delivery stream uses a Kinesis data stream as a source.</p> </li> </ul> <p>This
     * parameter is optional. If this parameter is omitted, delivery streams of all
     * types are returned.</p>
     */
    inline void SetDeliveryStreamType(DeliveryStreamType&& value) { m_deliveryStreamTypeHasBeenSet = true; m_deliveryStreamType = std::move(value); }

    /**
     * <p>The delivery stream type. This can be one of the following values:</p> <ul>
     * <li> <p> <code>DirectPut</code>: Provider applications access the delivery
     * stream directly.</p> </li> <li> <p> <code>KinesisStreamAsSource</code>: The
     * delivery stream uses a Kinesis data stream as a source.</p> </li> </ul> <p>This
     * parameter is optional. If this parameter is omitted, delivery streams of all
     * types are returned.</p>
     */
    inline ListDeliveryStreamsRequest& WithDeliveryStreamType(const DeliveryStreamType& value) { SetDeliveryStreamType(value); return *this;}

    /**
     * <p>The delivery stream type. This can be one of the following values:</p> <ul>
     * <li> <p> <code>DirectPut</code>: Provider applications access the delivery
     * stream directly.</p> </li> <li> <p> <code>KinesisStreamAsSource</code>: The
     * delivery stream uses a Kinesis data stream as a source.</p> </li> </ul> <p>This
     * parameter is optional. If this parameter is omitted, delivery streams of all
     * types are returned.</p>
     */
    inline ListDeliveryStreamsRequest& WithDeliveryStreamType(DeliveryStreamType&& value) { SetDeliveryStreamType(std::move(value)); return *this;}


    /**
     * <p>The list of delivery streams returned by this call to
     * <code>ListDeliveryStreams</code> will start with the delivery stream whose name
     * comes alphabetically immediately after the name you specify in
     * <code>ExclusiveStartDeliveryStreamName</code>.</p>
     */
    inline const Aws::String& GetExclusiveStartDeliveryStreamName() const{ return m_exclusiveStartDeliveryStreamName; }

    /**
     * <p>The list of delivery streams returned by this call to
     * <code>ListDeliveryStreams</code> will start with the delivery stream whose name
     * comes alphabetically immediately after the name you specify in
     * <code>ExclusiveStartDeliveryStreamName</code>.</p>
     */
    inline void SetExclusiveStartDeliveryStreamName(const Aws::String& value) { m_exclusiveStartDeliveryStreamNameHasBeenSet = true; m_exclusiveStartDeliveryStreamName = value; }

    /**
     * <p>The list of delivery streams returned by this call to
     * <code>ListDeliveryStreams</code> will start with the delivery stream whose name
     * comes alphabetically immediately after the name you specify in
     * <code>ExclusiveStartDeliveryStreamName</code>.</p>
     */
    inline void SetExclusiveStartDeliveryStreamName(Aws::String&& value) { m_exclusiveStartDeliveryStreamNameHasBeenSet = true; m_exclusiveStartDeliveryStreamName = std::move(value); }

    /**
     * <p>The list of delivery streams returned by this call to
     * <code>ListDeliveryStreams</code> will start with the delivery stream whose name
     * comes alphabetically immediately after the name you specify in
     * <code>ExclusiveStartDeliveryStreamName</code>.</p>
     */
    inline void SetExclusiveStartDeliveryStreamName(const char* value) { m_exclusiveStartDeliveryStreamNameHasBeenSet = true; m_exclusiveStartDeliveryStreamName.assign(value); }

    /**
     * <p>The list of delivery streams returned by this call to
     * <code>ListDeliveryStreams</code> will start with the delivery stream whose name
     * comes alphabetically immediately after the name you specify in
     * <code>ExclusiveStartDeliveryStreamName</code>.</p>
     */
    inline ListDeliveryStreamsRequest& WithExclusiveStartDeliveryStreamName(const Aws::String& value) { SetExclusiveStartDeliveryStreamName(value); return *this;}

    /**
     * <p>The list of delivery streams returned by this call to
     * <code>ListDeliveryStreams</code> will start with the delivery stream whose name
     * comes alphabetically immediately after the name you specify in
     * <code>ExclusiveStartDeliveryStreamName</code>.</p>
     */
    inline ListDeliveryStreamsRequest& WithExclusiveStartDeliveryStreamName(Aws::String&& value) { SetExclusiveStartDeliveryStreamName(std::move(value)); return *this;}

    /**
     * <p>The list of delivery streams returned by this call to
     * <code>ListDeliveryStreams</code> will start with the delivery stream whose name
     * comes alphabetically immediately after the name you specify in
     * <code>ExclusiveStartDeliveryStreamName</code>.</p>
     */
    inline ListDeliveryStreamsRequest& WithExclusiveStartDeliveryStreamName(const char* value) { SetExclusiveStartDeliveryStreamName(value); return *this;}

  private:

    int m_limit;
    bool m_limitHasBeenSet;

    DeliveryStreamType m_deliveryStreamType;
    bool m_deliveryStreamTypeHasBeenSet;

    Aws::String m_exclusiveStartDeliveryStreamName;
    bool m_exclusiveStartDeliveryStreamNameHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
