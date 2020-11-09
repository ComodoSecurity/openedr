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

#include <aws/firehose/model/NoEncryptionConfig.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/Globals.h>
#include <aws/core/utils/EnumParseOverflowContainer.h>

using namespace Aws::Utils;


namespace Aws
{
  namespace Firehose
  {
    namespace Model
    {
      namespace NoEncryptionConfigMapper
      {

        static const int NoEncryption_HASH = HashingUtils::HashString("NoEncryption");


        NoEncryptionConfig GetNoEncryptionConfigForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == NoEncryption_HASH)
          {
            return NoEncryptionConfig::NoEncryption;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<NoEncryptionConfig>(hashCode);
          }

          return NoEncryptionConfig::NOT_SET;
        }

        Aws::String GetNameForNoEncryptionConfig(NoEncryptionConfig enumValue)
        {
          switch(enumValue)
          {
          case NoEncryptionConfig::NoEncryption:
            return "NoEncryption";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace NoEncryptionConfigMapper
    } // namespace Model
  } // namespace Firehose
} // namespace Aws
