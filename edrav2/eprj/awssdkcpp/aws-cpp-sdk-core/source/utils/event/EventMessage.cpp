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

#include <aws/core/utils/event/EventMessage.h>
#include <aws/core/utils/HashingUtils.h>

namespace Aws
{
    namespace Utils
    {
        namespace Event
        {
            const char EVENT_TYPE_HEADER[] = ":event-type";
            const char CONTENT_TYPE_HEADER[] = ":content-type";
            const char MESSAGE_TYPE_HEADER[] = ":message-type";
            const char ERROR_CODE_HEADER[] = ":error-code";
            const char ERROR_MESSAGE_HEADER[] = ":error-message";

            static const int EVENT_HASH = HashingUtils::HashString("event");
            static const int ERROR_HASH = HashingUtils::HashString("error");

            Message::MessageType Message::GetMessageTypeForName(const Aws::String& name)
            {
                int hashCode = Aws::Utils::HashingUtils::HashString(name.c_str());
                if (hashCode == EVENT_HASH)
                {
                    return MessageType::EVENT;
                }
                else if (hashCode == ERROR_HASH)
                {
                    return MessageType::REQUEST_LEVEL_ERROR;
                }
                else
                {
                    return MessageType::UNKNOWN;
                }
            }

            Aws::String Message::GetNameForMessageType(MessageType value)
            {
                switch (value)
                {
                case MessageType::EVENT:
                    return "event";
                case MessageType::REQUEST_LEVEL_ERROR:
                    return "error";
                default:
                    return "unknown";
                }
            }

            void Message::Reset()
            {
                m_totalLength = 0;
                m_headersLength = 0;
                m_payloadLength = 0;

                m_eventHeaders.clear();
                m_eventPayload.clear();
            }

        } // namespace Event
    } // namespace Utils
} // namespace Aws

