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
namespace App2AppProvider {

    // Parameters for sending inter-app messages
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

        Core::JSON::String OriginId;
        Core::JSON::String TargetId;
        Core::JSON::String Payload;
        Core::JSON::String Type; // optional
    };

    // Event payload for "message" notifications
    class MessageeventParamsData : public Core::JSON::Container {
    public:
        MessageeventParamsData()
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

    // Register application parameters
    class RegisterappParamsData : public Core::JSON::Container {
    public:
        RegisterappParamsData()
            : Core::JSON::Container()
        {
            Add(_T("id"), &Id);
        }
        Core::JSON::String Id;
    };

    // Unregister application parameters
    class UnregisterappParamsData : public Core::JSON::Container {
    public:
        UnregisterappParamsData()
            : Core::JSON::Container()
        {
            Add(_T("id"), &Id);
        }
        Core::JSON::String Id;
    };

    // Config response payload
    class ConfigResultData : public Core::JSON::Container {
    public:
        ConfigResultData()
            : Core::JSON::Container()
        {
            Add(_T("allowAny"), &AllowAny);
            Add(_T("requireRegister"), &RequireRegister);
            Add(_T("maxPayloadBytes"), &MaxPayloadBytes);
            Add(_T("allowedApps"), &AllowedApps);
        }

        Core::JSON::Boolean AllowAny;
        Core::JSON::Boolean RequireRegister;
        Core::JSON::DecUInt32 MaxPayloadBytes;

        class AppArray : public Core::JSON::ArrayType<Core::JSON::String> {};
        AppArray AllowedApps;
    };

} // namespace App2AppProvider
} // namespace JsonData
