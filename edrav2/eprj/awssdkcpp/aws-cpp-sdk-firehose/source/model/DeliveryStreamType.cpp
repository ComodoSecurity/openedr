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

#include <aws/firehose/model/DeliveryStreamType.h>
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
      namespace DeliveryStreamTypeMapper
      {

        static const int DirectPut_HASH = HashingUtils::HashString("DirectPut");
        static const int KinesisStreamAsSource_HASH = HashingUtils::HashString("KinesisStreamAsSource");


        DeliveryStreamType GetDeliveryStreamTypeForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == DirectPut_HASH)
          {
            return DeliveryStreamType::DirectPut;
          }
          else if (hashCode == KinesisStreamAsSource_HASH)
          {
            return DeliveryStreamType::KinesisStreamAsSource;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<DeliveryStreamType>(hashCode);
          }

          return DeliveryStreamType::NOT_SET;
        }

        Aws::String GetNameForDeliveryStreamType(DeliveryStreamType enumValue)
        {
          switch(enumValue)
          {
          case DeliveryStreamType::DirectPut:
            return "DirectPut";
          case DeliveryStreamType::KinesisStreamAsSource:
            return "KinesisStreamAsSource";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace DeliveryStreamTypeMapper
    } // namespace Model
  } // namespace Firehose
} // namespace Aws
