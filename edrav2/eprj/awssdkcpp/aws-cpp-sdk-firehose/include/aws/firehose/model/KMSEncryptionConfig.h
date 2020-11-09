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
   * <p>Describes an encryption key for a destination in Amazon S3.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/KMSEncryptionConfig">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API KMSEncryptionConfig
  {
  public:
    KMSEncryptionConfig();
    KMSEncryptionConfig(Aws::Utils::Json::JsonView jsonValue);
    KMSEncryptionConfig& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>The Amazon Resource Name (ARN) of the encryption key. Must belong to the same
     * AWS Region as the destination Amazon S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline const Aws::String& GetAWSKMSKeyARN() const{ return m_aWSKMSKeyARN; }

    /**
     * <p>The Amazon Resource Name (ARN) of the encryption key. Must belong to the same
     * AWS Region as the destination Amazon S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetAWSKMSKeyARN(const Aws::String& value) { m_aWSKMSKeyARNHasBeenSet = true; m_aWSKMSKeyARN = value; }

    /**
     * <p>The Amazon Resource Name (ARN) of the encryption key. Must belong to the same
     * AWS Region as the destination Amazon S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetAWSKMSKeyARN(Aws::String&& value) { m_aWSKMSKeyARNHasBeenSet = true; m_aWSKMSKeyARN = std::move(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the encryption key. Must belong to the same
     * AWS Region as the destination Amazon S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetAWSKMSKeyARN(const char* value) { m_aWSKMSKeyARNHasBeenSet = true; m_aWSKMSKeyARN.assign(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the encryption key. Must belong to the same
     * AWS Region as the destination Amazon S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline KMSEncryptionConfig& WithAWSKMSKeyARN(const Aws::String& value) { SetAWSKMSKeyARN(value); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the encryption key. Must belong to the same
     * AWS Region as the destination Amazon S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline KMSEncryptionConfig& WithAWSKMSKeyARN(Aws::String&& value) { SetAWSKMSKeyARN(std::move(value)); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the encryption key. Must belong to the same
     * AWS Region as the destination Amazon S3 bucket. For more information, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline KMSEncryptionConfig& WithAWSKMSKeyARN(const char* value) { SetAWSKMSKeyARN(value); return *this;}

  private:

    Aws::String m_aWSKMSKeyARN;
    bool m_aWSKMSKeyARNHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
