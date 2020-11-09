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

#include <aws/firehose/model/SplunkS3BackupMode.h>
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
      namespace SplunkS3BackupModeMapper
      {

        static const int FailedEventsOnly_HASH = HashingUtils::HashString("FailedEventsOnly");
        static const int AllEvents_HASH = HashingUtils::HashString("AllEvents");


        SplunkS3BackupMode GetSplunkS3BackupModeForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == FailedEventsOnly_HASH)
          {
            return SplunkS3BackupMode::FailedEventsOnly;
          }
          else if (hashCode == AllEvents_HASH)
          {
            return SplunkS3BackupMode::AllEvents;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<SplunkS3BackupMode>(hashCode);
          }

          return SplunkS3BackupMode::NOT_SET;
        }

        Aws::String GetNameForSplunkS3BackupMode(SplunkS3BackupMode enumValue)
        {
          switch(enumValue)
          {
          case SplunkS3BackupMode::FailedEventsOnly:
            return "FailedEventsOnly";
          case SplunkS3BackupMode::AllEvents:
            return "AllEvents";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace SplunkS3BackupModeMapper
    } // namespace Model
  } // namespace Firehose
} // namespace Aws
