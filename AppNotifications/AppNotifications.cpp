#include "AppNotifications.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework {

    namespace {
        // Plugin metadata per Thunder best practice
        static Plugin::Metadata<Plugin::AppNotifications> metadata(
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
            {}, // Preconditions
            {}, // Terminations
            {}  // Controls
        );
    }

namespace Plugin {

    using namespace JsonData::AppNotifications;

    // Register the plugin in the Thunder/WPEFramework runtime
    SERVICE_REGISTRATION(AppNotifications, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

    AppNotifications::AppNotifications()
        : PluginHost::JSONRPC()
        , _service(nullptr)
        , _config()
        , _adminLock()
        , _allowedTopics()
        , _securityToken()
    {
    }

    AppNotifications::~AppNotifications() {
        if (_service != nullptr) {
            Deinitialize(_service);
        }
    }

    const string AppNotifications::Initialize(PluginHost::IShell* service) {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);

        _service = service;
        _service->AddRef();

        // Parse configuration
        _config.FromString(_service->ConfigLine());

        // Build allowed topics set for quick lookup
        for (uint16_t i = 0; i < _config.AllowedTopics.Length(); ++i) {
            _allowedTopics.insert(_config.AllowedTopics[i].Value());
        }

        // Optionally load a security token (if Thunder security is deployed)
        _securityToken = Core::SystemInfo::GetEnvironment(_T("THUNDER_SECURITY_TOKEN"), _T(""));

        // Register JSON-RPC methods
        RegisterAll();

        // Attach JSONRPC to service for any connection routing, even if not used directly here
        PluginHost::IShell::IConnectionServer::INotification* sink = nullptr;
        auto resultAttach = Attach(sink, _service);
        if (resultAttach != Core::ERROR_NONE) {
            SYSLOG(Logging::Startup, (_T("AppNotifications: Failed to attach JSONRPC handler to service (code %u)"), resultAttach));
        }

        return string(); // empty string indicates success
    }

    void AppNotifications::Deinitialize(PluginHost::IShell* service) {
        ASSERT(service == _service);

        // Detach JSONRPC
        PluginHost::IShell::IConnectionServer::INotification* sink = nullptr;
        Detach(sink);

        // Unregister JSON-RPC endpoints
        UnregisterAll();

        // Clear runtime state
        {
            Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
            _allowedTopics.clear();
        }

        if (_service != nullptr) {
            _service->Release();
            _service = nullptr;
        }
    }

    string AppNotifications::Information() const {
        Core::JSON::Object info;
        info.Set(_T("name"), Core::JSON::String(_T("AppNotifications")));
        info.Set(_T("version"), Core::JSON::String(Core::NumberType<uint32_t>(API_VERSION_NUMBER_MAJOR).Text() + _T(".") +
                                                  Core::NumberType<uint32_t>(API_VERSION_NUMBER_MINOR).Text() + _T(".") +
                                                  Core::NumberType<uint32_t>(API_VERSION_NUMBER_PATCH).Text()));
        info.Set(_T("description"), Core::JSON::String(_T("AppNotifications: topic-based broadcast notifications service.")));
        return info.ToString();
    }

    void AppNotifications::RegisterAll() {
        Register<BroadcastParamsData, void>(_T("broadcast"), &AppNotifications::endpoint_broadcast, this);
        Register<ManageTopicParamsData, void>(_T("allowtopic"), &AppNotifications::endpoint_allowtopic, this);
        Register<ManageTopicParamsData, void>(_T("disallowtopic"), &AppNotifications::endpoint_disallowtopic, this);
        Register<void, Core::JSON::ArrayType<Core::JSON::String>>(_T("listtopics"), &AppNotifications::endpoint_listtopics, this);
        Register<void, ConfigResultData>(_T("getconfig"), &AppNotifications::endpoint_getconfig, this);
    }

    void AppNotifications::UnregisterAll() {
        Unregister(_T("broadcast"));
        Unregister(_T("allowtopic"));
        Unregister(_T("disallowtopic"));
        Unregister(_T("listtopics"));
        Unregister(_T("getconfig"));
    }

    bool AppNotifications::IsAllowedTopic(const string& topic) const {
        if (_config.AllowAnyTopics.Value() == true) {
            return true;
        }
        return (_allowedTopics.find(topic) != _allowedTopics.end());
    }

    bool AppNotifications::ValidatePayloadSize(const string& payload) const {
        const uint32_t maxBytes = _config.MaxPayloadBytes.Value();
        return (payload.length() <= maxBytes);
    }

    void AppNotifications::EmitNotification(const string& topic, const string& payload, const string& type) {
        NotificationeventParamsData evt;
        evt.Topic = topic;
        evt.Payload = payload;
        evt.Type = type;
        Notify(_T("notification"), evt);
    }

    uint32_t AppNotifications::endpoint_broadcast(const BroadcastParamsData& params) {
        const string topic = params.Topic.Value();
        const string payload = params.Payload.Value();
        const string type = params.Type.IsSet() ? params.Type.Value() : string();

        if (topic.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }

        if (ValidatePayloadSize(payload) == false) {
            return Core::ERROR_BAD_LENGTH;
        }

        if (IsAllowedTopic(topic) == false) {
            return Core::ERROR_PRIVILAGED_REQUEST;
        }

        // Emit the event to all subscribers. Clients can filter by topic on the client side.
        EmitNotification(topic, payload, type);

        return Core::ERROR_NONE;
    }

    uint32_t AppNotifications::endpoint_allowtopic(const ManageTopicParamsData& params) {
        const string topic = params.Topic.Value();
        if (topic.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }

        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        _allowedTopics.insert(topic);
        return Core::ERROR_NONE;
    }

    uint32_t AppNotifications::endpoint_disallowtopic(const ManageTopicParamsData& params) {
        const string topic = params.Topic.Value();
        if (topic.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }

        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        auto it = _allowedTopics.find(topic);
        if (it != _allowedTopics.end()) {
            _allowedTopics.erase(it);
        }
        return Core::ERROR_NONE;
    }

    uint32_t AppNotifications::endpoint_listtopics(Core::JSON::ArrayType<Core::JSON::String>& response) const {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        for (const auto& t : _allowedTopics) {
            response.Add(Core::JSON::String(t));
        }
        return Core::ERROR_NONE;
    }

    uint32_t AppNotifications::endpoint_getconfig(ConfigResultData& response) const {
        response.AllowAnyTopics = _config.AllowAnyTopics.Value();
        response.MaxPayloadBytes = _config.MaxPayloadBytes.Value();

        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        for (const auto& entry : _allowedTopics) {
            response.AllowedTopics.Add(Core::JSON::String(entry));
        }
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
