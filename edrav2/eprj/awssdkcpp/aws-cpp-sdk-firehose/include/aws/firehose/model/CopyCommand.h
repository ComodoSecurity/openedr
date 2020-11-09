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
   * <p>Describes a <code>COPY</code> command for Amazon Redshift.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/CopyCommand">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API CopyCommand
  {
  public:
    CopyCommand();
    CopyCommand(Aws::Utils::Json::JsonView jsonValue);
    CopyCommand& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>The name of the target table. The table must already exist in the
     * database.</p>
     */
    inline const Aws::String& GetDataTableName() const{ return m_dataTableName; }

    /**
     * <p>The name of the target table. The table must already exist in the
     * database.</p>
     */
    inline void SetDataTableName(const Aws::String& value) { m_dataTableNameHasBeenSet = true; m_dataTableName = value; }

    /**
     * <p>The name of the target table. The table must already exist in the
     * database.</p>
     */
    inline void SetDataTableName(Aws::String&& value) { m_dataTableNameHasBeenSet = true; m_dataTableName = std::move(value); }

    /**
     * <p>The name of the target table. The table must already exist in the
     * database.</p>
     */
    inline void SetDataTableName(const char* value) { m_dataTableNameHasBeenSet = true; m_dataTableName.assign(value); }

    /**
     * <p>The name of the target table. The table must already exist in the
     * database.</p>
     */
    inline CopyCommand& WithDataTableName(const Aws::String& value) { SetDataTableName(value); return *this;}

    /**
     * <p>The name of the target table. The table must already exist in the
     * database.</p>
     */
    inline CopyCommand& WithDataTableName(Aws::String&& value) { SetDataTableName(std::move(value)); return *this;}

    /**
     * <p>The name of the target table. The table must already exist in the
     * database.</p>
     */
    inline CopyCommand& WithDataTableName(const char* value) { SetDataTableName(value); return *this;}


    /**
     * <p>A comma-separated list of column names.</p>
     */
    inline const Aws::String& GetDataTableColumns() const{ return m_dataTableColumns; }

    /**
     * <p>A comma-separated list of column names.</p>
     */
    inline void SetDataTableColumns(const Aws::String& value) { m_dataTableColumnsHasBeenSet = true; m_dataTableColumns = value; }

    /**
     * <p>A comma-separated list of column names.</p>
     */
    inline void SetDataTableColumns(Aws::String&& value) { m_dataTableColumnsHasBeenSet = true; m_dataTableColumns = std::move(value); }

    /**
     * <p>A comma-separated list of column names.</p>
     */
    inline void SetDataTableColumns(const char* value) { m_dataTableColumnsHasBeenSet = true; m_dataTableColumns.assign(value); }

    /**
     * <p>A comma-separated list of column names.</p>
     */
    inline CopyCommand& WithDataTableColumns(const Aws::String& value) { SetDataTableColumns(value); return *this;}

    /**
     * <p>A comma-separated list of column names.</p>
     */
    inline CopyCommand& WithDataTableColumns(Aws::String&& value) { SetDataTableColumns(std::move(value)); return *this;}

    /**
     * <p>A comma-separated list of column names.</p>
     */
    inline CopyCommand& WithDataTableColumns(const char* value) { SetDataTableColumns(value); return *this;}


    /**
     * <p>Optional parameters to use with the Amazon Redshift <code>COPY</code>
     * command. For more information, see the "Optional Parameters" section of <a
     * href="http://docs.aws.amazon.com/redshift/latest/dg/r_COPY.html">Amazon Redshift
     * COPY command</a>. Some possible examples that would apply to Kinesis Data
     * Firehose are as follows:</p> <p> <code>delimiter '\t' lzop;</code> - fields are
     * delimited with "\t" (TAB character) and compressed using lzop.</p> <p>
     * <code>delimiter '|'</code> - fields are delimited with "|" (this is the default
     * delimiter).</p> <p> <code>delimiter '|' escape</code> - the delimiter should be
     * escaped.</p> <p> <code>fixedwidth
     * 'venueid:3,venuename:25,venuecity:12,venuestate:2,venueseats:6'</code> - fields
     * are fixed width in the source, with each width specified after every column in
     * the table.</p> <p> <code>JSON 's3://mybucket/jsonpaths.txt'</code> - data is in
     * JSON format, and the path specified is the format of the data.</p> <p>For more
     * examples, see <a
     * href="http://docs.aws.amazon.com/redshift/latest/dg/r_COPY_command_examples.html">Amazon
     * Redshift COPY command examples</a>.</p>
     */
    inline const Aws::String& GetCopyOptions() const{ return m_copyOptions; }

    /**
     * <p>Optional parameters to use with the Amazon Redshift <code>COPY</code>
     * command. For more information, see the "Optional Parameters" section of <a
     * href="http://docs.aws.amazon.com/redshift/latest/dg/r_COPY.html">Amazon Redshift
     * COPY command</a>. Some possible examples that would apply to Kinesis Data
     * Firehose are as follows:</p> <p> <code>delimiter '\t' lzop;</code> - fields are
     * delimited with "\t" (TAB character) and compressed using lzop.</p> <p>
     * <code>delimiter '|'</code> - fields are delimited with "|" (this is the default
     * delimiter).</p> <p> <code>delimiter '|' escape</code> - the delimiter should be
     * escaped.</p> <p> <code>fixedwidth
     * 'venueid:3,venuename:25,venuecity:12,venuestate:2,venueseats:6'</code> - fields
     * are fixed width in the source, with each width specified after every column in
     * the table.</p> <p> <code>JSON 's3://mybucket/jsonpaths.txt'</code> - data is in
     * JSON format, and the path specified is the format of the data.</p> <p>For more
     * examples, see <a
     * href="http://docs.aws.amazon.com/redshift/latest/dg/r_COPY_command_examples.html">Amazon
     * Redshift COPY command examples</a>.</p>
     */
    inline void SetCopyOptions(const Aws::String& value) { m_copyOptionsHasBeenSet = true; m_copyOptions = value; }

    /**
     * <p>Optional parameters to use with the Amazon Redshift <code>COPY</code>
     * command. For more information, see the "Optional Parameters" section of <a
     * href="http://docs.aws.amazon.com/redshift/latest/dg/r_COPY.html">Amazon Redshift
     * COPY command</a>. Some possible examples that would apply to Kinesis Data
     * Firehose are as follows:</p> <p> <code>delimiter '\t' lzop;</code> - fields are
     * delimited with "\t" (TAB character) and compressed using lzop.</p> <p>
     * <code>delimiter '|'</code> - fields are delimited with "|" (this is the default
     * delimiter).</p> <p> <code>delimiter '|' escape</code> - the delimiter should be
     * escaped.</p> <p> <code>fixedwidth
     * 'venueid:3,venuename:25,venuecity:12,venuestate:2,venueseats:6'</code> - fields
     * are fixed width in the source, with each width specified after every column in
     * the table.</p> <p> <code>JSON 's3://mybucket/jsonpaths.txt'</code> - data is in
     * JSON format, and the path specified is the format of the data.</p> <p>For more
     * examples, see <a
     * href="http://docs.aws.amazon.com/redshift/latest/dg/r_COPY_command_examples.html">Amazon
     * Redshift COPY command examples</a>.</p>
     */
    inline void SetCopyOptions(Aws::String&& value) { m_copyOptionsHasBeenSet = true; m_copyOptions = std::move(value); }

    /**
     * <p>Optional parameters to use with the Amazon Redshift <code>COPY</code>
     * command. For more information, see the "Optional Parameters" section of <a
     * href="http://docs.aws.amazon.com/redshift/latest/dg/r_COPY.html">Amazon Redshift
     * COPY command</a>. Some possible examples that would apply to Kinesis Data
     * Firehose are as follows:</p> <p> <code>delimiter '\t' lzop;</code> - fields are
     * delimited with "\t" (TAB character) and compressed using lzop.</p> <p>
     * <code>delimiter '|'</code> - fields are delimited with "|" (this is the default
     * delimiter).</p> <p> <code>delimiter '|' escape</code> - the delimiter should be
     * escaped.</p> <p> <code>fixedwidth
     * 'venueid:3,venuename:25,venuecity:12,venuestate:2,venueseats:6'</code> - fields
     * are fixed width in the source, with each width specified after every column in
     * the table.</p> <p> <code>JSON 's3://mybucket/jsonpaths.txt'</code> - data is in
     * JSON format, and the path specified is the format of the data.</p> <p>For more
     * examples, see <a
     * href="http://docs.aws.amazon.com/redshift/latest/dg/r_COPY_command_examples.html">Amazon
     * Redshift COPY command examples</a>.</p>
     */
    inline void SetCopyOptions(const char* value) { m_copyOptionsHasBeenSet = true; m_copyOptions.assign(value); }

    /**
     * <p>Optional parameters to use with the Amazon Redshift <code>COPY</code>
     * command. For more information, see the "Optional Parameters" section of <a
     * href="http://docs.aws.amazon.com/redshift/latest/dg/r_COPY.html">Amazon Redshift
     * COPY command</a>. Some possible examples that would apply to Kinesis Data
     * Firehose are as follows:</p> <p> <code>delimiter '\t' lzop;</code> - fields are
     * delimited with "\t" (TAB character) and compressed using lzop.</p> <p>
     * <code>delimiter '|'</code> - fields are delimited with "|" (this is the default
     * delimiter).</p> <p> <code>delimiter '|' escape</code> - the delimiter should be
     * escaped.</p> <p> <code>fixedwidth
     * 'venueid:3,venuename:25,venuecity:12,venuestate:2,venueseats:6'</code> - fields
     * are fixed width in the source, with each width specified after every column in
     * the table.</p> <p> <code>JSON 's3://mybucket/jsonpaths.txt'</code> - data is in
     * JSON format, and the path specified is the format of the data.</p> <p>For more
     * examples, see <a
     * href="http://docs.aws.amazon.com/redshift/latest/dg/r_COPY_command_examples.html">Amazon
     * Redshift COPY command examples</a>.</p>
     */
    inline CopyCommand& WithCopyOptions(const Aws::String& value) { SetCopyOptions(value); return *this;}

    /**
     * <p>Optional parameters to use with the Amazon Redshift <code>COPY</code>
     * command. For more information, see the "Optional Parameters" section of <a
     * href="http://docs.aws.amazon.com/redshift/latest/dg/r_COPY.html">Amazon Redshift
     * COPY command</a>. Some possible examples that would apply to Kinesis Data
     * Firehose are as follows:</p> <p> <code>delimiter '\t' lzop;</code> - fields are
     * delimited with "\t" (TAB character) and compressed using lzop.</p> <p>
     * <code>delimiter '|'</code> - fields are delimited with "|" (this is the default
     * delimiter).</p> <p> <code>delimiter '|' escape</code> - the delimiter should be
     * escaped.</p> <p> <code>fixedwidth
     * 'venueid:3,venuename:25,venuecity:12,venuestate:2,venueseats:6'</code> - fields
     * are fixed width in the source, with each width specified after every column in
     * the table.</p> <p> <code>JSON 's3://mybucket/jsonpaths.txt'</code> - data is in
     * JSON format, and the path specified is the format of the data.</p> <p>For more
     * examples, see <a
     * href="http://docs.aws.amazon.com/redshift/latest/dg/r_COPY_command_examples.html">Amazon
     * Redshift COPY command examples</a>.</p>
     */
    inline CopyCommand& WithCopyOptions(Aws::String&& value) { SetCopyOptions(std::move(value)); return *this;}

    /**
     * <p>Optional parameters to use with the Amazon Redshift <code>COPY</code>
     * command. For more information, see the "Optional Parameters" section of <a
     * href="http://docs.aws.amazon.com/redshift/latest/dg/r_COPY.html">Amazon Redshift
     * COPY command</a>. Some possible examples that would apply to Kinesis Data
     * Firehose are as follows:</p> <p> <code>delimiter '\t' lzop;</code> - fields are
     * delimited with "\t" (TAB character) and compressed using lzop.</p> <p>
     * <code>delimiter '|'</code> - fields are delimited with "|" (this is the default
     * delimiter).</p> <p> <code>delimiter '|' escape</code> - the delimiter should be
     * escaped.</p> <p> <code>fixedwidth
     * 'venueid:3,venuename:25,venuecity:12,venuestate:2,venueseats:6'</code> - fields
     * are fixed width in the source, with each width specified after every column in
     * the table.</p> <p> <code>JSON 's3://mybucket/jsonpaths.txt'</code> - data is in
     * JSON format, and the path specified is the format of the data.</p> <p>For more
     * examples, see <a
     * href="http://docs.aws.amazon.com/redshift/latest/dg/r_COPY_command_examples.html">Amazon
     * Redshift COPY command examples</a>.</p>
     */
    inline CopyCommand& WithCopyOptions(const char* value) { SetCopyOptions(value); return *this;}

  private:

    Aws::String m_dataTableName;
    bool m_dataTableNameHasBeenSet;

    Aws::String m_dataTableColumns;
    bool m_dataTableColumnsHasBeenSet;

    Aws::String m_copyOptions;
    bool m_copyOptionsHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
