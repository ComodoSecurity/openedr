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
#include <aws/firehose/model/Tag.h>
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
  class AWS_FIREHOSE_API ListTagsForDeliveryStreamResult
  {
  public:
    ListTagsForDeliveryStreamResult();
    ListTagsForDeliveryStreamResult(const Aws::AmazonWebServiceResult<Aws::Utils::Json::JsonValue>& result);
    ListTagsForDeliveryStreamResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Json::JsonValue>& result);


    /**
     * <p>A list of tags associated with <code>DeliveryStreamName</code>, starting with
     * the first tag after <code>ExclusiveStartTagKey</code> and up to the specified
     * <code>Limit</code>.</p>
     */
    inline const Aws::Vector<Tag>& GetTags() const{ return m_tags; }

    /**
     * <p>A list of tags associated with <code>DeliveryStreamName</code>, starting with
     * the first tag after <code>ExclusiveStartTagKey</code> and up to the specified
     * <code>Limit</code>.</p>
     */
    inline void SetTags(const Aws::Vector<Tag>& value) { m_tags = value; }

    /**
     * <p>A list of tags associated with <code>DeliveryStreamName</code>, starting with
     * the first tag after <code>ExclusiveStartTagKey</code> and up to the specified
     * <code>Limit</code>.</p>
     */
    inline void SetTags(Aws::Vector<Tag>&& value) { m_tags = std::move(value); }

    /**
     * <p>A list of tags associated with <code>DeliveryStreamName</code>, starting with
     * the first tag after <code>ExclusiveStartTagKey</code> and up to the specified
     * <code>Limit</code>.</p>
     */
    inline ListTagsForDeliveryStreamResult& WithTags(const Aws::Vector<Tag>& value) { SetTags(value); return *this;}

    /**
     * <p>A list of tags associated with <code>DeliveryStreamName</code>, starting with
     * the first tag after <code>ExclusiveStartTagKey</code> and up to the specified
     * <code>Limit</code>.</p>
     */
    inline ListTagsForDeliveryStreamResult& WithTags(Aws::Vector<Tag>&& value) { SetTags(std::move(value)); return *this;}

    /**
     * <p>A list of tags associated with <code>DeliveryStreamName</code>, starting with
     * the first tag after <code>ExclusiveStartTagKey</code> and up to the specified
     * <code>Limit</code>.</p>
     */
    inline ListTagsForDeliveryStreamResult& AddTags(const Tag& value) { m_tags.push_back(value); return *this; }

    /**
     * <p>A list of tags associated with <code>DeliveryStreamName</code>, starting with
     * the first tag after <code>ExclusiveStartTagKey</code> and up to the specified
     * <code>Limit</code>.</p>
     */
    inline ListTagsForDeliveryStreamResult& AddTags(Tag&& value) { m_tags.push_back(std::move(value)); return *this; }


    /**
     * <p>If this is <code>true</code> in the response, more tags are available. To
     * list the remaining tags, set <code>ExclusiveStartTagKey</code> to the key of the
     * last tag returned and call <code>ListTagsForDeliveryStream</code> again.</p>
     */
    inline bool GetHasMoreTags() const{ return m_hasMoreTags; }

    /**
     * <p>If this is <code>true</code> in the response, more tags are available. To
     * list the remaining tags, set <code>ExclusiveStartTagKey</code> to the key of the
     * last tag returned and call <code>ListTagsForDeliveryStream</code> again.</p>
     */
    inline void SetHasMoreTags(bool value) { m_hasMoreTags = value; }

    /**
     * <p>If this is <code>true</code> in the response, more tags are available. To
     * list the remaining tags, set <code>ExclusiveStartTagKey</code> to the key of the
     * last tag returned and call <code>ListTagsForDeliveryStream</code> again.</p>
     */
    inline ListTagsForDeliveryStreamResult& WithHasMoreTags(bool value) { SetHasMoreTags(value); return *this;}

  private:

    Aws::Vector<Tag> m_tags;

    bool m_hasMoreTags;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
