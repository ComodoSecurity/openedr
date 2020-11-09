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

#include <aws/firehose/model/OrcFormatVersion.h>
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
      namespace OrcFormatVersionMapper
      {

        static const int V0_11_HASH = HashingUtils::HashString("V0_11");
        static const int V0_12_HASH = HashingUtils::HashString("V0_12");


        OrcFormatVersion GetOrcFormatVersionForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == V0_11_HASH)
          {
            return OrcFormatVersion::V0_11;
          }
          else if (hashCode == V0_12_HASH)
          {
            return OrcFormatVersion::V0_12;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<OrcFormatVersion>(hashCode);
          }

          return OrcFormatVersion::NOT_SET;
        }

        Aws::String GetNameForOrcFormatVersion(OrcFormatVersion enumValue)
        {
          switch(enumValue)
          {
          case OrcFormatVersion::V0_11:
            return "V0_11";
          case OrcFormatVersion::V0_12:
            return "V0_12";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace OrcFormatVersionMapper
    } // namespace Model
  } // namespace Firehose
} // namespace Aws
