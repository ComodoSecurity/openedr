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
#include <aws/firehose/model/HECEndpointType.h>
#include <aws/firehose/model/SplunkRetryOptions.h>
#include <aws/firehose/model/SplunkS3BackupMode.h>
#include <aws/firehose/model/S3DestinationConfiguration.h>
#include <aws/firehose/model/ProcessingConfiguration.h>
#include <aws/firehose/model/CloudWatchLoggingOptions.h>
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
   * <p>Describes the configuration of a destination in Splunk.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/SplunkDestinationConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API SplunkDestinationConfiguration
  {
  public:
    SplunkDestinationConfiguration();
    SplunkDestinationConfiguration(Aws::Utils::Json::JsonView jsonValue);
    SplunkDestinationConfiguration& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>The HTTP Event Collector (HEC) endpoint to which Kinesis Data Firehose sends
     * your data.</p>
     */
    inline const Aws::String& GetHECEndpoint() const{ return m_hECEndpoint; }

    /**
     * <p>The HTTP Event Collector (HEC) endpoint to which Kinesis Data Firehose sends
     * your data.</p>
     */
    inline void SetHECEndpoint(const Aws::String& value) { m_hECEndpointHasBeenSet = true; m_hECEndpoint = value; }

    /**
     * <p>The HTTP Event Collector (HEC) endpoint to which Kinesis Data Firehose sends
     * your data.</p>
     */
    inline void SetHECEndpoint(Aws::String&& value) { m_hECEndpointHasBeenSet = true; m_hECEndpoint = std::move(value); }

    /**
     * <p>The HTTP Event Collector (HEC) endpoint to which Kinesis Data Firehose sends
     * your data.</p>
     */
    inline void SetHECEndpoint(const char* value) { m_hECEndpointHasBeenSet = true; m_hECEndpoint.assign(value); }

    /**
     * <p>The HTTP Event Collector (HEC) endpoint to which Kinesis Data Firehose sends
     * your data.</p>
     */
    inline SplunkDestinationConfiguration& WithHECEndpoint(const Aws::String& value) { SetHECEndpoint(value); return *this;}

    /**
     * <p>The HTTP Event Collector (HEC) endpoint to which Kinesis Data Firehose sends
     * your data.</p>
     */
    inline SplunkDestinationConfiguration& WithHECEndpoint(Aws::String&& value) { SetHECEndpoint(std::move(value)); return *this;}

    /**
     * <p>The HTTP Event Collector (HEC) endpoint to which Kinesis Data Firehose sends
     * your data.</p>
     */
    inline SplunkDestinationConfiguration& WithHECEndpoint(const char* value) { SetHECEndpoint(value); return *this;}


    /**
     * <p>This type can be either "Raw" or "Event."</p>
     */
    inline const HECEndpointType& GetHECEndpointType() const{ return m_hECEndpointType; }

    /**
     * <p>This type can be either "Raw" or "Event."</p>
     */
    inline void SetHECEndpointType(const HECEndpointType& value) { m_hECEndpointTypeHasBeenSet = true; m_hECEndpointType = value; }

    /**
     * <p>This type can be either "Raw" or "Event."</p>
     */
    inline void SetHECEndpointType(HECEndpointType&& value) { m_hECEndpointTypeHasBeenSet = true; m_hECEndpointType = std::move(value); }

    /**
     * <p>This type can be either "Raw" or "Event."</p>
     */
    inline SplunkDestinationConfiguration& WithHECEndpointType(const HECEndpointType& value) { SetHECEndpointType(value); return *this;}

    /**
     * <p>This type can be either "Raw" or "Event."</p>
     */
    inline SplunkDestinationConfiguration& WithHECEndpointType(HECEndpointType&& value) { SetHECEndpointType(std::move(value)); return *this;}


    /**
     * <p>This is a GUID that you obtain from your Splunk cluster when you create a new
     * HEC endpoint.</p>
     */
    inline const Aws::String& GetHECToken() const{ return m_hECToken; }

    /**
     * <p>This is a GUID that you obtain from your Splunk cluster when you create a new
     * HEC endpoint.</p>
     */
    inline void SetHECToken(const Aws::String& value) { m_hECTokenHasBeenSet = true; m_hECToken = value; }

    /**
     * <p>This is a GUID that you obtain from your Splunk cluster when you create a new
     * HEC endpoint.</p>
     */
    inline void SetHECToken(Aws::String&& value) { m_hECTokenHasBeenSet = true; m_hECToken = std::move(value); }

    /**
     * <p>This is a GUID that you obtain from your Splunk cluster when you create a new
     * HEC endpoint.</p>
     */
    inline void SetHECToken(const char* value) { m_hECTokenHasBeenSet = true; m_hECToken.assign(value); }

    /**
     * <p>This is a GUID that you obtain from your Splunk cluster when you create a new
     * HEC endpoint.</p>
     */
    inline SplunkDestinationConfiguration& WithHECToken(const Aws::String& value) { SetHECToken(value); return *this;}

    /**
     * <p>This is a GUID that you obtain from your Splunk cluster when you create a new
     * HEC endpoint.</p>
     */
    inline SplunkDestinationConfiguration& WithHECToken(Aws::String&& value) { SetHECToken(std::move(value)); return *this;}

    /**
     * <p>This is a GUID that you obtain from your Splunk cluster when you create a new
     * HEC endpoint.</p>
     */
    inline SplunkDestinationConfiguration& WithHECToken(const char* value) { SetHECToken(value); return *this;}


    /**
     * <p>The amount of time that Kinesis Data Firehose waits to receive an
     * acknowledgment from Splunk after it sends it data. At the end of the timeout
     * period, Kinesis Data Firehose either tries to send the data again or considers
     * it an error, based on your retry settings.</p>
     */
    inline int GetHECAcknowledgmentTimeoutInSeconds() const{ return m_hECAcknowledgmentTimeoutInSeconds; }

    /**
     * <p>The amount of time that Kinesis Data Firehose waits to receive an
     * acknowledgment from Splunk after it sends it data. At the end of the timeout
     * period, Kinesis Data Firehose either tries to send the data again or considers
     * it an error, based on your retry settings.</p>
     */
    inline void SetHECAcknowledgmentTimeoutInSeconds(int value) { m_hECAcknowledgmentTimeoutInSecondsHasBeenSet = true; m_hECAcknowledgmentTimeoutInSeconds = value; }

    /**
     * <p>The amount of time that Kinesis Data Firehose waits to receive an
     * acknowledgment from Splunk after it sends it data. At the end of the timeout
     * period, Kinesis Data Firehose either tries to send the data again or considers
     * it an error, based on your retry settings.</p>
     */
    inline SplunkDestinationConfiguration& WithHECAcknowledgmentTimeoutInSeconds(int value) { SetHECAcknowledgmentTimeoutInSeconds(value); return *this;}


    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver data to
     * Splunk, or if it doesn't receive an acknowledgment of receipt from Splunk.</p>
     */
    inline const SplunkRetryOptions& GetRetryOptions() const{ return m_retryOptions; }

    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver data to
     * Splunk, or if it doesn't receive an acknowledgment of receipt from Splunk.</p>
     */
    inline void SetRetryOptions(const SplunkRetryOptions& value) { m_retryOptionsHasBeenSet = true; m_retryOptions = value; }

    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver data to
     * Splunk, or if it doesn't receive an acknowledgment of receipt from Splunk.</p>
     */
    inline void SetRetryOptions(SplunkRetryOptions&& value) { m_retryOptionsHasBeenSet = true; m_retryOptions = std::move(value); }

    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver data to
     * Splunk, or if it doesn't receive an acknowledgment of receipt from Splunk.</p>
     */
    inline SplunkDestinationConfiguration& WithRetryOptions(const SplunkRetryOptions& value) { SetRetryOptions(value); return *this;}

    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver data to
     * Splunk, or if it doesn't receive an acknowledgment of receipt from Splunk.</p>
     */
    inline SplunkDestinationConfiguration& WithRetryOptions(SplunkRetryOptions&& value) { SetRetryOptions(std::move(value)); return *this;}


    /**
     * <p>Defines how documents should be delivered to Amazon S3. When set to
     * <code>FailedDocumentsOnly</code>, Kinesis Data Firehose writes any data that
     * could not be indexed to the configured Amazon S3 destination. When set to
     * <code>AllDocuments</code>, Kinesis Data Firehose delivers all incoming records
     * to Amazon S3, and also writes failed documents to Amazon S3. Default value is
     * <code>FailedDocumentsOnly</code>. </p>
     */
    inline const SplunkS3BackupMode& GetS3BackupMode() const{ return m_s3BackupMode; }

    /**
     * <p>Defines how documents should be delivered to Amazon S3. When set to
     * <code>FailedDocumentsOnly</code>, Kinesis Data Firehose writes any data that
     * could not be indexed to the configured Amazon S3 destination. When set to
     * <code>AllDocuments</code>, Kinesis Data Firehose delivers all incoming records
     * to Amazon S3, and also writes failed documents to Amazon S3. Default value is
     * <code>FailedDocumentsOnly</code>. </p>
     */
    inline void SetS3BackupMode(const SplunkS3BackupMode& value) { m_s3BackupModeHasBeenSet = true; m_s3BackupMode = value; }

    /**
     * <p>Defines how documents should be delivered to Amazon S3. When set to
     * <code>FailedDocumentsOnly</code>, Kinesis Data Firehose writes any data that
     * could not be indexed to the configured Amazon S3 destination. When set to
     * <code>AllDocuments</code>, Kinesis Data Firehose delivers all incoming records
     * to Amazon S3, and also writes failed documents to Amazon S3. Default value is
     * <code>FailedDocumentsOnly</code>. </p>
     */
    inline void SetS3BackupMode(SplunkS3BackupMode&& value) { m_s3BackupModeHasBeenSet = true; m_s3BackupMode = std::move(value); }

    /**
     * <p>Defines how documents should be delivered to Amazon S3. When set to
     * <code>FailedDocumentsOnly</code>, Kinesis Data Firehose writes any data that
     * could not be indexed to the configured Amazon S3 destination. When set to
     * <code>AllDocuments</code>, Kinesis Data Firehose delivers all incoming records
     * to Amazon S3, and also writes failed documents to Amazon S3. Default value is
     * <code>FailedDocumentsOnly</code>. </p>
     */
    inline SplunkDestinationConfiguration& WithS3BackupMode(const SplunkS3BackupMode& value) { SetS3BackupMode(value); return *this;}

    /**
     * <p>Defines how documents should be delivered to Amazon S3. When set to
     * <code>FailedDocumentsOnly</code>, Kinesis Data Firehose writes any data that
     * could not be indexed to the configured Amazon S3 destination. When set to
     * <code>AllDocuments</code>, Kinesis Data Firehose delivers all incoming records
     * to Amazon S3, and also writes failed documents to Amazon S3. Default value is
     * <code>FailedDocumentsOnly</code>. </p>
     */
    inline SplunkDestinationConfiguration& WithS3BackupMode(SplunkS3BackupMode&& value) { SetS3BackupMode(std::move(value)); return *this;}


    /**
     * <p>The configuration for the backup Amazon S3 location.</p>
     */
    inline const S3DestinationConfiguration& GetS3Configuration() const{ return m_s3Configuration; }

    /**
     * <p>The configuration for the backup Amazon S3 location.</p>
     */
    inline void SetS3Configuration(const S3DestinationConfiguration& value) { m_s3ConfigurationHasBeenSet = true; m_s3Configuration = value; }

    /**
     * <p>The configuration for the backup Amazon S3 location.</p>
     */
    inline void SetS3Configuration(S3DestinationConfiguration&& value) { m_s3ConfigurationHasBeenSet = true; m_s3Configuration = std::move(value); }

    /**
     * <p>The configuration for the backup Amazon S3 location.</p>
     */
    inline SplunkDestinationConfiguration& WithS3Configuration(const S3DestinationConfiguration& value) { SetS3Configuration(value); return *this;}

    /**
     * <p>The configuration for the backup Amazon S3 location.</p>
     */
    inline SplunkDestinationConfiguration& WithS3Configuration(S3DestinationConfiguration&& value) { SetS3Configuration(std::move(value)); return *this;}


    /**
     * <p>The data processing configuration.</p>
     */
    inline const ProcessingConfiguration& GetProcessingConfiguration() const{ return m_processingConfiguration; }

    /**
     * <p>The data processing configuration.</p>
     */
    inline void SetProcessingConfiguration(const ProcessingConfiguration& value) { m_processingConfigurationHasBeenSet = true; m_processingConfiguration = value; }

    /**
     * <p>The data processing configuration.</p>
     */
    inline void SetProcessingConfiguration(ProcessingConfiguration&& value) { m_processingConfigurationHasBeenSet = true; m_processingConfiguration = std::move(value); }

    /**
     * <p>The data processing configuration.</p>
     */
    inline SplunkDestinationConfiguration& WithProcessingConfiguration(const ProcessingConfiguration& value) { SetProcessingConfiguration(value); return *this;}

    /**
     * <p>The data processing configuration.</p>
     */
    inline SplunkDestinationConfiguration& WithProcessingConfiguration(ProcessingConfiguration&& value) { SetProcessingConfiguration(std::move(value)); return *this;}


    /**
     * <p>The Amazon CloudWatch logging options for your delivery stream.</p>
     */
    inline const CloudWatchLoggingOptions& GetCloudWatchLoggingOptions() const{ return m_cloudWatchLoggingOptions; }

    /**
     * <p>The Amazon CloudWatch logging options for your delivery stream.</p>
     */
    inline void SetCloudWatchLoggingOptions(const CloudWatchLoggingOptions& value) { m_cloudWatchLoggingOptionsHasBeenSet = true; m_cloudWatchLoggingOptions = value; }

    /**
     * <p>The Amazon CloudWatch logging options for your delivery stream.</p>
     */
    inline void SetCloudWatchLoggingOptions(CloudWatchLoggingOptions&& value) { m_cloudWatchLoggingOptionsHasBeenSet = true; m_cloudWatchLoggingOptions = std::move(value); }

    /**
     * <p>The Amazon CloudWatch logging options for your delivery stream.</p>
     */
    inline SplunkDestinationConfiguration& WithCloudWatchLoggingOptions(const CloudWatchLoggingOptions& value) { SetCloudWatchLoggingOptions(value); return *this;}

    /**
     * <p>The Amazon CloudWatch logging options for your delivery stream.</p>
     */
    inline SplunkDestinationConfiguration& WithCloudWatchLoggingOptions(CloudWatchLoggingOptions&& value) { SetCloudWatchLoggingOptions(std::move(value)); return *this;}

  private:

    Aws::String m_hECEndpoint;
    bool m_hECEndpointHasBeenSet;

    HECEndpointType m_hECEndpointType;
    bool m_hECEndpointTypeHasBeenSet;

    Aws::String m_hECToken;
    bool m_hECTokenHasBeenSet;

    int m_hECAcknowledgmentTimeoutInSeconds;
    bool m_hECAcknowledgmentTimeoutInSecondsHasBeenSet;

    SplunkRetryOptions m_retryOptions;
    bool m_retryOptionsHasBeenSet;

    SplunkS3BackupMode m_s3BackupMode;
    bool m_s3BackupModeHasBeenSet;

    S3DestinationConfiguration m_s3Configuration;
    bool m_s3ConfigurationHasBeenSet;

    ProcessingConfiguration m_processingConfiguration;
    bool m_processingConfigurationHasBeenSet;

    CloudWatchLoggingOptions m_cloudWatchLoggingOptions;
    bool m_cloudWatchLoggingOptionsHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
