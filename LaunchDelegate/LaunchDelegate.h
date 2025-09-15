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
#include "JsonData_LaunchDelegate.h"

#include <map>
#include <set>

namespace WPEFramework {
namespace Plugin {

    // LaunchDelegate plugin: manages application lifecycle operations.
    // Exposes launch/stop/suspend/resume/state endpoints and emits state change events.
    class LaunchDelegate : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    private:
        LaunchDelegate(const LaunchDelegate&) = delete;
        LaunchDelegate& operator=(const LaunchDelegate&) = delete;

        // Runtime configuration model
        class Config : public Core::JSON::Container {
        public:
            Config()
                : Core::JSON::Container()
            {
                Add(_T("allowAnyApps"), &AllowAnyApps);
                Add(_T("allowedApps"), &AllowedApps);

                // Defaults
                AllowAnyApps = true;
            }

            Core::JSON::Boolean AllowAnyApps;

            class AppArray : public Core::JSON::ArrayType<Core::JSON::String> {};
            AppArray AllowedApps;
        };

        // Internal state per application
        struct AppRuntime {
            string State;     // "stopped", "running", "suspended", etc.
            string LastArgs;  // last launch args (opaque)
            string Reason;    // last reason for transition
            AppRuntime() : State(_T("stopped")), LastArgs(), Reason() {}
        };

    public:
        // PUBLIC_INTERFACE
        // Constructor: no endpoints registered yet.
        LaunchDelegate();

        // PUBLIC_INTERFACE
        // Destructor ensures deinitialization.
        ~LaunchDelegate() override;

        BEGIN_INTERFACE_MAP(LaunchDelegate)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

        // PUBLIC_INTERFACE
        // Initializes the plugin, parses configuration and registers JSON-RPC endpoints.
        const string Initialize(PluginHost::IShell* service) override;

        // PUBLIC_INTERFACE
        // Deinitializes the plugin and unregisters JSON-RPC endpoints.
        void Deinitialize(PluginHost::IShell* service) override;

        // PUBLIC_INTERFACE
        // Returns informational string about the plugin.
        string Information() const override;

    private:
        // JSON-RPC endpoint registration
        void RegisterAll();
        void UnregisterAll();

        // Helpers
        bool IsAllowedApp(const string& appId) const;
        void EnsureAppEntry(const string& appId);
        void SetStateAndEmit(const string& appId, const string& newState, const string& reason);

        // Event emitters
        void EmitStateChanged(const string& appId, const string& state, const string& reason);
        void EmitLaunched(const string& appId);
        void EmitStopped(const string& appId, const string& reason);
        void EmitSuspended(const string& appId);
        void EmitResumed(const string& appId);

        // JSON-RPC endpoints

        // PUBLIC_INTERFACE
        // Launch an application. Transitions state to "running".
        uint32_t endpoint_launch(const JsonData::LaunchDelegate::LaunchParamsData& params);

        // PUBLIC_INTERFACE
        // Stop an application. Transitions state to "stopped".
        uint32_t endpoint_stop(const JsonData::LaunchDelegate::AppIdParamsData& params);

        // PUBLIC_INTERFACE
        // Suspend an application. Transitions state to "suspended".
        uint32_t endpoint_suspend(const JsonData::LaunchDelegate::AppIdParamsData& params);

        // PUBLIC_INTERFACE
        // Resume a suspended application. Transitions state to "running".
        uint32_t endpoint_resume(const JsonData::LaunchDelegate::AppIdParamsData& params);

        // PUBLIC_INTERFACE
        // Get the current state of the application.
        uint32_t endpoint_state(const JsonData::LaunchDelegate::AppIdParamsData& params,
                                JsonData::LaunchDelegate::StateResultData& response) const;

        // PUBLIC_INTERFACE
        // Get the active configuration allowlist.
        uint32_t endpoint_getconfig(JsonData::LaunchDelegate::ConfigResultData& response) const;

        // PUBLIC_INTERFACE
        // List configured/allowed applications (configuration snapshot).
        uint32_t endpoint_listapps(Core::JSON::ArrayType<Core::JSON::String>& response) const;

    private:
        PluginHost::IShell* _service;
        Config _config;
        mutable Core::CriticalSection _adminLock;

        // Cache of allowed apps for quick lookup
        std::set<string> _allowlist;

        // Tracked runtime state
        std::map<string, AppRuntime> _apps;

        // Optional token for security-enabled deployments
        string _securityToken;
    };

} // namespace Plugin
} // namespace WPEFramework
