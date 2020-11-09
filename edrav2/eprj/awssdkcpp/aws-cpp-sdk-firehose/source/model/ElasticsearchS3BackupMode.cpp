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

#include <aws/firehose/model/ElasticsearchS3BackupMode.h>
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
      namespace ElasticsearchS3BackupModeMapper
      {

        static const int FailedDocumentsOnly_HASH = HashingUtils::HashString("FailedDocumentsOnly");
        static const int AllDocuments_HASH = HashingUtils::HashString("AllDocuments");


        ElasticsearchS3BackupMode GetElasticsearchS3BackupModeForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == FailedDocumentsOnly_HASH)
          {
            return ElasticsearchS3BackupMode::FailedDocumentsOnly;
          }
          else if (hashCode == AllDocuments_HASH)
          {
            return ElasticsearchS3BackupMode::AllDocuments;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<ElasticsearchS3BackupMode>(hashCode);
          }

          return ElasticsearchS3BackupMode::NOT_SET;
        }

        Aws::String GetNameForElasticsearchS3BackupMode(ElasticsearchS3BackupMode enumValue)
        {
          switch(enumValue)
          {
          case ElasticsearchS3BackupMode::FailedDocumentsOnly:
            return "FailedDocumentsOnly";
          case ElasticsearchS3BackupMode::AllDocuments:
            return "AllDocuments";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace ElasticsearchS3BackupModeMapper
    } // namespace Model
  } // namespace Firehose
} // namespace Aws
