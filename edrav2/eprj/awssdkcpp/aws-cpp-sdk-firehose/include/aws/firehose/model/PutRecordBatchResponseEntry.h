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
   * <p>Contains the result for an individual record from a <a>PutRecordBatch</a>
   * request. If the record is successfully added to your delivery stream, it
   * receives a record ID. If the record fails to be added to your delivery stream,
   * the result includes an error code and an error message.</p><p><h3>See Also:</h3>
   * <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/PutRecordBatchResponseEntry">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API PutRecordBatchResponseEntry
  {
  public:
    PutRecordBatchResponseEntry();
    PutRecordBatchResponseEntry(Aws::Utils::Json::JsonView jsonValue);
    PutRecordBatchResponseEntry& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>The ID of the record.</p>
     */
    inline const Aws::String& GetRecordId() const{ return m_recordId; }

    /**
     * <p>The ID of the record.</p>
     */
    inline void SetRecordId(const Aws::String& value) { m_recordIdHasBeenSet = true; m_recordId = value; }

    /**
     * <p>The ID of the record.</p>
     */
    inline void SetRecordId(Aws::String&& value) { m_recordIdHasBeenSet = true; m_recordId = std::move(value); }

    /**
     * <p>The ID of the record.</p>
     */
    inline void SetRecordId(const char* value) { m_recordIdHasBeenSet = true; m_recordId.assign(value); }

    /**
     * <p>The ID of the record.</p>
     */
    inline PutRecordBatchResponseEntry& WithRecordId(const Aws::String& value) { SetRecordId(value); return *this;}

    /**
     * <p>The ID of the record.</p>
     */
    inline PutRecordBatchResponseEntry& WithRecordId(Aws::String&& value) { SetRecordId(std::move(value)); return *this;}

    /**
     * <p>The ID of the record.</p>
     */
    inline PutRecordBatchResponseEntry& WithRecordId(const char* value) { SetRecordId(value); return *this;}


    /**
     * <p>The error code for an individual record result.</p>
     */
    inline const Aws::String& GetErrorCode() const{ return m_errorCode; }

    /**
     * <p>The error code for an individual record result.</p>
     */
    inline void SetErrorCode(const Aws::String& value) { m_errorCodeHasBeenSet = true; m_errorCode = value; }

    /**
     * <p>The error code for an individual record result.</p>
     */
    inline void SetErrorCode(Aws::String&& value) { m_errorCodeHasBeenSet = true; m_errorCode = std::move(value); }

    /**
     * <p>The error code for an individual record result.</p>
     */
    inline void SetErrorCode(const char* value) { m_errorCodeHasBeenSet = true; m_errorCode.assign(value); }

    /**
     * <p>The error code for an individual record result.</p>
     */
    inline PutRecordBatchResponseEntry& WithErrorCode(const Aws::String& value) { SetErrorCode(value); return *this;}

    /**
     * <p>The error code for an individual record result.</p>
     */
    inline PutRecordBatchResponseEntry& WithErrorCode(Aws::String&& value) { SetErrorCode(std::move(value)); return *this;}

    /**
     * <p>The error code for an individual record result.</p>
     */
    inline PutRecordBatchResponseEntry& WithErrorCode(const char* value) { SetErrorCode(value); return *this;}


    /**
     * <p>The error message for an individual record result.</p>
     */
    inline const Aws::String& GetErrorMessage() const{ return m_errorMessage; }

    /**
     * <p>The error message for an individual record result.</p>
     */
    inline void SetErrorMessage(const Aws::String& value) { m_errorMessageHasBeenSet = true; m_errorMessage = value; }

    /**
     * <p>The error message for an individual record result.</p>
     */
    inline void SetErrorMessage(Aws::String&& value) { m_errorMessageHasBeenSet = true; m_errorMessage = std::move(value); }

    /**
     * <p>The error message for an individual record result.</p>
     */
    inline void SetErrorMessage(const char* value) { m_errorMessageHasBeenSet = true; m_errorMessage.assign(value); }

    /**
     * <p>The error message for an individual record result.</p>
     */
    inline PutRecordBatchResponseEntry& WithErrorMessage(const Aws::String& value) { SetErrorMessage(value); return *this;}

    /**
     * <p>The error message for an individual record result.</p>
     */
    inline PutRecordBatchResponseEntry& WithErrorMessage(Aws::String&& value) { SetErrorMessage(std::move(value)); return *this;}

    /**
     * <p>The error message for an individual record result.</p>
     */
    inline PutRecordBatchResponseEntry& WithErrorMessage(const char* value) { SetErrorMessage(value); return *this;}

  private:

    Aws::String m_recordId;
    bool m_recordIdHasBeenSet;

    Aws::String m_errorCode;
    bool m_errorCodeHasBeenSet;

    Aws::String m_errorMessage;
    bool m_errorMessageHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
