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
  class AWS_FIREHOSE_API CreateDeliveryStreamResult
  {
  public:
    CreateDeliveryStreamResult();
    CreateDeliveryStreamResult(const Aws::AmazonWebServiceResult<Aws::Utils::Json::JsonValue>& result);
    CreateDeliveryStreamResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Json::JsonValue>& result);


    /**
     * <p>The ARN of the delivery stream.</p>
     */
    inline const Aws::String& GetDeliveryStreamARN() const{ return m_deliveryStreamARN; }

    /**
     * <p>The ARN of the delivery stream.</p>
     */
    inline void SetDeliveryStreamARN(const Aws::String& value) { m_deliveryStreamARN = value; }

    /**
     * <p>The ARN of the delivery stream.</p>
     */
    inline void SetDeliveryStreamARN(Aws::String&& value) { m_deliveryStreamARN = std::move(value); }

    /**
     * <p>The ARN of the delivery stream.</p>
     */
    inline void SetDeliveryStreamARN(const char* value) { m_deliveryStreamARN.assign(value); }

    /**
     * <p>The ARN of the delivery stream.</p>
     */
    inline CreateDeliveryStreamResult& WithDeliveryStreamARN(const Aws::String& value) { SetDeliveryStreamARN(value); return *this;}

    /**
     * <p>The ARN of the delivery stream.</p>
     */
    inline CreateDeliveryStreamResult& WithDeliveryStreamARN(Aws::String&& value) { SetDeliveryStreamARN(std::move(value)); return *this;}

    /**
     * <p>The ARN of the delivery stream.</p>
     */
    inline CreateDeliveryStreamResult& WithDeliveryStreamARN(const char* value) { SetDeliveryStreamARN(value); return *this;}

  private:

    Aws::String m_deliveryStreamARN;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
