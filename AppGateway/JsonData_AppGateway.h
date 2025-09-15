#pragma once

/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
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
namespace AppGateway {

// Core request/response models used by AppGateway JSON-RPC

// Launch endpoint: launch an application with optional arguments
class LaunchParamsData : public Core::JSON::Container {
public:
    LaunchParamsData()
        : Core::JSON::Container()
    {
        Add(_T("id"), &Id);
        Add(_T("args"), &Args);
    }

    // Application identifier
    Core::JSON::String Id;
    // Optional opaque arguments for launch, free-form JSON as string
    Core::JSON::String Args;
};

// Simple application id parameter model
class AppIdParamsData : public Core::JSON::Container {
public:
    AppIdParamsData()
        : Core::JSON::Container()
    {
        Add(_T("id"), &Id);
    }

    Core::JSON::String Id;
};

// Send message model for app-2-app messages
class SendmessageParamsData : public Core::JSON::Container {
public:
    SendmessageParamsData()
        : Core::JSON::Container()
    {
        Add(_T("originId"), &OriginId);
        Add(_T("targetId"), &TargetId);
        Add(_T("payload"), &Payload);
        Add(_T("type"), &Type);
    }

    // Originating application id
    Core::JSON::String OriginId;
    // Target application id
    Core::JSON::String TargetId;
    // Payload - opaque JSON/String payload
    Core::JSON::String Payload;
    // Optional message type for routing or filtering
    Core::JSON::String Type;
};

// Broadcast model for app notifications
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
    Core::JSON::String Type;
};

// State response model
class StateResultData : public Core::JSON::Container {
public:
    StateResultData()
        : Core::JSON::Container()
    {
        Add(_T("state"), &State);
    }

    Core::JSON::String State;
};

// Config subset response model
class ConfigResultData : public Core::JSON::Container {
public:
    ConfigResultData()
        : Core::JSON::Container()
    {
        Add(_T("launchDelegateCallsign"), &LaunchDelegateCallsign);
        Add(_T("app2appProviderCallsign"), &App2AppProviderCallsign);
        Add(_T("appNotificationsCallsign"), &AppNotificationsCallsign);
        Add(_T("subscribeAppEvents"), &SubscribeAppEvents);
        Add(_T("allowedApps"), &AllowedApps);
    }

    Core::JSON::String LaunchDelegateCallsign;
    Core::JSON::String App2AppProviderCallsign;
    Core::JSON::String AppNotificationsCallsign;
    Core::JSON::Boolean SubscribeAppEvents;

    class AppArray : public Core::JSON::ArrayType<Core::JSON::String> {};
    AppArray AllowedApps;
};

// Event payloads
class AppstatechangedParamsData : public Core::JSON::Container {
public:
    AppstatechangedParamsData()
        : Core::JSON::Container()
    {
        Add(_T("id"), &Id);
        Add(_T("state"), &State);
        Add(_T("reason"), &Reason);
    }
    Core::JSON::String Id;
    Core::JSON::String State;
    Core::JSON::String Reason;
};

class AppmessageParamsData : public Core::JSON::Container {
public:
    AppmessageParamsData()
        : Core::JSON::Container()
    {
        Add(_T("originId"), &OriginId);
        Add(_T("targetId"), &TargetId);
        Add(_T("payload"), &Payload);
        Add(_T("type"), &Type);
    }
    Core::JSON::String OriginId;
    Core::JSON::String TargetId;
    Core::JSON::String Payload;
    Core::JSON::String Type;
};

class AppstartedParamsData : public Core::JSON::Container {
public:
    AppstartedParamsData()
        : Core::JSON::Container()
    {
        Add(_T("id"), &Id);
    }
    Core::JSON::String Id;
};

class AppstoppedParamsData : public Core::JSON::Container {
public:
    AppstoppedParamsData()
        : Core::JSON::Container()
    {
        Add(_T("id"), &Id);
        Add(_T("reason"), &Reason);
    }
    Core::JSON::String Id;
    Core::JSON::String Reason;
};

} // namespace AppGateway
} // namespace JsonData
