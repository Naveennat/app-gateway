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
#include "JsonData_App2AppProvider.h"

#include <set>
#include <algorithm>

namespace WPEFramework {
namespace Plugin {

    // App2AppProvider plugin: provides inter-application messaging primitive over JSON-RPC.
    // Exposes a sendmessage() endpoint and emits "message" events to all subscribers.
    // Optional allowlist and registration policies can be configured.
    class App2AppProvider : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    private:
        App2AppProvider(const App2AppProvider&) = delete;
        App2AppProvider& operator=(const App2AppProvider&) = delete;

        // Configuration model for the plugin
        class Config : public Core::JSON::Container {
        public:
            Config()
                : Core::JSON::Container()
            {
                Add(_T("allowAny"), &AllowAny);
                Add(_T("requireRegister"), &RequireRegister);
                Add(_T("maxPayloadBytes"), &MaxPayloadBytes);
                Add(_T("allowedApps"), &AllowedApps);

                // Defaults
                AllowAny = true;
                RequireRegister = false;
                MaxPayloadBytes = 262144; // 256 KiB
            }

            Core::JSON::Boolean AllowAny;
            Core::JSON::Boolean RequireRegister;
            Core::JSON::DecUInt32 MaxPayloadBytes;

            class AppArray : public Core::JSON::ArrayType<Core::JSON::String> {};
            AppArray AllowedApps;
        };

    public:
        // PUBLIC_INTERFACE
        // Construct the plugin. Endpoints are registered during Initialize.
        App2AppProvider();

        // PUBLIC_INTERFACE
        // Destructor ensures clean deinitialization.
        ~App2AppProvider() override;

        BEGIN_INTERFACE_MAP(App2AppProvider)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

        // PUBLIC_INTERFACE
        // Initializes the plugin, parses configuration and registers JSON-RPC endpoints.
        const string Initialize(PluginHost::IShell* service) override;

        // PUBLIC_INTERFACE
        // Deinitializes, unregisters endpoints and clears runtime state.
        void Deinitialize(PluginHost::IShell* service) override;

        // PUBLIC_INTERFACE
        // Returns informational string about the plugin.
        string Information() const override;

    private:
        // JSON-RPC endpoint registration
        void RegisterAll();
        void UnregisterAll();

        // Policy helpers
        bool IsAllowedOrigin(const string& originId) const;
        bool IsRegistered(const string& appId) const;
        bool ValidatePayloadSize(const string& payload) const;

        // Event emitters
        void EmitMessage(const string& originId,
                         const string& targetId,
                         const string& payload,
                         const string& type);

        // JSON-RPC endpoints

        // PUBLIC_INTERFACE
        // Send an inter-application message. If policy permits, emits a "message" event.
        uint32_t endpoint_sendmessage(const JsonData::App2AppProvider::SendmessageParamsData& params);

        // PUBLIC_INTERFACE
        // Register an application id for permission checks (if RequireRegister = true).
        uint32_t endpoint_registerapp(const JsonData::App2AppProvider::RegisterappParamsData& params);

        // PUBLIC_INTERFACE
        // Unregister a previously registered application id.
        uint32_t endpoint_unregisterapp(const JsonData::App2AppProvider::UnregisterappParamsData& params);

        // PUBLIC_INTERFACE
        // List registered application identifiers.
        uint32_t endpoint_listregistered(Core::JSON::ArrayType<Core::JSON::String>& response) const;

        // PUBLIC_INTERFACE
        // Get the active configuration and allowlist.
        uint32_t endpoint_getconfig(JsonData::App2AppProvider::ConfigResultData& response) const;

    private:
        PluginHost::IShell* _service;
        Config _config;
        mutable Core::CriticalSection _adminLock;

        // Runtime registration store (ids explicitly registered during runtime)
        std::set<string> _registered;

        // Cached allowlist (set for quick lookup)
        std::set<string> _allowlist;

        // Optional token for security-enabled deployments
        string _securityToken;
    };

} // namespace Plugin
} // namespace WPEFramework
