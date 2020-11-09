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

#include <aws/firehose/model/DeliveryStreamEncryptionStatus.h>
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
      namespace DeliveryStreamEncryptionStatusMapper
      {

        static const int ENABLED_HASH = HashingUtils::HashString("ENABLED");
        static const int ENABLING_HASH = HashingUtils::HashString("ENABLING");
        static const int DISABLED_HASH = HashingUtils::HashString("DISABLED");
        static const int DISABLING_HASH = HashingUtils::HashString("DISABLING");


        DeliveryStreamEncryptionStatus GetDeliveryStreamEncryptionStatusForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == ENABLED_HASH)
          {
            return DeliveryStreamEncryptionStatus::ENABLED;
          }
          else if (hashCode == ENABLING_HASH)
          {
            return DeliveryStreamEncryptionStatus::ENABLING;
          }
          else if (hashCode == DISABLED_HASH)
          {
            return DeliveryStreamEncryptionStatus::DISABLED;
          }
          else if (hashCode == DISABLING_HASH)
          {
            return DeliveryStreamEncryptionStatus::DISABLING;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<DeliveryStreamEncryptionStatus>(hashCode);
          }

          return DeliveryStreamEncryptionStatus::NOT_SET;
        }

        Aws::String GetNameForDeliveryStreamEncryptionStatus(DeliveryStreamEncryptionStatus enumValue)
        {
          switch(enumValue)
          {
          case DeliveryStreamEncryptionStatus::ENABLED:
            return "ENABLED";
          case DeliveryStreamEncryptionStatus::ENABLING:
            return "ENABLING";
          case DeliveryStreamEncryptionStatus::DISABLED:
            return "DISABLED";
          case DeliveryStreamEncryptionStatus::DISABLING:
            return "DISABLING";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace DeliveryStreamEncryptionStatusMapper
    } // namespace Model
  } // namespace Firehose
} // namespace Aws
