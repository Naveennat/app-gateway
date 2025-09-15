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
#include "JsonData_AppGateway.h"

namespace WPEFramework {
namespace Plugin {

    // AppGateway plugin bridges LaunchDelegate, App2AppProvider, and AppNotifications
    // through a single JSON-RPC namespace and forwards events for unified clients integration.
    class AppGateway : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    private:
        AppGateway(const AppGateway&) = delete;
        AppGateway& operator=(const AppGateway&) = delete;

        // Forward events received from other plugins through IDispatcher subscriptions
        class RemoteEventForwarder : public PluginHost::IDispatcher::ICallback {
        private:
            RemoteEventForwarder() = delete;
            RemoteEventForwarder(const RemoteEventForwarder&) = delete;
            RemoteEventForwarder& operator=(const RemoteEventForwarder&) = delete;

        public:
            explicit RemoteEventForwarder(AppGateway& parent, const string& sourceTag, const string& purpose)
                : _parent(parent)
                , _source(sourceTag)
                , _purpose(purpose)
            {
            }
            ~RemoteEventForwarder() override = default;

            BEGIN_INTERFACE_MAP(RemoteEventForwarder)
            INTERFACE_ENTRY(PluginHost::IDispatcher::ICallback)
            END_INTERFACE_MAP

            // PUBLIC_INTERFACE
            // Receives events from remote IDispatcher subscriptions and re-emits a normalized event on AppGateway.
            Core::hresult Event(const string& event, const string& designator, const string& parameters /* @restrict:(4M-1) */) override;

        private:
            AppGateway& _parent;
            const string _source;   // callsign we subscribed to
            const string _purpose;  // "launch", "app2app", "notifications"
        };

        // Parsed configuration for AppGateway
        class Config : public Core::JSON::Container {
        public:
            Config()
                : Core::JSON::Container()
            {
                Add(_T("launchDelegateCallsign"), &LaunchDelegateCallsign);
                Add(_T("app2appProviderCallsign"), &App2AppProviderCallsign);
                Add(_T("appNotificationsCallsign"), &AppNotificationsCallsign);
                Add(_T("subscribeAppEvents"), &SubscribeAppEvents);

                Add(_T("startMethod"), &StartMethod);
                Add(_T("stopMethod"), &StopMethod);
                Add(_T("suspendMethod"), &SuspendMethod);
                Add(_T("resumeMethod"), &ResumeMethod);
                Add(_T("stateMethod"), &StateMethod);
                Add(_T("sendMessageMethod"), &SendMessageMethod);
                Add(_T("broadcastMethod"), &BroadcastMethod);

                Add(_T("allowedApps"), &AllowedApps);

                // Defaults
                SubscribeAppEvents = true;

                StartMethod = _T("launch");
                StopMethod = _T("stop");
                SuspendMethod = _T("suspend");
                ResumeMethod = _T("resume");
                StateMethod = _T("state");
                SendMessageMethod = _T("sendmessage");
                BroadcastMethod = _T("broadcast");
            }

            Core::JSON::String LaunchDelegateCallsign;
            Core::JSON::String App2AppProviderCallsign;
            Core::JSON::String AppNotificationsCallsign;
            Core::JSON::Boolean SubscribeAppEvents;

            Core::JSON::String StartMethod;
            Core::JSON::String StopMethod;
            Core::JSON::String SuspendMethod;
            Core::JSON::String ResumeMethod;
            Core::JSON::String StateMethod;

            Core::JSON::String SendMessageMethod;
            Core::JSON::String BroadcastMethod;

            class AppArray : public Core::JSON::ArrayType<Core::JSON::String> {};
            AppArray AllowedApps;
        };

    public:
        // PUBLIC_INTERFACE
        // Constructor: sets up JSONRPC base without registering handlers. Handlers are registered in Initialize.
        AppGateway();

        // PUBLIC_INTERFACE
        // Destructor: ensures deinitialization
        ~AppGateway() override;

        BEGIN_INTERFACE_MAP(AppGateway)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

        // Plugin lifecycle
        // PUBLIC_INTERFACE
        // Initializes the plugin, parses configuration, connects to dependent plugins, and registers JSON-RPC endpoints.
        const string Initialize(PluginHost::IShell* service) override;

        // PUBLIC_INTERFACE
        // Deinitializes the plugin, unsubscribes events, releases remote handles, and unregisters JSON-RPC endpoints.
        void Deinitialize(PluginHost::IShell* service) override;

        // PUBLIC_INTERFACE
        // Provides informational string about the plugin.
        string Information() const override;

    private:
        // JSON-RPC endpoint registration
        void RegisterAll();
        void UnregisterAll();

        // Helpers for remote invoke/subscribe
        uint32_t EnsureRemoteDispatchers();
        uint32_t SubscribeRemoteEvents();
        void UnsubscribeRemoteEvents();
        uint32_t InvokeRemote(PluginHost::IDispatcher* dispatcher,
                              const string& method,
                              const string& parameters,
                              string& response) const;

        // Normalized event emitters
        void EmitAppStateChanged(const string& appId, const string& state, const string& reason);
        void EmitAppMessage(const string& origin, const string& target, const string& payload, const string& type);
        void EmitAppStarted(const string& appId);
        void EmitAppStopped(const string& appId, const string& reason);

    private:
        // JSON-RPC endpoints

        // PUBLIC_INTERFACE
        // Launch an application via LaunchDelegate (configured method).
        uint32_t endpoint_launch(const JsonData::AppGateway::LaunchParamsData& params);

        // PUBLIC_INTERFACE
        // Stop an application via LaunchDelegate (configured method).
        uint32_t endpoint_stop(const JsonData::AppGateway::AppIdParamsData& params);

        // PUBLIC_INTERFACE
        // Suspend an application via LaunchDelegate (configured method).
        uint32_t endpoint_suspend(const JsonData::AppGateway::AppIdParamsData& params);

        // PUBLIC_INTERFACE
        // Resume an application via LaunchDelegate (configured method).
        uint32_t endpoint_resume(const JsonData::AppGateway::AppIdParamsData& params);

        // PUBLIC_INTERFACE
        // Get the state of an application from LaunchDelegate (configured method).
        uint32_t endpoint_state(const JsonData::AppGateway::AppIdParamsData& params, JsonData::AppGateway::StateResultData& response);

        // PUBLIC_INTERFACE
        // Send a message from one app to another using App2AppProvider.
        uint32_t endpoint_sendmessage(const JsonData::AppGateway::SendmessageParamsData& params);

        // PUBLIC_INTERFACE
        // Broadcast a notification using AppNotifications provider.
        uint32_t endpoint_broadcast(const JsonData::AppGateway::BroadcastParamsData& params);

        // PUBLIC_INTERFACE
        // Get a subset of the AppGateway configuration and allowed apps.
        uint32_t endpoint_getconfig(JsonData::AppGateway::ConfigResultData& response) const;

        // PUBLIC_INTERFACE
        // Get allowed apps list configured in AppGateway.
        uint32_t endpoint_listapps(Core::JSON::ArrayType<Core::JSON::String>& response) const;

    private:
        PluginHost::IShell* _service;
        Config _config;

        // Remote JSON-RPC dispatchers
        PluginHost::IDispatcher* _launchDispatcher;
        PluginHost::IDispatcher* _a2aDispatcher;
        PluginHost::IDispatcher* _notifDispatcher;

        // Remote event sinks
        Core::Sink<RemoteEventForwarder> _launchEventsSink;
        Core::Sink<RemoteEventForwarder> _a2aEventsSink;

        // Subscription state flags
        bool _launchEventsSubscribed;
        bool _a2aEventsSubscribed;

        // Token used for cross-plugin security (if available). Empty by default.
        string _securityToken;
    };

} // namespace Plugin
} // namespace WPEFramework
