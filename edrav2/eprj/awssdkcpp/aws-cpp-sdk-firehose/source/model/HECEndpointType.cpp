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

#include <aws/firehose/model/HECEndpointType.h>
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
      namespace HECEndpointTypeMapper
      {

        static const int Raw_HASH = HashingUtils::HashString("Raw");
        static const int Event_HASH = HashingUtils::HashString("Event");


        HECEndpointType GetHECEndpointTypeForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == Raw_HASH)
          {
            return HECEndpointType::Raw;
          }
          else if (hashCode == Event_HASH)
          {
            return HECEndpointType::Event;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<HECEndpointType>(hashCode);
          }

          return HECEndpointType::NOT_SET;
        }

        Aws::String GetNameForHECEndpointType(HECEndpointType enumValue)
        {
          switch(enumValue)
          {
          case HECEndpointType::Raw:
            return "Raw";
          case HECEndpointType::Event:
            return "Event";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace HECEndpointTypeMapper
    } // namespace Model
  } // namespace Firehose
} // namespace Aws
