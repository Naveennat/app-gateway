#pragma once

/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Module.h"

namespace JsonData {
namespace AppNotifications {

    // Params for broadcast endpoint
    class BroadcastParamsData : public Core::JSON::Container {
    public:
        BroadcastParamsData()
            : Core::JSON::Container()
        {
            Add(_T("topic"), &Topic);
            Add(_T("payload"), &Payload);
            Add(_T("type"), &Type);
        }

        Core::JSON::String Topic;
        Core::JSON::String Payload;
        Core::JSON::String Type; // optional
    };

    // Generic manage topic params (for allowtopic/disallowtopic)
    class ManageTopicParamsData : public Core::JSON::Container {
    public:
        ManageTopicParamsData()
            : Core::JSON::Container()
        {
            Add(_T("topic"), &Topic);
        }

        Core::JSON::String Topic;
    };

    // Event payload for "notification" event
    class NotificationeventParamsData : public Core::JSON::Container {
    public:
        NotificationeventParamsData()
            : Core::JSON::Container()
        {
            Add(_T("topic"), &Topic);
            Add(_T("payload"), &Payload);
            Add(_T("type"), &Type);
        }

        Core::JSON::String Topic;
        Core::JSON::String Payload;
        Core::JSON::String Type;
    };

    // Config response payload
    class ConfigResultData : public Core::JSON::Container {
    public:
        ConfigResultData()
            : Core::JSON::Container()
        {
            Add(_T("allowAnyTopics"), &AllowAnyTopics);
            Add(_T("maxPayloadBytes"), &MaxPayloadBytes);
            Add(_T("allowedTopics"), &AllowedTopics);
        }

        Core::JSON::Boolean AllowAnyTopics;
        Core::JSON::DecUInt32 MaxPayloadBytes;

        class TopicArray : public Core::JSON::ArrayType<Core::JSON::String> {};
        TopicArray AllowedTopics;
    };

} // namespace AppNotifications
} // namespace JsonData
