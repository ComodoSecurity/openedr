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

#include <aws/core/Core_EXPORTS.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <memory>


namespace Aws
{
    namespace Client
    {
        class AWSAuthSigner;
    }
    namespace Auth
    {
        class AWS_CORE_API AWSAuthSignerProvider
        {
        public:
            virtual std::shared_ptr<Aws::Client::AWSAuthSigner> GetSigner(const Aws::String& signerName) const = 0;
            virtual ~AWSAuthSignerProvider() = default;
        };

        class AWS_CORE_API DefaultAuthSignerProvider : public AWSAuthSignerProvider
        {
        public:
            explicit DefaultAuthSignerProvider(const std::shared_ptr<Aws::Client::AWSAuthSigner>& signer);
            std::shared_ptr<Aws::Client::AWSAuthSigner> GetSigner(const Aws::String& signerName) const override;
        private:
            Aws::Vector<std::shared_ptr<Aws::Client::AWSAuthSigner>> m_signers;
        };
    }
}
