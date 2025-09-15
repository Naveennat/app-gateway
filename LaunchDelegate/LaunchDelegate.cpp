#include "LaunchDelegate.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework {

    namespace {
        // Plugin metadata per Thunder best practice
        static Plugin::Metadata<Plugin::LaunchDelegate> metadata(
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
            {}, // Preconditions
            {}, // Terminations
            {}  // Controls
        );
    }

namespace Plugin {

    using namespace JsonData::LaunchDelegate;

    // Register the plugin in the Thunder/WPEFramework runtime
    SERVICE_REGISTRATION(LaunchDelegate, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

    LaunchDelegate::LaunchDelegate()
        : PluginHost::JSONRPC()
        , _service(nullptr)
        , _config()
        , _adminLock()
        , _allowlist()
        , _apps()
        , _securityToken()
    {
    }

    LaunchDelegate::~LaunchDelegate() {
        if (_service != nullptr) {
            Deinitialize(_service);
        }
    }

    const string LaunchDelegate::Initialize(PluginHost::IShell* service) {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);

        _service = service;
        _service->AddRef();

        // Parse configuration
        _config.FromString(_service->ConfigLine());

        // Build allowlist set for quick lookup
        for (uint16_t i = 0; i < _config.AllowedApps.Length(); ++i) {
            _allowlist.insert(_config.AllowedApps[i].Value());
        }

        // Optionally load a security token (if Thunder security is deployed)
        _securityToken = Core::SystemInfo::GetEnvironment(_T("THUNDER_SECURITY_TOKEN"), _T(""));

        // Register JSON-RPC methods
        RegisterAll();

        // Attach JSONRPC to service for connection routing (standard practice)
        PluginHost::IShell::IConnectionServer::INotification* sink = nullptr;
        auto resultAttach = Attach(sink, _service);
        if (resultAttach != Core::ERROR_NONE) {
            SYSLOG(Logging::Startup, (_T("LaunchDelegate: Failed to attach JSONRPC handler to service (code %u)"), resultAttach));
        }

        return string(); // empty string indicates success
    }

    void LaunchDelegate::Deinitialize(PluginHost::IShell* service) {
        ASSERT(service == _service);

        // Detach JSONRPC
        PluginHost::IShell::IConnectionServer::INotification* sink = nullptr;
        Detach(sink);

        // Unregister JSON-RPC endpoints
        UnregisterAll();

        // Clear runtime state
        {
            Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
            _allowlist.clear();
            _apps.clear();
        }

        if (_service != nullptr) {
            _service->Release();
            _service = nullptr;
        }
    }

    string LaunchDelegate::Information() const {
        Core::JSON::Object info;
        info.Set(_T("name"), Core::JSON::String(_T("LaunchDelegate")));
        info.Set(_T("version"), Core::JSON::String(Core::NumberType<uint32_t>(API_VERSION_NUMBER_MAJOR).Text() + _T(".") +
                                                  Core::NumberType<uint32_t>(API_VERSION_NUMBER_MINOR).Text() + _T(".") +
                                                  Core::NumberType<uint32_t>(API_VERSION_NUMBER_PATCH).Text()));
        info.Set(_T("description"), Core::JSON::String(_T("LaunchDelegate: Application lifecycle management delegate.")));
        return info.ToString();
    }

    void LaunchDelegate::RegisterAll() {
        Register<LaunchParamsData, void>(_T("launch"), &LaunchDelegate::endpoint_launch, this);
        Register<AppIdParamsData, void>(_T("stop"), &LaunchDelegate::endpoint_stop, this);
        Register<AppIdParamsData, void>(_T("suspend"), &LaunchDelegate::endpoint_suspend, this);
        Register<AppIdParamsData, void>(_T("resume"), &LaunchDelegate::endpoint_resume, this);
        Register<AppIdParamsData, StateResultData>(_T("state"), &LaunchDelegate::endpoint_state, this);
        Register<void, Core::JSON::ArrayType<Core::JSON::String>>(_T("listapps"), &LaunchDelegate::endpoint_listapps, this);
        Register<void, ConfigResultData>(_T("getconfig"), &LaunchDelegate::endpoint_getconfig, this);
    }

    void LaunchDelegate::UnregisterAll() {
        Unregister(_T("launch"));
        Unregister(_T("stop"));
        Unregister(_T("suspend"));
        Unregister(_T("resume"));
        Unregister(_T("state"));
        Unregister(_T("listapps"));
        Unregister(_T("getconfig"));
    }

    bool LaunchDelegate::IsAllowedApp(const string& appId) const {
        if (_config.AllowAnyApps.Value() == true) {
            return true;
        }
        return (_allowlist.find(appId) != _allowlist.end());
    }

    void LaunchDelegate::EnsureAppEntry(const string& appId) {
        if (_apps.find(appId) == _apps.end()) {
            _apps.emplace(appId, AppRuntime());
        }
    }

    void LaunchDelegate::SetStateAndEmit(const string& appId, const string& newState, const string& reason) {
        {
            Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
            EnsureAppEntry(appId);
            _apps[appId].State = newState;
            _apps[appId].Reason = reason;
        }
        EmitStateChanged(appId, newState, reason);
    }

    void LaunchDelegate::EmitStateChanged(const string& appId, const string& state, const string& reason) {
        StatechangeeventParamsData payload;
        payload.Id = appId;
        payload.State = state;
        payload.Reason = reason;
        Notify(_T("statechange"), payload);
    }

    void LaunchDelegate::EmitLaunched(const string& appId) {
        LaunchedeventParamsData payload;
        payload.Id = appId;
        Notify(_T("launched"), payload);
    }

    void LaunchDelegate::EmitStopped(const string& appId, const string& reason) {
        StoppedeventParamsData payload;
        payload.Id = appId;
        payload.Reason = reason;
        Notify(_T("stopped"), payload);
    }

    void LaunchDelegate::EmitSuspended(const string& appId) {
        SuspendedeventParamsData payload;
        payload.Id = appId;
        Notify(_T("suspended"), payload);
    }

    void LaunchDelegate::EmitResumed(const string& appId) {
        ResumedeventParamsData payload;
        payload.Id = appId;
        Notify(_T("resumed"), payload);
    }

    uint32_t LaunchDelegate::endpoint_launch(const LaunchParamsData& params) {
        const string id = params.Id.Value();
        const string args = params.Args.IsSet() ? params.Args.Value() : string();

        if (id.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }

        if (IsAllowedApp(id) == false) {
            return Core::ERROR_PRIVILAGED_REQUEST;
        }

        {
            Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
            EnsureAppEntry(id);
            _apps[id].LastArgs = args;
        }

        // Transition to running and emit events
        SetStateAndEmit(id, _T("running"), _T("launch"));
        EmitLaunched(id);

        return Core::ERROR_NONE;
    }

    uint32_t LaunchDelegate::endpoint_stop(const AppIdParamsData& params) {
        const string id = params.Id.Value();

        if (id.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }

        if (IsAllowedApp(id) == false) {
            return Core::ERROR_PRIVILAGED_REQUEST;
        }

        // Transition to stopped and emit events
        SetStateAndEmit(id, _T("stopped"), _T("request"));
        EmitStopped(id, _T("request"));

        return Core::ERROR_NONE;
    }

    uint32_t LaunchDelegate::endpoint_suspend(const AppIdParamsData& params) {
        const string id = params.Id.Value();

        if (id.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }

        if (IsAllowedApp(id) == false) {
            return Core::ERROR_PRIVILAGED_REQUEST;
        }

        // Transition to suspended and emit events
        SetStateAndEmit(id, _T("suspended"), _T("request"));
        EmitSuspended(id);

        return Core::ERROR_NONE;
    }

    uint32_t LaunchDelegate::endpoint_resume(const AppIdParamsData& params) {
        const string id = params.Id.Value();

        if (id.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }

        if (IsAllowedApp(id) == false) {
            return Core::ERROR_PRIVILAGED_REQUEST;
        }

        // Transition to running and emit events
        SetStateAndEmit(id, _T("running"), _T("request"));
        EmitResumed(id);

        return Core::ERROR_NONE;
    }

    uint32_t LaunchDelegate::endpoint_state(const AppIdParamsData& params, StateResultData& response) const {
        const string id = params.Id.Value();

        if (id.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }

        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        auto it = _apps.find(id);
        if (it == _apps.end()) {
            // Default if never seen: stopped
            response.State = _T("stopped");
        } else {
            response.State = it->second.State;
        }

        return Core::ERROR_NONE;
    }

    uint32_t LaunchDelegate::endpoint_getconfig(ConfigResultData& response) const {
        response.AllowAnyApps = _config.AllowAnyApps.Value();

        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        for (const auto& entry : _allowlist) {
            response.AllowedApps.Add(Core::JSON::String(entry));
        }
        return Core::ERROR_NONE;
    }

    uint32_t LaunchDelegate::endpoint_listapps(Core::JSON::ArrayType<Core::JSON::String>& response) const {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        // Return allowlist if any, otherwise return keys of tracked apps
        if (_config.AllowAnyApps.Value() == false) {
            for (const auto& entry : _allowlist) {
                response.Add(Core::JSON::String(entry));
            }
        } else {
            for (const auto& kv : _apps) {
                response.Add(Core::JSON::String(kv.first));
            }
        }
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
