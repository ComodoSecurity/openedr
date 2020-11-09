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
#include <aws/firehose/model/PutRecordBatchResponseEntry.h>
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
  class AWS_FIREHOSE_API PutRecordBatchResult
  {
  public:
    PutRecordBatchResult();
    PutRecordBatchResult(const Aws::AmazonWebServiceResult<Aws::Utils::Json::JsonValue>& result);
    PutRecordBatchResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Json::JsonValue>& result);


    /**
     * <p>The number of records that might have failed processing. This number might be
     * greater than 0 even if the <a>PutRecordBatch</a> call succeeds. Check
     * <code>FailedPutCount</code> to determine whether there are records that you need
     * to resend.</p>
     */
    inline int GetFailedPutCount() const{ return m_failedPutCount; }

    /**
     * <p>The number of records that might have failed processing. This number might be
     * greater than 0 even if the <a>PutRecordBatch</a> call succeeds. Check
     * <code>FailedPutCount</code> to determine whether there are records that you need
     * to resend.</p>
     */
    inline void SetFailedPutCount(int value) { m_failedPutCount = value; }

    /**
     * <p>The number of records that might have failed processing. This number might be
     * greater than 0 even if the <a>PutRecordBatch</a> call succeeds. Check
     * <code>FailedPutCount</code> to determine whether there are records that you need
     * to resend.</p>
     */
    inline PutRecordBatchResult& WithFailedPutCount(int value) { SetFailedPutCount(value); return *this;}


    /**
     * <p>Indicates whether server-side encryption (SSE) was enabled during this
     * operation.</p>
     */
    inline bool GetEncrypted() const{ return m_encrypted; }

    /**
     * <p>Indicates whether server-side encryption (SSE) was enabled during this
     * operation.</p>
     */
    inline void SetEncrypted(bool value) { m_encrypted = value; }

    /**
     * <p>Indicates whether server-side encryption (SSE) was enabled during this
     * operation.</p>
     */
    inline PutRecordBatchResult& WithEncrypted(bool value) { SetEncrypted(value); return *this;}


    /**
     * <p>The results array. For each record, the index of the response element is the
     * same as the index used in the request array.</p>
     */
    inline const Aws::Vector<PutRecordBatchResponseEntry>& GetRequestResponses() const{ return m_requestResponses; }

    /**
     * <p>The results array. For each record, the index of the response element is the
     * same as the index used in the request array.</p>
     */
    inline void SetRequestResponses(const Aws::Vector<PutRecordBatchResponseEntry>& value) { m_requestResponses = value; }

    /**
     * <p>The results array. For each record, the index of the response element is the
     * same as the index used in the request array.</p>
     */
    inline void SetRequestResponses(Aws::Vector<PutRecordBatchResponseEntry>&& value) { m_requestResponses = std::move(value); }

    /**
     * <p>The results array. For each record, the index of the response element is the
     * same as the index used in the request array.</p>
     */
    inline PutRecordBatchResult& WithRequestResponses(const Aws::Vector<PutRecordBatchResponseEntry>& value) { SetRequestResponses(value); return *this;}

    /**
     * <p>The results array. For each record, the index of the response element is the
     * same as the index used in the request array.</p>
     */
    inline PutRecordBatchResult& WithRequestResponses(Aws::Vector<PutRecordBatchResponseEntry>&& value) { SetRequestResponses(std::move(value)); return *this;}

    /**
     * <p>The results array. For each record, the index of the response element is the
     * same as the index used in the request array.</p>
     */
    inline PutRecordBatchResult& AddRequestResponses(const PutRecordBatchResponseEntry& value) { m_requestResponses.push_back(value); return *this; }

    /**
     * <p>The results array. For each record, the index of the response element is the
     * same as the index used in the request array.</p>
     */
    inline PutRecordBatchResult& AddRequestResponses(PutRecordBatchResponseEntry&& value) { m_requestResponses.push_back(std::move(value)); return *this; }

  private:

    int m_failedPutCount;

    bool m_encrypted;

    Aws::Vector<PutRecordBatchResponseEntry> m_requestResponses;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
