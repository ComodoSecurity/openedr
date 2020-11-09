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
#include <aws/firehose/Firehose_EXPORTS.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/firehose/model/S3DestinationDescription.h>
#include <aws/firehose/model/ExtendedS3DestinationDescription.h>
#include <aws/firehose/model/RedshiftDestinationDescription.h>
#include <aws/firehose/model/ElasticsearchDestinationDescription.h>
#include <aws/firehose/model/SplunkDestinationDescription.h>
#include <utility>

namespace Aws
{
namespace Utils
{
namespace Json
{
  class JsonValue;
  class JsonView;
} // namespace Json
} // namespace Utils
namespace Firehose
{
namespace Model
{

  /**
   * <p>Describes the destination for a delivery stream.</p><p><h3>See Also:</h3>  
   * <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/DestinationDescription">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API DestinationDescription
  {
  public:
    DestinationDescription();
    DestinationDescription(Aws::Utils::Json::JsonView jsonValue);
    DestinationDescription& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>The ID of the destination.</p>
     */
    inline const Aws::String& GetDestinationId() const{ return m_destinationId; }

    /**
     * <p>The ID of the destination.</p>
     */
    inline void SetDestinationId(const Aws::String& value) { m_destinationIdHasBeenSet = true; m_destinationId = value; }

    /**
     * <p>The ID of the destination.</p>
     */
    inline void SetDestinationId(Aws::String&& value) { m_destinationIdHasBeenSet = true; m_destinationId = std::move(value); }

    /**
     * <p>The ID of the destination.</p>
     */
    inline void SetDestinationId(const char* value) { m_destinationIdHasBeenSet = true; m_destinationId.assign(value); }

    /**
     * <p>The ID of the destination.</p>
     */
    inline DestinationDescription& WithDestinationId(const Aws::String& value) { SetDestinationId(value); return *this;}

    /**
     * <p>The ID of the destination.</p>
     */
    inline DestinationDescription& WithDestinationId(Aws::String&& value) { SetDestinationId(std::move(value)); return *this;}

    /**
     * <p>The ID of the destination.</p>
     */
    inline DestinationDescription& WithDestinationId(const char* value) { SetDestinationId(value); return *this;}


    /**
     * <p>[Deprecated] The destination in Amazon S3.</p>
     */
    inline const S3DestinationDescription& GetS3DestinationDescription() const{ return m_s3DestinationDescription; }

    /**
     * <p>[Deprecated] The destination in Amazon S3.</p>
     */
    inline void SetS3DestinationDescription(const S3DestinationDescription& value) { m_s3DestinationDescriptionHasBeenSet = true; m_s3DestinationDescription = value; }

    /**
     * <p>[Deprecated] The destination in Amazon S3.</p>
     */
    inline void SetS3DestinationDescription(S3DestinationDescription&& value) { m_s3DestinationDescriptionHasBeenSet = true; m_s3DestinationDescription = std::move(value); }

    /**
     * <p>[Deprecated] The destination in Amazon S3.</p>
     */
    inline DestinationDescription& WithS3DestinationDescription(const S3DestinationDescription& value) { SetS3DestinationDescription(value); return *this;}

    /**
     * <p>[Deprecated] The destination in Amazon S3.</p>
     */
    inline DestinationDescription& WithS3DestinationDescription(S3DestinationDescription&& value) { SetS3DestinationDescription(std::move(value)); return *this;}


    /**
     * <p>The destination in Amazon S3.</p>
     */
    inline const ExtendedS3DestinationDescription& GetExtendedS3DestinationDescription() const{ return m_extendedS3DestinationDescription; }

    /**
     * <p>The destination in Amazon S3.</p>
     */
    inline void SetExtendedS3DestinationDescription(const ExtendedS3DestinationDescription& value) { m_extendedS3DestinationDescriptionHasBeenSet = true; m_extendedS3DestinationDescription = value; }

    /**
     * <p>The destination in Amazon S3.</p>
     */
    inline void SetExtendedS3DestinationDescription(ExtendedS3DestinationDescription&& value) { m_extendedS3DestinationDescriptionHasBeenSet = true; m_extendedS3DestinationDescription = std::move(value); }

    /**
     * <p>The destination in Amazon S3.</p>
     */
    inline DestinationDescription& WithExtendedS3DestinationDescription(const ExtendedS3DestinationDescription& value) { SetExtendedS3DestinationDescription(value); return *this;}

    /**
     * <p>The destination in Amazon S3.</p>
     */
    inline DestinationDescription& WithExtendedS3DestinationDescription(ExtendedS3DestinationDescription&& value) { SetExtendedS3DestinationDescription(std::move(value)); return *this;}


    /**
     * <p>The destination in Amazon Redshift.</p>
     */
    inline const RedshiftDestinationDescription& GetRedshiftDestinationDescription() const{ return m_redshiftDestinationDescription; }

    /**
     * <p>The destination in Amazon Redshift.</p>
     */
    inline void SetRedshiftDestinationDescription(const RedshiftDestinationDescription& value) { m_redshiftDestinationDescriptionHasBeenSet = true; m_redshiftDestinationDescription = value; }

    /**
     * <p>The destination in Amazon Redshift.</p>
     */
    inline void SetRedshiftDestinationDescription(RedshiftDestinationDescription&& value) { m_redshiftDestinationDescriptionHasBeenSet = true; m_redshiftDestinationDescription = std::move(value); }

    /**
     * <p>The destination in Amazon Redshift.</p>
     */
    inline DestinationDescription& WithRedshiftDestinationDescription(const RedshiftDestinationDescription& value) { SetRedshiftDestinationDescription(value); return *this;}

    /**
     * <p>The destination in Amazon Redshift.</p>
     */
    inline DestinationDescription& WithRedshiftDestinationDescription(RedshiftDestinationDescription&& value) { SetRedshiftDestinationDescription(std::move(value)); return *this;}


    /**
     * <p>The destination in Amazon ES.</p>
     */
    inline const ElasticsearchDestinationDescription& GetElasticsearchDestinationDescription() const{ return m_elasticsearchDestinationDescription; }

    /**
     * <p>The destination in Amazon ES.</p>
     */
    inline void SetElasticsearchDestinationDescription(const ElasticsearchDestinationDescription& value) { m_elasticsearchDestinationDescriptionHasBeenSet = true; m_elasticsearchDestinationDescription = value; }

    /**
     * <p>The destination in Amazon ES.</p>
     */
    inline void SetElasticsearchDestinationDescription(ElasticsearchDestinationDescription&& value) { m_elasticsearchDestinationDescriptionHasBeenSet = true; m_elasticsearchDestinationDescription = std::move(value); }

    /**
     * <p>The destination in Amazon ES.</p>
     */
    inline DestinationDescription& WithElasticsearchDestinationDescription(const ElasticsearchDestinationDescription& value) { SetElasticsearchDestinationDescription(value); return *this;}

    /**
     * <p>The destination in Amazon ES.</p>
     */
    inline DestinationDescription& WithElasticsearchDestinationDescription(ElasticsearchDestinationDescription&& value) { SetElasticsearchDestinationDescription(std::move(value)); return *this;}


    /**
     * <p>The destination in Splunk.</p>
     */
    inline const SplunkDestinationDescription& GetSplunkDestinationDescription() const{ return m_splunkDestinationDescription; }

    /**
     * <p>The destination in Splunk.</p>
     */
    inline void SetSplunkDestinationDescription(const SplunkDestinationDescription& value) { m_splunkDestinationDescriptionHasBeenSet = true; m_splunkDestinationDescription = value; }

    /**
     * <p>The destination in Splunk.</p>
     */
    inline void SetSplunkDestinationDescription(SplunkDestinationDescription&& value) { m_splunkDestinationDescriptionHasBeenSet = true; m_splunkDestinationDescription = std::move(value); }

    /**
     * <p>The destination in Splunk.</p>
     */
    inline DestinationDescription& WithSplunkDestinationDescription(const SplunkDestinationDescription& value) { SetSplunkDestinationDescription(value); return *this;}

    /**
     * <p>The destination in Splunk.</p>
     */
    inline DestinationDescription& WithSplunkDestinationDescription(SplunkDestinationDescription&& value) { SetSplunkDestinationDescription(std::move(value)); return *this;}

  private:

    Aws::String m_destinationId;
    bool m_destinationIdHasBeenSet;

    S3DestinationDescription m_s3DestinationDescription;
    bool m_s3DestinationDescriptionHasBeenSet;

    ExtendedS3DestinationDescription m_extendedS3DestinationDescription;
    bool m_extendedS3DestinationDescriptionHasBeenSet;

    RedshiftDestinationDescription m_redshiftDestinationDescription;
    bool m_redshiftDestinationDescriptionHasBeenSet;

    ElasticsearchDestinationDescription m_elasticsearchDestinationDescription;
    bool m_elasticsearchDestinationDescriptionHasBeenSet;

    SplunkDestinationDescription m_splunkDestinationDescription;
    bool m_splunkDestinationDescriptionHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
