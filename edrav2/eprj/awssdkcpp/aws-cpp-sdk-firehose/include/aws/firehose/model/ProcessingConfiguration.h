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
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/firehose/model/Processor.h>
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
   * <p>Describes a data processing configuration.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/firehose-2015-08-04/ProcessingConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_FIREHOSE_API ProcessingConfiguration
  {
  public:
    ProcessingConfiguration();
    ProcessingConfiguration(Aws::Utils::Json::JsonView jsonValue);
    ProcessingConfiguration& operator=(Aws::Utils::Json::JsonView jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;


    /**
     * <p>Enables or disables data processing.</p>
     */
    inline bool GetEnabled() const{ return m_enabled; }

    /**
     * <p>Enables or disables data processing.</p>
     */
    inline void SetEnabled(bool value) { m_enabledHasBeenSet = true; m_enabled = value; }

    /**
     * <p>Enables or disables data processing.</p>
     */
    inline ProcessingConfiguration& WithEnabled(bool value) { SetEnabled(value); return *this;}


    /**
     * <p>The data processors.</p>
     */
    inline const Aws::Vector<Processor>& GetProcessors() const{ return m_processors; }

    /**
     * <p>The data processors.</p>
     */
    inline void SetProcessors(const Aws::Vector<Processor>& value) { m_processorsHasBeenSet = true; m_processors = value; }

    /**
     * <p>The data processors.</p>
     */
    inline void SetProcessors(Aws::Vector<Processor>&& value) { m_processorsHasBeenSet = true; m_processors = std::move(value); }

    /**
     * <p>The data processors.</p>
     */
    inline ProcessingConfiguration& WithProcessors(const Aws::Vector<Processor>& value) { SetProcessors(value); return *this;}

    /**
     * <p>The data processors.</p>
     */
    inline ProcessingConfiguration& WithProcessors(Aws::Vector<Processor>&& value) { SetProcessors(std::move(value)); return *this;}

    /**
     * <p>The data processors.</p>
     */
    inline ProcessingConfiguration& AddProcessors(const Processor& value) { m_processorsHasBeenSet = true; m_processors.push_back(value); return *this; }

    /**
     * <p>The data processors.</p>
     */
    inline ProcessingConfiguration& AddProcessors(Processor&& value) { m_processorsHasBeenSet = true; m_processors.push_back(std::move(value)); return *this; }

  private:

    bool m_enabled;
    bool m_enabledHasBeenSet;

    Aws::Vector<Processor> m_processors;
    bool m_processorsHasBeenSet;
  };

} // namespace Model
} // namespace Firehose
} // namespace Aws
