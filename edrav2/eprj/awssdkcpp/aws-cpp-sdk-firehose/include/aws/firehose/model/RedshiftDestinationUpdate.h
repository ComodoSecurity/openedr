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
#include <aws/firehose/model/CopyCommand.h>
#include <aws/firehose/model/RedshiftRetryOptions.h>
#include <aws/firehose/model/S3DestinationUpdate.h>
#include <aws/firehose/model/ProcessingConfiguration.h>
#include <aws/firehose/model/RedshiftS3BackupMode.h>
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
   * <p>Describes an update for a destination in Amazon Redshift.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/RedshiftDestinationUpdate">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API RedshiftDestinationUpdate
  {
  public:
    RedshiftDestinationUpdate();
    RedshiftDestinationUpdate(Aws::Utils::Json::JsonView jsonValue);
    RedshiftDestinationUpdate& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline const Aws::String& GetRoleARN() const{ return m_roleARN; }

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetRoleARN(const Aws::String& value) { m_roleARNHasBeenSet = true; m_roleARN = value; }

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetRoleARN(Aws::String&& value) { m_roleARNHasBeenSet = true; m_roleARN = std::move(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline void SetRoleARN(const char* value) { m_roleARNHasBeenSet = true; m_roleARN.assign(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline RedshiftDestinationUpdate& WithRoleARN(const Aws::String& value) { SetRoleARN(value); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline RedshiftDestinationUpdate& WithRoleARN(Aws::String&& value) { SetRoleARN(std::move(value)); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS credentials. For more information,
     * see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/aws-arns-and-namespaces.html">Amazon
     * Resource Names (ARNs) and AWS Service Namespaces</a>.</p>
     */
    inline RedshiftDestinationUpdate& WithRoleARN(const char* value) { SetRoleARN(value); return *this;}


    /**
     * <p>The database connection string.</p>
     */
    inline const Aws::String& GetClusterJDBCURL() const{ return m_clusterJDBCURL; }

    /**
     * <p>The database connection string.</p>
     */
    inline void SetClusterJDBCURL(const Aws::String& value) { m_clusterJDBCURLHasBeenSet = true; m_clusterJDBCURL = value; }

    /**
     * <p>The database connection string.</p>
     */
    inline void SetClusterJDBCURL(Aws::String&& value) { m_clusterJDBCURLHasBeenSet = true; m_clusterJDBCURL = std::move(value); }

    /**
     * <p>The database connection string.</p>
     */
    inline void SetClusterJDBCURL(const char* value) { m_clusterJDBCURLHasBeenSet = true; m_clusterJDBCURL.assign(value); }

    /**
     * <p>The database connection string.</p>
     */
    inline RedshiftDestinationUpdate& WithClusterJDBCURL(const Aws::String& value) { SetClusterJDBCURL(value); return *this;}

    /**
     * <p>The database connection string.</p>
     */
    inline RedshiftDestinationUpdate& WithClusterJDBCURL(Aws::String&& value) { SetClusterJDBCURL(std::move(value)); return *this;}

    /**
     * <p>The database connection string.</p>
     */
    inline RedshiftDestinationUpdate& WithClusterJDBCURL(const char* value) { SetClusterJDBCURL(value); return *this;}


    /**
     * <p>The <code>COPY</code> command.</p>
     */
    inline const CopyCommand& GetCopyCommand() const{ return m_copyCommand; }

    /**
     * <p>The <code>COPY</code> command.</p>
     */
    inline void SetCopyCommand(const CopyCommand& value) { m_copyCommandHasBeenSet = true; m_copyCommand = value; }

    /**
     * <p>The <code>COPY</code> command.</p>
     */
    inline void SetCopyCommand(CopyCommand&& value) { m_copyCommandHasBeenSet = true; m_copyCommand = std::move(value); }

    /**
     * <p>The <code>COPY</code> command.</p>
     */
    inline RedshiftDestinationUpdate& WithCopyCommand(const CopyCommand& value) { SetCopyCommand(value); return *this;}

    /**
     * <p>The <code>COPY</code> command.</p>
     */
    inline RedshiftDestinationUpdate& WithCopyCommand(CopyCommand&& value) { SetCopyCommand(std::move(value)); return *this;}


    /**
     * <p>The name of the user.</p>
     */
    inline const Aws::String& GetUsername() const{ return m_username; }

    /**
     * <p>The name of the user.</p>
     */
    inline void SetUsername(const Aws::String& value) { m_usernameHasBeenSet = true; m_username = value; }

    /**
     * <p>The name of the user.</p>
     */
    inline void SetUsername(Aws::String&& value) { m_usernameHasBeenSet = true; m_username = std::move(value); }

    /**
     * <p>The name of the user.</p>
     */
    inline void SetUsername(const char* value) { m_usernameHasBeenSet = true; m_username.assign(value); }

    /**
     * <p>The name of the user.</p>
     */
    inline RedshiftDestinationUpdate& WithUsername(const Aws::String& value) { SetUsername(value); return *this;}

    /**
     * <p>The name of the user.</p>
     */
    inline RedshiftDestinationUpdate& WithUsername(Aws::String&& value) { SetUsername(std::move(value)); return *this;}

    /**
     * <p>The name of the user.</p>
     */
    inline RedshiftDestinationUpdate& WithUsername(const char* value) { SetUsername(value); return *this;}


    /**
     * <p>The user password.</p>
     */
    inline const Aws::String& GetPassword() const{ return m_password; }

    /**
     * <p>The user password.</p>
     */
    inline void SetPassword(const Aws::String& value) { m_passwordHasBeenSet = true; m_password = value; }

    /**
     * <p>The user password.</p>
     */
    inline void SetPassword(Aws::String&& value) { m_passwordHasBeenSet = true; m_password = std::move(value); }

    /**
     * <p>The user password.</p>
     */
    inline void SetPassword(const char* value) { m_passwordHasBeenSet = true; m_password.assign(value); }

    /**
     * <p>The user password.</p>
     */
    inline RedshiftDestinationUpdate& WithPassword(const Aws::String& value) { SetPassword(value); return *this;}

    /**
     * <p>The user password.</p>
     */
    inline RedshiftDestinationUpdate& WithPassword(Aws::String&& value) { SetPassword(std::move(value)); return *this;}

    /**
     * <p>The user password.</p>
     */
    inline RedshiftDestinationUpdate& WithPassword(const char* value) { SetPassword(value); return *this;}


    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver
     * documents to Amazon Redshift. Default value is 3600 (60 minutes).</p>
     */
    inline const RedshiftRetryOptions& GetRetryOptions() const{ return m_retryOptions; }

    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver
     * documents to Amazon Redshift. Default value is 3600 (60 minutes).</p>
     */
    inline void SetRetryOptions(const RedshiftRetryOptions& value) { m_retryOptionsHasBeenSet = true; m_retryOptions = value; }

    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver
     * documents to Amazon Redshift. Default value is 3600 (60 minutes).</p>
     */
    inline void SetRetryOptions(RedshiftRetryOptions&& value) { m_retryOptionsHasBeenSet = true; m_retryOptions = std::move(value); }

    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver
     * documents to Amazon Redshift. Default value is 3600 (60 minutes).</p>
     */
    inline RedshiftDestinationUpdate& WithRetryOptions(const RedshiftRetryOptions& value) { SetRetryOptions(value); return *this;}

    /**
     * <p>The retry behavior in case Kinesis Data Firehose is unable to deliver
     * documents to Amazon Redshift. Default value is 3600 (60 minutes).</p>
     */
    inline RedshiftDestinationUpdate& WithRetryOptions(RedshiftRetryOptions&& value) { SetRetryOptions(std::move(value)); return *this;}


    /**
     * <p>The Amazon S3 destination.</p> <p>The compression formats <code>SNAPPY</code>
     * or <code>ZIP</code> cannot be specified in
     * <code>RedshiftDestinationUpdate.S3Update</code> because the Amazon Redshift
     * <code>COPY</code> operation that reads from the S3 bucket doesn't support these
     * compression formats.</p>
     */
    inline const S3DestinationUpdate& GetS3Update() const{ return m_s3Update; }

    /**
     * <p>The Amazon S3 destination.</p> <p>The compression formats <code>SNAPPY</code>
     * or <code>ZIP</code> cannot be specified in
     * <code>RedshiftDestinationUpdate.S3Update</code> because the Amazon Redshift
     * <code>COPY</code> operation that reads from the S3 bucket doesn't support these
     * compression formats.</p>
     */
    inline void SetS3Update(const S3DestinationUpdate& value) { m_s3UpdateHasBeenSet = true; m_s3Update = value; }

    /**
     * <p>The Amazon S3 destination.</p> <p>The compression formats <code>SNAPPY</code>
     * or <code>ZIP</code> cannot be specified in
     * <code>RedshiftDestinationUpdate.S3Update</code> because the Amazon Redshift
     * <code>COPY</code> operation that reads from the S3 bucket doesn't support these
     * compression formats.</p>
     */
    inline void SetS3Update(S3DestinationUpdate&& value) { m_s3UpdateHasBeenSet = true; m_s3Update = std::move(value); }

    /**
     * <p>The Amazon S3 destination.</p> <p>The compression formats <code>SNAPPY</code>
     * or <code>ZIP</code> cannot be specified in
     * <code>RedshiftDestinationUpdate.S3Update</code> because the Amazon Redshift
     * <code>COPY</code> operation that reads from the S3 bucket doesn't support these
     * compression formats.</p>
     */
    inline RedshiftDestinationUpdate& WithS3Update(const S3DestinationUpdate& value) { SetS3Update(value); return *this;}

    /**
     * <p>The Amazon S3 destination.</p> <p>The compression formats <code>SNAPPY</code>
     * or <code>ZIP</code> cannot be specified in
     * <code>RedshiftDestinationUpdate.S3Update</code> because the Amazon Redshift
     * <code>COPY</code> operation that reads from the S3 bucket doesn't support these
     * compression formats.</p>
     */
    inline RedshiftDestinationUpdate& WithS3Update(S3DestinationUpdate&& value) { SetS3Update(std::move(value)); return *this;}


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
    inline RedshiftDestinationUpdate& WithProcessingConfiguration(const ProcessingConfiguration& value) { SetProcessingConfiguration(value); return *this;}

    /**
     * <p>The data processing configuration.</p>
     */
    inline RedshiftDestinationUpdate& WithProcessingConfiguration(ProcessingConfiguration&& value) { SetProcessingConfiguration(std::move(value)); return *this;}


    /**
     * <p>The Amazon S3 backup mode.</p>
     */
    inline const RedshiftS3BackupMode& GetS3BackupMode() const{ return m_s3BackupMode; }

    /**
     * <p>The Amazon S3 backup mode.</p>
     */
    inline void SetS3BackupMode(const RedshiftS3BackupMode& value) { m_s3BackupModeHasBeenSet = true; m_s3BackupMode = value; }

    /**
     * <p>The Amazon S3 backup mode.</p>
     */
    inline void SetS3BackupMode(RedshiftS3BackupMode&& value) { m_s3BackupModeHasBeenSet = true; m_s3BackupMode = std::move(value); }

    /**
     * <p>The Amazon S3 backup mode.</p>
     */
    inline RedshiftDestinationUpdate& WithS3BackupMode(const RedshiftS3BackupMode& value) { SetS3BackupMode(value); return *this;}

    /**
     * <p>The Amazon S3 backup mode.</p>
     */
    inline RedshiftDestinationUpdate& WithS3BackupMode(RedshiftS3BackupMode&& value) { SetS3BackupMode(std::move(value)); return *this;}


    /**
     * <p>The Amazon S3 destination for backup.</p>
     */
    inline const S3DestinationUpdate& GetS3BackupUpdate() const{ return m_s3BackupUpdate; }

    /**
     * <p>The Amazon S3 destination for backup.</p>
     */
    inline void SetS3BackupUpdate(const S3DestinationUpdate& value) { m_s3BackupUpdateHasBeenSet = true; m_s3BackupUpdate = value; }

    /**
     * <p>The Amazon S3 destination for backup.</p>
     */
    inline void SetS3BackupUpdate(S3DestinationUpdate&& value) { m_s3BackupUpdateHasBeenSet = true; m_s3BackupUpdate = std::move(value); }

    /**
     * <p>The Amazon S3 destination for backup.</p>
     */
    inline RedshiftDestinationUpdate& WithS3BackupUpdate(const S3DestinationUpdate& value) { SetS3BackupUpdate(value); return *this;}

    /**
     * <p>The Amazon S3 destination for backup.</p>
     */
    inline RedshiftDestinationUpdate& WithS3BackupUpdate(S3DestinationUpdate&& value) { SetS3BackupUpdate(std::move(value)); return *this;}


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
    inline RedshiftDestinationUpdate& WithCloudWatchLoggingOptions(const CloudWatchLoggingOptions& value) { SetCloudWatchLoggingOptions(value); return *this;}

    /**
     * <p>The Amazon CloudWatch logging options for your delivery stream.</p>
     */
    inline RedshiftDestinationUpdate& WithCloudWatchLoggingOptions(CloudWatchLoggingOptions&& value) { SetCloudWatchLoggingOptions(std::move(value)); return *this;}

  private:

    Aws::String m_roleARN;
    bool m_roleARNHasBeenSet;

    Aws::String m_clusterJDBCURL;
    bool m_clusterJDBCURLHasBeenSet;

    CopyCommand m_copyCommand;
    bool m_copyCommandHasBeenSet;

    Aws::String m_username;
    bool m_usernameHasBeenSet;

    Aws::String m_password;
    bool m_passwordHasBeenSet;

    RedshiftRetryOptions m_retryOptions;
    bool m_retryOptionsHasBeenSet;

    S3DestinationUpdate m_s3Update;
    bool m_s3UpdateHasBeenSet;

    ProcessingConfiguration m_processingConfiguration;
    bool m_processingConfigurationHasBeenSet;

    RedshiftS3BackupMode m_s3BackupMode;
    bool m_s3BackupModeHasBeenSet;

    S3DestinationUpdate m_s3BackupUpdate;
    bool m_s3BackupUpdateHasBeenSet;

    CloudWatchLoggingOptions m_cloudWatchLoggingOptions;
    bool m_cloudWatchLoggingOptionsHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
