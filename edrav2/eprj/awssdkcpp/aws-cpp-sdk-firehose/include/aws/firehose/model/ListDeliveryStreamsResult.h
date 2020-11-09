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
#include <aws/core/utils/memory/stl/AWSVector.h>
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
  class AWS_FIREHOSE_API ListDeliveryStreamsResult
  {
  public:
    ListDeliveryStreamsResult();
    ListDeliveryStreamsResult(const Aws::AmazonWebServiceResult<Aws::Utils::Json::JsonValue>& result);
    ListDeliveryStreamsResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Json::JsonValue>& result);


    /**
     * <p>The names of the delivery streams.</p>
     */
    inline const Aws::Vector<Aws::String>& GetDeliveryStreamNames() const{ return m_deliveryStreamNames; }

    /**
     * <p>The names of the delivery streams.</p>
     */
    inline void SetDeliveryStreamNames(const Aws::Vector<Aws::String>& value) { m_deliveryStreamNames = value; }

    /**
     * <p>The names of the delivery streams.</p>
     */
    inline void SetDeliveryStreamNames(Aws::Vector<Aws::String>&& value) { m_deliveryStreamNames = std::move(value); }

    /**
     * <p>The names of the delivery streams.</p>
     */
    inline ListDeliveryStreamsResult& WithDeliveryStreamNames(const Aws::Vector<Aws::String>& value) { SetDeliveryStreamNames(value); return *this;}

    /**
     * <p>The names of the delivery streams.</p>
     */
    inline ListDeliveryStreamsResult& WithDeliveryStreamNames(Aws::Vector<Aws::String>&& value) { SetDeliveryStreamNames(std::move(value)); return *this;}

    /**
     * <p>The names of the delivery streams.</p>
     */
    inline ListDeliveryStreamsResult& AddDeliveryStreamNames(const Aws::String& value) { m_deliveryStreamNames.push_back(value); return *this; }

    /**
     * <p>The names of the delivery streams.</p>
     */
    inline ListDeliveryStreamsResult& AddDeliveryStreamNames(Aws::String&& value) { m_deliveryStreamNames.push_back(std::move(value)); return *this; }

    /**
     * <p>The names of the delivery streams.</p>
     */
    inline ListDeliveryStreamsResult& AddDeliveryStreamNames(const char* value) { m_deliveryStreamNames.push_back(value); return *this; }


    /**
     * <p>Indicates whether there are more delivery streams available to list.</p>
     */
    inline bool GetHasMoreDeliveryStreams() const{ return m_hasMoreDeliveryStreams; }

    /**
     * <p>Indicates whether there are more delivery streams available to list.</p>
     */
    inline void SetHasMoreDeliveryStreams(bool value) { m_hasMoreDeliveryStreams = value; }

    /**
     * <p>Indicates whether there are more delivery streams available to list.</p>
     */
    inline ListDeliveryStreamsResult& WithHasMoreDeliveryStreams(bool value) { SetHasMoreDeliveryStreams(value); return *this;}

  private:

    Aws::Vector<Aws::String> m_deliveryStreamNames;

    bool m_hasMoreDeliveryStreams;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
