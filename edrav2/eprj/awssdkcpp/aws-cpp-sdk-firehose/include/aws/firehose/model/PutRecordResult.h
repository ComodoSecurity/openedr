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
  class AWS_FIREHOSE_API PutRecordResult
  {
  public:
    PutRecordResult();
    PutRecordResult(const Aws::AmazonWebServiceResult<Aws::Utils::Json::JsonValue>& result);
    PutRecordResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Json::JsonValue>& result);


    /**
     * <p>The ID of the record.</p>
     */
    inline const Aws::String& GetRecordId() const{ return m_recordId; }

    /**
     * <p>The ID of the record.</p>
     */
    inline void SetRecordId(const Aws::String& value) { m_recordId = value; }

    /**
     * <p>The ID of the record.</p>
     */
    inline void SetRecordId(Aws::String&& value) { m_recordId = std::move(value); }

    /**
     * <p>The ID of the record.</p>
     */
    inline void SetRecordId(const char* value) { m_recordId.assign(value); }

    /**
     * <p>The ID of the record.</p>
     */
    inline PutRecordResult& WithRecordId(const Aws::String& value) { SetRecordId(value); return *this;}

    /**
     * <p>The ID of the record.</p>
     */
    inline PutRecordResult& WithRecordId(Aws::String&& value) { SetRecordId(std::move(value)); return *this;}

    /**
     * <p>The ID of the record.</p>
     */
    inline PutRecordResult& WithRecordId(const char* value) { SetRecordId(value); return *this;}


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
    inline PutRecordResult& WithEncrypted(bool value) { SetEncrypted(value); return *this;}

  private:

    Aws::String m_recordId;

    bool m_encrypted;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
