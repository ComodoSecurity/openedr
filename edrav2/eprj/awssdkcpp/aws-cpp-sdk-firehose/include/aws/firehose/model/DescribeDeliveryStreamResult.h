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
#include <aws/firehose/model/DeliveryStreamDescription.h>
#include <utility>

namespace Aws
{
template<typename RESULT_TYPE>
class AmazonWebServiceResult;

namespace Utils
{
namespace Json
{
  class JsonValue;
} // namespace Json
} // namespace Utils
namespace Firehose
{
namespace Model
{
  class AWS_FIREHOSE_API DescribeDeliveryStreamResult
  {
  public:
    DescribeDeliveryStreamResult();
    DescribeDeliveryStreamResult(const Aws::AmazonWebServiceResult<Aws::Utils::Json::JsonValue>& result);
    DescribeDeliveryStreamResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Json::JsonValue>& result);


    /**
     * <p>Information about the delivery stream.</p>
     */
    inline const DeliveryStreamDescription& GetDeliveryStreamDescription() const{ return m_deliveryStreamDescription; }

    /**
     * <p>Information about the delivery stream.</p>
     */
    inline void SetDeliveryStreamDescription(const DeliveryStreamDescription& value) { m_deliveryStreamDescription = value; }

    /**
     * <p>Information about the delivery stream.</p>
     */
    inline void SetDeliveryStreamDescription(DeliveryStreamDescription&& value) { m_deliveryStreamDescription = std::move(value); }

    /**
     * <p>Information about the delivery stream.</p>
     */
    inline DescribeDeliveryStreamResult& WithDeliveryStreamDescription(const DeliveryStreamDescription& value) { SetDeliveryStreamDescription(value); return *this;}

    /**
     * <p>Information about the delivery stream.</p>
     */
    inline DescribeDeliveryStreamResult& WithDeliveryStreamDescription(DeliveryStreamDescription&& value) { SetDeliveryStreamDescription(std::move(value)); return *this;}

  private:

    DeliveryStreamDescription m_deliveryStreamDescription;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
