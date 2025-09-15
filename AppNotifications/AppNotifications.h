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
#include "JsonData_AppNotifications.h"

#include <set>

namespace WPEFramework {
namespace Plugin {

    // AppNotifications plugin: broadcast notifications to subscribers via a "notification" event.
    // Guards on topic allowlist and payload size are configurable.
    class AppNotifications : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    private:
        AppNotifications(const AppNotifications&) = delete;
        AppNotifications& operator=(const AppNotifications&) = delete;

        // Runtime configuration model
        class Config : public Core::JSON::Container {
        public:
            Config()
                : Core::JSON::Container()
            {
                Add(_T("allowAnyTopics"), &AllowAnyTopics);
                Add(_T("maxPayloadBytes"), &MaxPayloadBytes);
                Add(_T("allowedTopics"), &AllowedTopics);

                // Defaults
                AllowAnyTopics = true;
                MaxPayloadBytes = 262144; // 256 KiB
            }

            Core::JSON::Boolean AllowAnyTopics;
            Core::JSON::DecUInt32 MaxPayloadBytes;

            class TopicArray : public Core::JSON::ArrayType<Core::JSON::String> {};
            TopicArray AllowedTopics;
        };

    public:
        // PUBLIC_INTERFACE
        // Construct the plugin. Endpoints are registered during Initialize.
        AppNotifications();

        // PUBLIC_INTERFACE
        // Destructor ensures clean deinitialization.
        ~AppNotifications() override;

        BEGIN_INTERFACE_MAP(AppNotifications)
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
        bool IsAllowedTopic(const string& topic) const;
        bool ValidatePayloadSize(const string& payload) const;

        // Event emitter
        void EmitNotification(const string& topic, const string& payload, const string& type);

        // JSON-RPC endpoints

        // PUBLIC_INTERFACE
        // Broadcast a notification. Emits "notification" event.
        uint32_t endpoint_broadcast(const JsonData::AppNotifications::BroadcastParamsData& params);

        // PUBLIC_INTERFACE
        // Allow a topic at runtime (adds to allowlist).
        uint32_t endpoint_allowtopic(const JsonData::AppNotifications::ManageTopicParamsData& params);

        // PUBLIC_INTERFACE
        // Disallow a topic at runtime (removes from allowlist).
        uint32_t endpoint_disallowtopic(const JsonData::AppNotifications::ManageTopicParamsData& params);

        // PUBLIC_INTERFACE
        // List allowed topics (current allowlist snapshot).
        uint32_t endpoint_listtopics(Core::JSON::ArrayType<Core::JSON::String>& response) const;

        // PUBLIC_INTERFACE
        // Get active configuration.
        uint32_t endpoint_getconfig(JsonData::AppNotifications::ConfigResultData& response) const;

    private:
        PluginHost::IShell* _service;
        Config _config;
        mutable Core::CriticalSection _adminLock;

        // Cached allowlist (set for quick lookup)
        std::set<string> _allowedTopics;

        // Optional token for security-enabled deployments
        string _securityToken;
    };

} // namespace Plugin
} // namespace WPEFramework
