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
namespace LaunchDelegate {

    // Parameters for launch endpoint
    class LaunchParamsData : public Core::JSON::Container {
    public:
        LaunchParamsData()
            : Core::JSON::Container()
        {
            Add(_T("id"), &Id);
            Add(_T("args"), &Args);
        }
        Core::JSON::String Id;
        Core::JSON::String Args; // optional, opaque free-form string
    };

    // Simple app id param for stop/suspend/resume/state
    class AppIdParamsData : public Core::JSON::Container {
    public:
        AppIdParamsData()
            : Core::JSON::Container()
        {
            Add(_T("id"), &Id);
        }
        Core::JSON::String Id;
    };

    // State result payload
    class StateResultData : public Core::JSON::Container {
    public:
        StateResultData()
            : Core::JSON::Container()
        {
            Add(_T("state"), &State);
        }
        Core::JSON::String State;
    };

    // Event payloads

    class StatechangeeventParamsData : public Core::JSON::Container {
    public:
        StatechangeeventParamsData()
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

    class LaunchedeventParamsData : public Core::JSON::Container {
    public:
        LaunchedeventParamsData()
            : Core::JSON::Container()
        {
            Add(_T("id"), &Id);
        }
        Core::JSON::String Id;
    };

    class StoppedeventParamsData : public Core::JSON::Container {
    public:
        StoppedeventParamsData()
            : Core::JSON::Container()
        {
            Add(_T("id"), &Id);
            Add(_T("reason"), &Reason);
        }
        Core::JSON::String Id;
        Core::JSON::String Reason;
    };

    class SuspendedeventParamsData : public Core::JSON::Container {
    public:
        SuspendedeventParamsData()
            : Core::JSON::Container()
        {
            Add(_T("id"), &Id);
        }
        Core::JSON::String Id;
    };

    class ResumedeventParamsData : public Core::JSON::Container {
    public:
        ResumedeventParamsData()
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
            Add(_T("allowAnyApps"), &AllowAnyApps);
            Add(_T("allowedApps"), &AllowedApps);
        }

        Core::JSON::Boolean AllowAnyApps;

        class AppArray : public Core::JSON::ArrayType<Core::JSON::String> {};
        AppArray AllowedApps;
    };

} // namespace LaunchDelegate
} // namespace JsonData
