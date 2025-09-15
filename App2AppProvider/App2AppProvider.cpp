#include "App2AppProvider.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework {

    namespace {
        // Plugin metadata per Thunder best practice
        static Plugin::Metadata<Plugin::App2AppProvider> metadata(
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
            {}, // Preconditions
            {}, // Terminations
            {}  // Controls
        );
    }

namespace Plugin {

    using namespace JsonData::App2AppProvider;

    // Register the plugin in the Thunder/WPEFramework runtime
    SERVICE_REGISTRATION(App2AppProvider, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

    App2AppProvider::App2AppProvider()
        : PluginHost::JSONRPC()
        , _service(nullptr)
        , _config()
        , _adminLock()
        , _registered()
        , _allowlist()
        , _securityToken()
    {
    }

    App2AppProvider::~App2AppProvider() {
        if (_service != nullptr) {
            Deinitialize(_service);
        }
    }

    const string App2AppProvider::Initialize(PluginHost::IShell* service) {
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

        // Attach JSONRPC to service for any connection routing, even if not used directly here
        PluginHost::IShell::IConnectionServer::INotification* sink = nullptr;
        auto resultAttach = Attach(sink, _service);
        if (resultAttach != Core::ERROR_NONE) {
            SYSLOG(Logging::Startup, (_T("App2AppProvider: Failed to attach JSONRPC handler to service (code %u)"), resultAttach));
        }

        return string(); // empty string indicates success
    }

    void App2AppProvider::Deinitialize(PluginHost::IShell* service) {
        ASSERT(service == _service);

        // Detach JSONRPC
        PluginHost::IShell::IConnectionServer::INotification* sink = nullptr;
        Detach(sink);

        // Unregister JSON-RPC endpoints
        UnregisterAll();

        // Clear runtime state
        {
            Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
            _registered.clear();
            _allowlist.clear();
        }

        if (_service != nullptr) {
            _service->Release();
            _service = nullptr;
        }
    }

    string App2AppProvider::Information() const {
        Core::JSON::Object info;
        info.Set(_T("name"), Core::JSON::String(_T("App2AppProvider")));
        info.Set(_T("version"), Core::JSON::String(Core::NumberType<uint32_t>(API_VERSION_NUMBER_MAJOR).Text() + _T(".") +
                                                  Core::NumberType<uint32_t>(API_VERSION_NUMBER_MINOR).Text() + _T(".") +
                                                  Core::NumberType<uint32_t>(API_VERSION_NUMBER_PATCH).Text()));
        info.Set(_T("description"), Core::JSON::String(_T("App2AppProvider: inter-application messaging service.")));
        return info.ToString();
    }

    void App2AppProvider::RegisterAll() {
        Register<SendmessageParamsData, void>(_T("sendmessage"), &App2AppProvider::endpoint_sendmessage, this);
        Register<RegisterappParamsData, void>(_T("registerapp"), &App2AppProvider::endpoint_registerapp, this);
        Register<UnregisterappParamsData, void>(_T("unregisterapp"), &App2AppProvider::endpoint_unregisterapp, this);
        Register<void, Core::JSON::ArrayType<Core::JSON::String>>(_T("listregistered"), &App2AppProvider::endpoint_listregistered, this);
        Register<void, ConfigResultData>(_T("getconfig"), &App2AppProvider::endpoint_getconfig, this);
    }

    void App2AppProvider::UnregisterAll() {
        Unregister(_T("sendmessage"));
        Unregister(_T("registerapp"));
        Unregister(_T("unregisterapp"));
        Unregister(_T("listregistered"));
        Unregister(_T("getconfig"));
    }

    bool App2AppProvider::IsAllowedOrigin(const string& originId) const {
        if (_config.AllowAny.Value() == true) {
            return true;
        }
        return (_allowlist.find(originId) != _allowlist.end());
    }

    bool App2AppProvider::IsRegistered(const string& appId) const {
        return (_registered.find(appId) != _registered.end());
    }

    bool App2AppProvider::ValidatePayloadSize(const string& payload) const {
        const uint32_t maxBytes = _config.MaxPayloadBytes.Value();
        // Use ASCII character count as size approximation; in general Thunder limits are enforced at transport too.
        return (payload.length() <= maxBytes);
    }

    void App2AppProvider::EmitMessage(const string& originId,
                                      const string& targetId,
                                      const string& payload,
                                      const string& type)
    {
        MessageeventParamsData evt;
        evt.OriginId = originId;
        evt.TargetId = targetId;
        evt.Payload = payload;
        evt.Type = type;
        Notify(_T("message"), evt);
    }

    uint32_t App2AppProvider::endpoint_sendmessage(const SendmessageParamsData& params) {
        const string origin = params.OriginId.Value();
        const string target = params.TargetId.Value();
        const string payload = params.Payload.Value();
        const string type = params.Type.IsSet() ? params.Type.Value() : string();

        if (origin.empty() || target.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }

        if (ValidatePayloadSize(payload) == false) {
            return Core::ERROR_BAD_LENGTH;
        }

        // Permission checks
        if (_config.RequireRegister.Value() == true) {
            Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
            if (_registered.find(origin) == _registered.end()) {
                return Core::ERROR_PRIVILAGED_REQUEST; // Not registered, not allowed
            }
        }

        if (IsAllowedOrigin(origin) == false) {
            return Core::ERROR_PRIVILAGED_REQUEST; // Not in allowlist
        }

        // Emit the event to all subscribers. Recipients can filter by targetId on the client side.
        EmitMessage(origin, target, payload, type);

        return Core::ERROR_NONE;
    }

    uint32_t App2AppProvider::endpoint_registerapp(const RegisterappParamsData& params) {
        const string id = params.Id.Value();
        if (id.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }

        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        _registered.insert(id);
        return Core::ERROR_NONE;
    }

    uint32_t App2AppProvider::endpoint_unregisterapp(const UnregisterappParamsData& params) {
        const string id = params.Id.Value();
        if (id.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        auto it = _registered.find(id);
        if (it != _registered.end()) {
            _registered.erase(it);
        }
        return Core::ERROR_NONE;
    }

    uint32_t App2AppProvider::endpoint_listregistered(Core::JSON::ArrayType<Core::JSON::String>& response) const {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        for (const auto& id : _registered) {
            response.Add(Core::JSON::String(id));
        }
        return Core::ERROR_NONE;
    }

    uint32_t App2AppProvider::endpoint_getconfig(ConfigResultData& response) const {
        response.AllowAny = _config.AllowAny.Value();
        response.RequireRegister = _config.RequireRegister.Value();
        response.MaxPayloadBytes = _config.MaxPayloadBytes.Value();

        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        for (const auto& entry : _allowlist) {
            response.AllowedApps.Add(Core::JSON::String(entry));
        }
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
