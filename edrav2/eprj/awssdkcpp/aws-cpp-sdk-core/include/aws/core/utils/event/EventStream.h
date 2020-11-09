/*
 * Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/core/Core_EXPORTS.h>
#include <aws/core/utils/event/EventStreamBuf.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>

namespace Aws
{
    namespace Utils
    {
        namespace Event
        {
            /**
             * It's only used for output stream right now.
             */
            class AWS_CORE_API EventStream : public Aws::IOStream
            {
            public:
                /**
                 * @param decoder decodes the stream from server side, so as to invoke related callback functions.
                 * @param eventStreamBufLength The length of the underlying buffer.
                 */
                EventStream(EventStreamDecoder& decoder, size_t eventStreamBufLength = Aws::Utils::Event::DEFAULT_BUF_SIZE);

                EventStream(const EventStream&) = delete;
                EventStream(EventStream&&) = delete;
                EventStream& operator=(const EventStream&) = delete;
                EventStream& operator=(EventStream&&) = delete;

                virtual ~EventStream() {}

            private:
                EventStreamBuf m_eventStreamBuf;
            };
        }
    }
}