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

#include <aws/firehose/model/CompressionFormat.h>
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
      namespace CompressionFormatMapper
      {

        static const int UNCOMPRESSED_HASH = HashingUtils::HashString("UNCOMPRESSED");
        static const int GZIP_HASH = HashingUtils::HashString("GZIP");
        static const int ZIP_HASH = HashingUtils::HashString("ZIP");
        static const int Snappy_HASH = HashingUtils::HashString("Snappy");


        CompressionFormat GetCompressionFormatForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == UNCOMPRESSED_HASH)
          {
            return CompressionFormat::UNCOMPRESSED;
          }
          else if (hashCode == GZIP_HASH)
          {
            return CompressionFormat::GZIP;
          }
          else if (hashCode == ZIP_HASH)
          {
            return CompressionFormat::ZIP;
          }
          else if (hashCode == Snappy_HASH)
          {
            return CompressionFormat::Snappy;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<CompressionFormat>(hashCode);
          }

          return CompressionFormat::NOT_SET;
        }

        Aws::String GetNameForCompressionFormat(CompressionFormat enumValue)
        {
          switch(enumValue)
          {
          case CompressionFormat::UNCOMPRESSED:
            return "UNCOMPRESSED";
          case CompressionFormat::GZIP:
            return "GZIP";
          case CompressionFormat::ZIP:
            return "ZIP";
          case CompressionFormat::Snappy:
            return "Snappy";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace CompressionFormatMapper
    } // namespace Model
  } // namespace Firehose
} // namespace Aws
