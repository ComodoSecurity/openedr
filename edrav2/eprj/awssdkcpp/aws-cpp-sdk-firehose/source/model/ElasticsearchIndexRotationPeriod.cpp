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

#include <aws/firehose/model/ElasticsearchIndexRotationPeriod.h>
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
      namespace ElasticsearchIndexRotationPeriodMapper
      {

        static const int NoRotation_HASH = HashingUtils::HashString("NoRotation");
        static const int OneHour_HASH = HashingUtils::HashString("OneHour");
        static const int OneDay_HASH = HashingUtils::HashString("OneDay");
        static const int OneWeek_HASH = HashingUtils::HashString("OneWeek");
        static const int OneMonth_HASH = HashingUtils::HashString("OneMonth");


        ElasticsearchIndexRotationPeriod GetElasticsearchIndexRotationPeriodForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == NoRotation_HASH)
          {
            return ElasticsearchIndexRotationPeriod::NoRotation;
          }
          else if (hashCode == OneHour_HASH)
          {
            return ElasticsearchIndexRotationPeriod::OneHour;
          }
          else if (hashCode == OneDay_HASH)
          {
            return ElasticsearchIndexRotationPeriod::OneDay;
          }
          else if (hashCode == OneWeek_HASH)
          {
            return ElasticsearchIndexRotationPeriod::OneWeek;
          }
          else if (hashCode == OneMonth_HASH)
          {
            return ElasticsearchIndexRotationPeriod::OneMonth;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<ElasticsearchIndexRotationPeriod>(hashCode);
          }

          return ElasticsearchIndexRotationPeriod::NOT_SET;
        }

        Aws::String GetNameForElasticsearchIndexRotationPeriod(ElasticsearchIndexRotationPeriod enumValue)
        {
          switch(enumValue)
          {
          case ElasticsearchIndexRotationPeriod::NoRotation:
            return "NoRotation";
          case ElasticsearchIndexRotationPeriod::OneHour:
            return "OneHour";
          case ElasticsearchIndexRotationPeriod::OneDay:
            return "OneDay";
          case ElasticsearchIndexRotationPeriod::OneWeek:
            return "OneWeek";
          case ElasticsearchIndexRotationPeriod::OneMonth:
            return "OneMonth";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace ElasticsearchIndexRotationPeriodMapper
    } // namespace Model
  } // namespace Firehose
} // namespace Aws
