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
#include <aws/firehose/model/NoEncryptionConfig.h>
#include <aws/firehose/model/KMSEncryptionConfig.h>
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
   * <p>Describes the encryption for a destination in Amazon S3.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/EncryptionConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API EncryptionConfiguration
  {
  public:
    EncryptionConfiguration();
    EncryptionConfiguration(Aws::Utils::Json::JsonView jsonValue);
    EncryptionConfiguration& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>Specifically override existing encryption information to ensure that no
     * encryption is used.</p>
     */
    inline const NoEncryptionConfig& GetNoEncryptionConfig() const{ return m_noEncryptionConfig; }

    /**
     * <p>Specifically override existing encryption information to ensure that no
     * encryption is used.</p>
     */
    inline void SetNoEncryptionConfig(const NoEncryptionConfig& value) { m_noEncryptionConfigHasBeenSet = true; m_noEncryptionConfig = value; }

    /**
     * <p>Specifically override existing encryption information to ensure that no
     * encryption is used.</p>
     */
    inline void SetNoEncryptionConfig(NoEncryptionConfig&& value) { m_noEncryptionConfigHasBeenSet = true; m_noEncryptionConfig = std::move(value); }

    /**
     * <p>Specifically override existing encryption information to ensure that no
     * encryption is used.</p>
     */
    inline EncryptionConfiguration& WithNoEncryptionConfig(const NoEncryptionConfig& value) { SetNoEncryptionConfig(value); return *this;}

    /**
     * <p>Specifically override existing encryption information to ensure that no
     * encryption is used.</p>
     */
    inline EncryptionConfiguration& WithNoEncryptionConfig(NoEncryptionConfig&& value) { SetNoEncryptionConfig(std::move(value)); return *this;}


    /**
     * <p>The encryption key.</p>
     */
    inline const KMSEncryptionConfig& GetKMSEncryptionConfig() const{ return m_kMSEncryptionConfig; }

    /**
     * <p>The encryption key.</p>
     */
    inline void SetKMSEncryptionConfig(const KMSEncryptionConfig& value) { m_kMSEncryptionConfigHasBeenSet = true; m_kMSEncryptionConfig = value; }

    /**
     * <p>The encryption key.</p>
     */
    inline void SetKMSEncryptionConfig(KMSEncryptionConfig&& value) { m_kMSEncryptionConfigHasBeenSet = true; m_kMSEncryptionConfig = std::move(value); }

    /**
     * <p>The encryption key.</p>
     */
    inline EncryptionConfiguration& WithKMSEncryptionConfig(const KMSEncryptionConfig& value) { SetKMSEncryptionConfig(value); return *this;}

    /**
     * <p>The encryption key.</p>
     */
    inline EncryptionConfiguration& WithKMSEncryptionConfig(KMSEncryptionConfig&& value) { SetKMSEncryptionConfig(std::move(value)); return *this;}

  private:

    NoEncryptionConfig m_noEncryptionConfig;
    bool m_noEncryptionConfigHasBeenSet;

    KMSEncryptionConfig m_kMSEncryptionConfig;
    bool m_kMSEncryptionConfigHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
