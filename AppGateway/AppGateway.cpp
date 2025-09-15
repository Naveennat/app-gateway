#include "AppGateway.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework {

    namespace {
        // Plugin metadata per Thunder best practice
        static Plugin::Metadata<Plugin::AppGateway> metadata(
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
            {}, // Preconditions
            {}, // Terminations
            {}  // Controls
        );
    }

namespace Plugin {

    using namespace JsonData::AppGateway;

    // Register the plugin in the Thunder/WPEFramework runtime
    SERVICE_REGISTRATION(AppGateway, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

    // =============================
    // RemoteEventForwarder
    // =============================

    Core::hresult AppGateway::RemoteEventForwarder::Event(const string& event, const string& designator, const string& parameters) {
        // Normalize and forward events based on their source purpose
        // purpose: "launch" => state changes and lifecycle events, "app2app" => message relay
        if (_purpose == _T("launch")) {
            // Expected remote events could be "statechange", "launched", "stopped", "suspended", "resumed"
            if (event == _T("statechange") || event == _T("statechanged") || event == _T("appstatechange")) {
                // parameters expected: {"id":"<appId>","state":"<state>","reason":"<reason>"}
                Core::JSON::Object json;
                json.FromString(parameters);

                Core::JSON::String id;
                Core::JSON::String state;
                Core::JSON::String reason;
                id = json.Get(_T("id")).Value();
                state = json.Get(_T("state")).Value();
                reason = json.Get(_T("reason")).Value();

                _parent.EmitAppStateChanged(id.Value(), state.Value(), reason.Value());
            } else if (event == _T("launched") || event == _T("started")) {
                Core::JSON::Object json;
                json.FromString(parameters);
                Core::JSON::String id;
                id = json.Get(_T("id")).Value();
                _parent.EmitAppStarted(id.Value());
            } else if (event == _T("stopped")) {
                Core::JSON::Object json;
                json.FromString(parameters);
                Core::JSON::String id;
                Core::JSON::String reason;
                id = json.Get(_T("id")).Value();
                reason = json.Get(_T("reason")).Value();
                _parent.EmitAppStopped(id.Value(), reason.Value());
            } else {
                // Unknown event: forward as appstatechanged passthrough
                _parent.EmitAppStateChanged(_source, event, parameters);
            }
        } else if (_purpose == _T("app2app")) {
            // Expected remote event "message"
            if (event == _T("message")) {
                // parameters expected: {"originId":"...","targetId":"...","payload":"...","type":"..."}
                Core::JSON::Object json;
                json.FromString(parameters);
                Core::JSON::String origin;
                Core::JSON::String target;
                Core::JSON::String payload;
                Core::JSON::String type;

                origin = json.Get(_T("originId")).Value();
                target = json.Get(_T("targetId")).Value();
                payload = json.Get(_T("payload")).Value();
                type = json.Get(_T("type")).Value();
                _parent.EmitAppMessage(origin.Value(), target.Value(), payload.Value(), type.Value());
            } else {
                // Unknown event: ignore or forward as generic message
                _parent.EmitAppMessage(_source, designator, parameters, _T("raw"));
            }
        }

        return Core::ERROR_NONE;
    }

    // =============================
    // AppGateway
    // =============================

    AppGateway::AppGateway()
        : PluginHost::JSONRPC()
        , _service(nullptr)
        , _launchDispatcher(nullptr)
        , _a2aDispatcher(nullptr)
        , _notifDispatcher(nullptr)
        , _launchEventsSink(*this, _T("LaunchDelegate"), _T("launch"))
        , _a2aEventsSink(*this, _T("App2AppProvider"), _T("app2app"))
        , _launchEventsSubscribed(false)
        , _a2aEventsSubscribed(false)
    {
    }

    AppGateway::~AppGateway() {
        // Make sure all is cleaned up if Deinitialize was not called
        if (_service != nullptr) {
            Deinitialize(_service);
        }
    }

    const string AppGateway::Initialize(PluginHost::IShell* service) {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);

        _service = service;
        _service->AddRef();

        // Parse configuration
        _config.FromString(_service->ConfigLine());

        // Load token if any (optional). This is a placeholder; in a security-enabled deployment, retrieve a token.
        _securityToken = Core::SystemInfo::GetEnvironment(_T("THUNDER_SECURITY_TOKEN"), _T(""));

        // Register JSON-RPC methods
        RegisterAll();

        // Attach JSONRPC to service for event routing
        PluginHost::IShell::IConnectionServer::INotification* sink = nullptr;
        auto resultAttach = Attach(sink, _service);
        if (resultAttach != Core::ERROR_NONE) {
            SYSLOG(Logging::Startup, (_T("AppGateway: Failed to attach JSONRPC handler to service (code %u)"), resultAttach));
        }

        // Resolve remote dispatchers and subscribe for remote events if configured
        const uint32_t res = EnsureRemoteDispatchers();
        if (res != Core::ERROR_NONE) {
            SYSLOG(Logging::Startup, (_T("AppGateway: Remote plugins are not fully available (code %u). Some endpoints may fail."), res));
        }

        if (_config.SubscribeAppEvents.Value() == true) {
            SubscribeRemoteEvents();
        }

        return string(); // empty = success
    }

    void AppGateway::Deinitialize(PluginHost::IShell* service) {
        ASSERT(service == _service);

        // Unsubscribe remote events
        UnsubscribeRemoteEvents();

        // Detach JSONRPC
        PluginHost::IShell::IConnectionServer::INotification* sink = nullptr;
        Detach(sink);

        // Unregister endpoints
        UnregisterAll();

        // Release remote dispatchers
        if (_launchDispatcher != nullptr) {
            _launchDispatcher->Release();
            _launchDispatcher = nullptr;
        }
        if (_a2aDispatcher != nullptr) {
            _a2aDispatcher->Release();
            _a2aDispatcher = nullptr;
        }
        if (_notifDispatcher != nullptr) {
            _notifDispatcher->Release();
            _notifDispatcher = nullptr;
        }

        if (_service != nullptr) {
            _service->Release();
            _service = nullptr;
        }
    }

    string AppGateway::Information() const {
        // Return basic info
        Core::JSON::Object info;
        info.Set(_T("name"), Core::JSON::String(_T("AppGateway")));
        info.Set(_T("version"), Core::JSON::String(Core::NumberType<uint32_t>(API_VERSION_NUMBER_MAJOR).Text() + _T(".") +
                                                  Core::NumberType<uint32_t>(API_VERSION_NUMBER_MINOR).Text() + _T(".") +
                                                  Core::NumberType<uint32_t>(API_VERSION_NUMBER_PATCH).Text()));
        info.Set(_T("description"), Core::JSON::String(_T("Unified application gateway bridging LaunchDelegate, App2AppProvider and AppNotifications")) );
        return info.ToString();
    }

    void AppGateway::RegisterAll() {
        // JSON-RPC endpoints
        Register<LaunchParamsData, void>(_T("launch"), &AppGateway::endpoint_launch, this);
        Register<AppIdParamsData, void>(_T("stop"), &AppGateway::endpoint_stop, this);
        Register<AppIdParamsData, void>(_T("suspend"), &AppGateway::endpoint_suspend, this);
        Register<AppIdParamsData, void>(_T("resume"), &AppGateway::endpoint_resume, this);
        Register<AppIdParamsData, StateResultData>(_T("state"), &AppGateway::endpoint_state, this);

        Register<SendmessageParamsData, void>(_T("sendmessage"), &AppGateway::endpoint_sendmessage, this);
        Register<BroadcastParamsData, void>(_T("broadcast"), &AppGateway::endpoint_broadcast, this);

        Register<void, ConfigResultData>(_T("getconfig"), &AppGateway::endpoint_getconfig, this);
        Register<void, Core::JSON::ArrayType<Core::JSON::String>>(_T("listapps"), &AppGateway::endpoint_listapps, this);
    }

    void AppGateway::UnregisterAll() {
        Unregister(_T("launch"));
        Unregister(_T("stop"));
        Unregister(_T("suspend"));
        Unregister(_T("resume"));
        Unregister(_T("state"));

        Unregister(_T("sendmessage"));
        Unregister(_T("broadcast"));

        Unregister(_T("getconfig"));
        Unregister(_T("listapps"));
    }

    uint32_t AppGateway::EnsureRemoteDispatchers() {
        uint32_t result = Core::ERROR_NONE;

        if ((_launchDispatcher == nullptr) && (_config.LaunchDelegateCallsign.IsSet() == true) && (_config.LaunchDelegateCallsign.Value().empty() == false)) {
            _launchDispatcher = _service->QueryInterfaceByCallsign<PluginHost::IDispatcher>(_config.LaunchDelegateCallsign.Value());
            if (_launchDispatcher == nullptr) {
                result = Core::ERROR_UNAVAILABLE;
            }
        }

        if ((_a2aDispatcher == nullptr) && (_config.App2AppProviderCallsign.IsSet() == true) && (_config.App2AppProviderCallsign.Value().empty() == false)) {
            _a2aDispatcher = _service->QueryInterfaceByCallsign<PluginHost::IDispatcher>(_config.App2AppProviderCallsign.Value());
            if (_a2aDispatcher == nullptr) {
                result = Core::ERROR_UNAVAILABLE;
            }
        }

        if ((_notifDispatcher == nullptr) && (_config.AppNotificationsCallsign.IsSet() == true) && (_config.AppNotificationsCallsign.Value().empty() == false)) {
            _notifDispatcher = _service->QueryInterfaceByCallsign<PluginHost::IDispatcher>(_config.AppNotificationsCallsign.Value());
            if (_notifDispatcher == nullptr) {
                result = Core::ERROR_UNAVAILABLE;
            }
        }

        return result;
    }

    uint32_t AppGateway::SubscribeRemoteEvents() {
        uint32_t status = Core::ERROR_NONE;

        // Subscribe to LaunchDelegate state events
        if ((_launchDispatcher != nullptr) && (_launchEventsSubscribed == false)) {
            // We subscribe to "statechange" and "launched"/"stopped"
            if (_launchDispatcher->Subscribe(&_launchEventsSink, _T("statechange"), EMPTY_STRING) != Core::ERROR_NONE) {
                // Try the alt event name
                _launchDispatcher->Subscribe(&_launchEventsSink, _T("statechanged"), EMPTY_STRING);
            }
            _launchDispatcher->Subscribe(&_launchEventsSink, _T("launched"), EMPTY_STRING);
            _launchDispatcher->Subscribe(&_launchEventsSink, _T("stopped"), EMPTY_STRING);
            _launchDispatcher->Subscribe(&_launchEventsSink, _T("suspended"), EMPTY_STRING);
            _launchDispatcher->Subscribe(&_launchEventsSink, _T("resumed"), EMPTY_STRING);
            _launchEventsSubscribed = true;
        }

        // Subscribe to App2AppProvider message events
        if ((_a2aDispatcher != nullptr) && (_a2aEventsSubscribed == false)) {
            if (_a2aDispatcher->Subscribe(&_a2aEventsSink, _T("message"), EMPTY_STRING) != Core::ERROR_NONE) {
                status = Core::ERROR_GENERAL;
            }
            _a2aEventsSubscribed = true;
        }

        return status;
    }

    void AppGateway::UnsubscribeRemoteEvents() {
        if ((_launchDispatcher != nullptr) && (_launchEventsSubscribed == true)) {
            _launchDispatcher->Unsubscribe(&_launchEventsSink, _T("statechange"), EMPTY_STRING);
            _launchDispatcher->Unsubscribe(&_launchEventsSink, _T("statechanged"), EMPTY_STRING);
            _launchDispatcher->Unsubscribe(&_launchEventsSink, _T("launched"), EMPTY_STRING);
            _launchDispatcher->Unsubscribe(&_launchEventsSink, _T("stopped"), EMPTY_STRING);
            _launchDispatcher->Unsubscribe(&_launchEventsSink, _T("suspended"), EMPTY_STRING);
            _launchDispatcher->Unsubscribe(&_launchEventsSink, _T("resumed"), EMPTY_STRING);
            _launchEventsSubscribed = false;
        }

        if ((_a2aDispatcher != nullptr) && (_a2aEventsSubscribed == true)) {
            _a2aDispatcher->Unsubscribe(&_a2aEventsSink, _T("message"), EMPTY_STRING);
            _a2aEventsSubscribed = false;
        }
    }

    uint32_t AppGateway::InvokeRemote(PluginHost::IDispatcher* dispatcher,
                                      const string& method,
                                      const string& parameters,
                                      string& response) const
    {
        if (dispatcher == nullptr) {
            return Core::ERROR_UNAVAILABLE;
        }
        // Use default channel and id 0 for internal plugin invocation
        return dispatcher->Invoke(0 /*channel*/, 0 /*id*/, _securityToken /*token*/, method, parameters, response);
    }

    void AppGateway::EmitAppStateChanged(const string& appId, const string& state, const string& reason) {
        AppstatechangedParamsData payload;
        payload.Id = appId;
        payload.State = state;
        payload.Reason = reason;
        Notify(_T("appstatechanged"), payload);
    }

    void AppGateway::EmitAppMessage(const string& origin, const string& target, const string& payloadStr, const string& type) {
        AppmessageParamsData payload;
        payload.OriginId = origin;
        payload.TargetId = target;
        payload.Payload = payloadStr;
        payload.Type = type;
        Notify(_T("appmessage"), payload);
    }

    void AppGateway::EmitAppStarted(const string& appId) {
        AppstartedParamsData payload;
        payload.Id = appId;
        Notify(_T("appstarted"), payload);
    }

    void AppGateway::EmitAppStopped(const string& appId, const string& reason) {
        AppstoppedParamsData payload;
        payload.Id = appId;
        payload.Reason = reason;
        Notify(_T("appstopped"), payload);
    }

    // =============================
    // JSON-RPC Endpoints
    // =============================

    uint32_t AppGateway::endpoint_launch(const LaunchParamsData& params) {
        const string callsign = _config.LaunchDelegateCallsign.Value();

        if (callsign.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }

        EnsureRemoteDispatchers();

        if (_launchDispatcher == nullptr) {
            return Core::ERROR_UNAVAILABLE;
        }

        // Build parameters for remote delegate
        Core::JSON::Object p;
        p.Set(_T("id"), Core::JSON::String(params.Id.Value()));
        if (params.Args.IsSet() && params.Args.Value().empty() == false) {
            p.Set(_T("args"), Core::JSON::String(params.Args.Value()));
        }

        string response;
        const uint32_t result = InvokeRemote(_launchDispatcher, _config.StartMethod.Value(), p.ToString(), response);
        return result;
    }

    uint32_t AppGateway::endpoint_stop(const AppIdParamsData& params) {
        EnsureRemoteDispatchers();

        if (_launchDispatcher == nullptr) {
            return Core::ERROR_UNAVAILABLE;
        }

        Core::JSON::Object p;
        p.Set(_T("id"), Core::JSON::String(params.Id.Value()));

        string response;
        return InvokeRemote(_launchDispatcher, _config.StopMethod.Value(), p.ToString(), response);
    }

    uint32_t AppGateway::endpoint_suspend(const AppIdParamsData& params) {
        EnsureRemoteDispatchers();

        if (_launchDispatcher == nullptr) {
            return Core::ERROR_UNAVAILABLE;
        }
        Core::JSON::Object p;
        p.Set(_T("id"), Core::JSON::String(params.Id.Value()));

        string response;
        return InvokeRemote(_launchDispatcher, _config.SuspendMethod.Value(), p.ToString(), response);
    }

    uint32_t AppGateway::endpoint_resume(const AppIdParamsData& params) {
        EnsureRemoteDispatchers();

        if (_launchDispatcher == nullptr) {
            return Core::ERROR_UNAVAILABLE;
        }
        Core::JSON::Object p;
        p.Set(_T("id"), Core::JSON::String(params.Id.Value()));

        string response;
        return InvokeRemote(_launchDispatcher, _config.ResumeMethod.Value(), p.ToString(), response);
    }

    uint32_t AppGateway::endpoint_state(const AppIdParamsData& params, StateResultData& responseObj) {
        EnsureRemoteDispatchers();

        if (_launchDispatcher == nullptr) {
            return Core::ERROR_UNAVAILABLE;
        }

        Core::JSON::Object p;
        p.Set(_T("id"), Core::JSON::String(params.Id.Value()));

        string response;
        const uint32_t result = InvokeRemote(_launchDispatcher, _config.StateMethod.Value(), p.ToString(), response);

        if (result == Core::ERROR_NONE) {
            Core::JSON::Object state;
            state.FromString(response);
            Core::JSON::String s = state.Get(_T("state")).Value();
            responseObj.State = s.Value();
        }

        return result;
    }

    uint32_t AppGateway::endpoint_sendmessage(const SendmessageParamsData& params) {
        EnsureRemoteDispatchers();

        if (_a2aDispatcher == nullptr) {
            return Core::ERROR_UNAVAILABLE;
        }

        Core::JSON::Object p;
        p.Set(_T("originId"), Core::JSON::String(params.OriginId.Value()));
        p.Set(_T("targetId"), Core::JSON::String(params.TargetId.Value()));
        p.Set(_T("payload"), Core::JSON::String(params.Payload.Value()));
        if (params.Type.IsSet()) {
            p.Set(_T("type"), Core::JSON::String(params.Type.Value()));
        }

        string response;
        return InvokeRemote(_a2aDispatcher, _config.SendMessageMethod.Value(), p.ToString(), response);
    }

    uint32_t AppGateway::endpoint_broadcast(const BroadcastParamsData& params) {
        EnsureRemoteDispatchers();

        if (_notifDispatcher == nullptr) {
            return Core::ERROR_UNAVAILABLE;
        }

        Core::JSON::Object p;
        p.Set(_T("topic"), Core::JSON::String(params.Topic.Value()));
        p.Set(_T("payload"), Core::JSON::String(params.Payload.Value()));
        if (params.Type.IsSet()) {
            p.Set(_T("type"), Core::JSON::String(params.Type.Value()));
        }

        string response;
        return InvokeRemote(_notifDispatcher, _config.BroadcastMethod.Value(), p.ToString(), response);
    }

    uint32_t AppGateway::endpoint_getconfig(ConfigResultData& response) const {
        response.LaunchDelegateCallsign = _config.LaunchDelegateCallsign.Value();
        response.App2AppProviderCallsign = _config.App2AppProviderCallsign.Value();
        response.AppNotificationsCallsign = _config.AppNotificationsCallsign.Value();
        response.SubscribeAppEvents = _config.SubscribeAppEvents.Value();

        // Copy allowedApps
        for (uint16_t i = 0; i < _config.AllowedApps.Length(); ++i) {
            Core::JSON::String entry = _config.AllowedApps[i].Value();
            response.AllowedApps.Add(entry);
        }

        return Core::ERROR_NONE;
    }

    uint32_t AppGateway::endpoint_listapps(Core::JSON::ArrayType<Core::JSON::String>& response) const {
        for (uint16_t i = 0; i < _config.AllowedApps.Length(); ++i) {
            response.Add(_config.AllowedApps[i].Value());
        }
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
