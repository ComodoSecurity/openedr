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
   * <p>Specifies the schema to which you want Kinesis Data Firehose to configure
   * your data before it writes it to Amazon S3.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/SchemaConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API SchemaConfiguration
  {
  public:
    SchemaConfiguration();
    SchemaConfiguration(Aws::Utils::Json::JsonView jsonValue);
    SchemaConfiguration& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>The role that Kinesis Data Firehose can use to access AWS Glue. This role
     * must be in the same account you use for Kinesis Data Firehose. Cross-account
     * roles aren't allowed.</p>
     */
    inline const Aws::String& GetRoleARN() const{ return m_roleARN; }

    /**
     * <p>The role that Kinesis Data Firehose can use to access AWS Glue. This role
     * must be in the same account you use for Kinesis Data Firehose. Cross-account
     * roles aren't allowed.</p>
     */
    inline void SetRoleARN(const Aws::String& value) { m_roleARNHasBeenSet = true; m_roleARN = value; }

    /**
     * <p>The role that Kinesis Data Firehose can use to access AWS Glue. This role
     * must be in the same account you use for Kinesis Data Firehose. Cross-account
     * roles aren't allowed.</p>
     */
    inline void SetRoleARN(Aws::String&& value) { m_roleARNHasBeenSet = true; m_roleARN = std::move(value); }

    /**
     * <p>The role that Kinesis Data Firehose can use to access AWS Glue. This role
     * must be in the same account you use for Kinesis Data Firehose. Cross-account
     * roles aren't allowed.</p>
     */
    inline void SetRoleARN(const char* value) { m_roleARNHasBeenSet = true; m_roleARN.assign(value); }

    /**
     * <p>The role that Kinesis Data Firehose can use to access AWS Glue. This role
     * must be in the same account you use for Kinesis Data Firehose. Cross-account
     * roles aren't allowed.</p>
     */
    inline SchemaConfiguration& WithRoleARN(const Aws::String& value) { SetRoleARN(value); return *this;}

    /**
     * <p>The role that Kinesis Data Firehose can use to access AWS Glue. This role
     * must be in the same account you use for Kinesis Data Firehose. Cross-account
     * roles aren't allowed.</p>
     */
    inline SchemaConfiguration& WithRoleARN(Aws::String&& value) { SetRoleARN(std::move(value)); return *this;}

    /**
     * <p>The role that Kinesis Data Firehose can use to access AWS Glue. This role
     * must be in the same account you use for Kinesis Data Firehose. Cross-account
     * roles aren't allowed.</p>
     */
    inline SchemaConfiguration& WithRoleARN(const char* value) { SetRoleARN(value); return *this;}


    /**
     * <p>The ID of the AWS Glue Data Catalog. If you don't supply this, the AWS
     * account ID is used by default.</p>
     */
    inline const Aws::String& GetCatalogId() const{ return m_catalogId; }

    /**
     * <p>The ID of the AWS Glue Data Catalog. If you don't supply this, the AWS
     * account ID is used by default.</p>
     */
    inline void SetCatalogId(const Aws::String& value) { m_catalogIdHasBeenSet = true; m_catalogId = value; }

    /**
     * <p>The ID of the AWS Glue Data Catalog. If you don't supply this, the AWS
     * account ID is used by default.</p>
     */
    inline void SetCatalogId(Aws::String&& value) { m_catalogIdHasBeenSet = true; m_catalogId = std::move(value); }

    /**
     * <p>The ID of the AWS Glue Data Catalog. If you don't supply this, the AWS
     * account ID is used by default.</p>
     */
    inline void SetCatalogId(const char* value) { m_catalogIdHasBeenSet = true; m_catalogId.assign(value); }

    /**
     * <p>The ID of the AWS Glue Data Catalog. If you don't supply this, the AWS
     * account ID is used by default.</p>
     */
    inline SchemaConfiguration& WithCatalogId(const Aws::String& value) { SetCatalogId(value); return *this;}

    /**
     * <p>The ID of the AWS Glue Data Catalog. If you don't supply this, the AWS
     * account ID is used by default.</p>
     */
    inline SchemaConfiguration& WithCatalogId(Aws::String&& value) { SetCatalogId(std::move(value)); return *this;}

    /**
     * <p>The ID of the AWS Glue Data Catalog. If you don't supply this, the AWS
     * account ID is used by default.</p>
     */
    inline SchemaConfiguration& WithCatalogId(const char* value) { SetCatalogId(value); return *this;}


    /**
     * <p>Specifies the name of the AWS Glue database that contains the schema for the
     * output data.</p>
     */
    inline const Aws::String& GetDatabaseName() const{ return m_databaseName; }

    /**
     * <p>Specifies the name of the AWS Glue database that contains the schema for the
     * output data.</p>
     */
    inline void SetDatabaseName(const Aws::String& value) { m_databaseNameHasBeenSet = true; m_databaseName = value; }

    /**
     * <p>Specifies the name of the AWS Glue database that contains the schema for the
     * output data.</p>
     */
    inline void SetDatabaseName(Aws::String&& value) { m_databaseNameHasBeenSet = true; m_databaseName = std::move(value); }

    /**
     * <p>Specifies the name of the AWS Glue database that contains the schema for the
     * output data.</p>
     */
    inline void SetDatabaseName(const char* value) { m_databaseNameHasBeenSet = true; m_databaseName.assign(value); }

    /**
     * <p>Specifies the name of the AWS Glue database that contains the schema for the
     * output data.</p>
     */
    inline SchemaConfiguration& WithDatabaseName(const Aws::String& value) { SetDatabaseName(value); return *this;}

    /**
     * <p>Specifies the name of the AWS Glue database that contains the schema for the
     * output data.</p>
     */
    inline SchemaConfiguration& WithDatabaseName(Aws::String&& value) { SetDatabaseName(std::move(value)); return *this;}

    /**
     * <p>Specifies the name of the AWS Glue database that contains the schema for the
     * output data.</p>
     */
    inline SchemaConfiguration& WithDatabaseName(const char* value) { SetDatabaseName(value); return *this;}


    /**
     * <p>Specifies the AWS Glue table that contains the column information that
     * constitutes your data schema.</p>
     */
    inline const Aws::String& GetTableName() const{ return m_tableName; }

    /**
     * <p>Specifies the AWS Glue table that contains the column information that
     * constitutes your data schema.</p>
     */
    inline void SetTableName(const Aws::String& value) { m_tableNameHasBeenSet = true; m_tableName = value; }

    /**
     * <p>Specifies the AWS Glue table that contains the column information that
     * constitutes your data schema.</p>
     */
    inline void SetTableName(Aws::String&& value) { m_tableNameHasBeenSet = true; m_tableName = std::move(value); }

    /**
     * <p>Specifies the AWS Glue table that contains the column information that
     * constitutes your data schema.</p>
     */
    inline void SetTableName(const char* value) { m_tableNameHasBeenSet = true; m_tableName.assign(value); }

    /**
     * <p>Specifies the AWS Glue table that contains the column information that
     * constitutes your data schema.</p>
     */
    inline SchemaConfiguration& WithTableName(const Aws::String& value) { SetTableName(value); return *this;}

    /**
     * <p>Specifies the AWS Glue table that contains the column information that
     * constitutes your data schema.</p>
     */
    inline SchemaConfiguration& WithTableName(Aws::String&& value) { SetTableName(std::move(value)); return *this;}

    /**
     * <p>Specifies the AWS Glue table that contains the column information that
     * constitutes your data schema.</p>
     */
    inline SchemaConfiguration& WithTableName(const char* value) { SetTableName(value); return *this;}


    /**
     * <p>If you don't specify an AWS Region, the default is the current Region.</p>
     */
    inline const Aws::String& GetRegion() const{ return m_region; }

    /**
     * <p>If you don't specify an AWS Region, the default is the current Region.</p>
     */
    inline void SetRegion(const Aws::String& value) { m_regionHasBeenSet = true; m_region = value; }

    /**
     * <p>If you don't specify an AWS Region, the default is the current Region.</p>
     */
    inline void SetRegion(Aws::String&& value) { m_regionHasBeenSet = true; m_region = std::move(value); }

    /**
     * <p>If you don't specify an AWS Region, the default is the current Region.</p>
     */
    inline void SetRegion(const char* value) { m_regionHasBeenSet = true; m_region.assign(value); }

    /**
     * <p>If you don't specify an AWS Region, the default is the current Region.</p>
     */
    inline SchemaConfiguration& WithRegion(const Aws::String& value) { SetRegion(value); return *this;}

    /**
     * <p>If you don't specify an AWS Region, the default is the current Region.</p>
     */
    inline SchemaConfiguration& WithRegion(Aws::String&& value) { SetRegion(std::move(value)); return *this;}

    /**
     * <p>If you don't specify an AWS Region, the default is the current Region.</p>
     */
    inline SchemaConfiguration& WithRegion(const char* value) { SetRegion(value); return *this;}


    /**
     * <p>Specifies the table version for the output data schema. If you don't specify
     * this version ID, or if you set it to <code>LATEST</code>, Kinesis Data Firehose
     * uses the most recent version. This means that any updates to the table are
     * automatically picked up.</p>
     */
    inline const Aws::String& GetVersionId() const{ return m_versionId; }

    /**
     * <p>Specifies the table version for the output data schema. If you don't specify
     * this version ID, or if you set it to <code>LATEST</code>, Kinesis Data Firehose
     * uses the most recent version. This means that any updates to the table are
     * automatically picked up.</p>
     */
    inline void SetVersionId(const Aws::String& value) { m_versionIdHasBeenSet = true; m_versionId = value; }

    /**
     * <p>Specifies the table version for the output data schema. If you don't specify
     * this version ID, or if you set it to <code>LATEST</code>, Kinesis Data Firehose
     * uses the most recent version. This means that any updates to the table are
     * automatically picked up.</p>
     */
    inline void SetVersionId(Aws::String&& value) { m_versionIdHasBeenSet = true; m_versionId = std::move(value); }

    /**
     * <p>Specifies the table version for the output data schema. If you don't specify
     * this version ID, or if you set it to <code>LATEST</code>, Kinesis Data Firehose
     * uses the most recent version. This means that any updates to the table are
     * automatically picked up.</p>
     */
    inline void SetVersionId(const char* value) { m_versionIdHasBeenSet = true; m_versionId.assign(value); }

    /**
     * <p>Specifies the table version for the output data schema. If you don't specify
     * this version ID, or if you set it to <code>LATEST</code>, Kinesis Data Firehose
     * uses the most recent version. This means that any updates to the table are
     * automatically picked up.</p>
     */
    inline SchemaConfiguration& WithVersionId(const Aws::String& value) { SetVersionId(value); return *this;}

    /**
     * <p>Specifies the table version for the output data schema. If you don't specify
     * this version ID, or if you set it to <code>LATEST</code>, Kinesis Data Firehose
     * uses the most recent version. This means that any updates to the table are
     * automatically picked up.</p>
     */
    inline SchemaConfiguration& WithVersionId(Aws::String&& value) { SetVersionId(std::move(value)); return *this;}

    /**
     * <p>Specifies the table version for the output data schema. If you don't specify
     * this version ID, or if you set it to <code>LATEST</code>, Kinesis Data Firehose
     * uses the most recent version. This means that any updates to the table are
     * automatically picked up.</p>
     */
    inline SchemaConfiguration& WithVersionId(const char* value) { SetVersionId(value); return *this;}

  private:

    Aws::String m_roleARN;
    bool m_roleARNHasBeenSet;

    Aws::String m_catalogId;
    bool m_catalogIdHasBeenSet;

    Aws::String m_databaseName;
    bool m_databaseNameHasBeenSet;

    Aws::String m_tableName;
    bool m_tableNameHasBeenSet;

    Aws::String m_region;
    bool m_regionHasBeenSet;

    Aws::String m_versionId;
    bool m_versionIdHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
