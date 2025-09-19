#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <optional>
#include <cassert>

#include "../AppGateway/Module.h"
#include "../AppGateway/AppGateway.h"

// Lightweight stubs mimicking Thunder PluginHost types enough for unit tests.
// We avoid linking real Thunder by providing minimal behavior.

namespace TestStubs {

    class DispatcherMock : public PluginHost::IDispatcher {
    public:
        struct InvokeCall {
            uint32_t channel;
            uint32_t id;
            string token;
            string method;
            string parameters;
        };

        DispatcherMock()
        : _refcount(1) {}

        // IUnknown-like reference management
        void AddRef() { ++_refcount; }
        uint32_t Release() override {
            if (--_refcount == 0) {
                delete this;
                return 0;
            }
            return _refcount;
        }

        BEGIN_INTERFACE_MAP(DispatcherMock)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

        // Invocation behavior can be configured per method
        void SetInvokeResult(const string& method, uint32_t resultCode, const string& response = string()) {
            _methodResults[method] = std::make_pair(resultCode, response);
        }

        // Track invocations
        const std::vector<InvokeCall>& Invocations() const { return _invocations; }

        uint32_t Invoke(const uint32_t channelId,
                        const uint32_t id,
                        const string& token,
                        const string& method,
                        const string& parameters,
                        string& response) override
        {
            _invocations.push_back({channelId, id, token, method, parameters});
            auto it = _methodResults.find(method);
            if (it != _methodResults.end()) {
                response = it->second.second;
                return it->second.first;
            }
            response.clear();
            return Core::ERROR_NONE;
        }

        // Event subscription/notification
        uint32_t Subscribe(PluginHost::IDispatcher::ICallback* callback,
                           const string& event,
                           const string& /*designator*/) override
        {
            _subscriptions.emplace_back(event, callback);
            return Core::ERROR_NONE;
        }

        uint32_t Unsubscribe(PluginHost::IDispatcher::ICallback* callback,
                             const string& event,
                             const string& /*designator*/) override
        {
            _subscriptions.erase(std::remove_if(_subscriptions.begin(), _subscriptions.end(),
                [&](const auto& p) { return (p.first == event) && (p.second == callback); }), _subscriptions.end());
            return Core::ERROR_NONE;
        }

        // Helper to simulate an incoming remote event for subscribers
        void EmitRemoteEvent(const string& event, const string& designator, const string& parameters) {
            for (auto& entry : _subscriptions) {
                if (entry.first == event) {
                    entry.second->Event(event, designator, parameters);
                }
            }
        }

    private:
        uint32_t _refcount;
        std::vector<std::pair<string, PluginHost::IDispatcher::ICallback*>> _subscriptions;
        std::map<string, std::pair<uint32_t, string>> _methodResults;
        std::vector<InvokeCall> _invocations;
    };

    class ShellMock : public PluginHost::IShell {
    public:
        ShellMock() : _refcount(1) {}

        void AddRef() override { ++_refcount; }
        uint32_t Release() override {
            if (--_refcount == 0) {
                delete this;
                return 0;
            }
            return _refcount;
        }

        // Configure config line that AppGateway reads
        void SetConfigLine(const string& cfg) { _configLine = cfg; }
        string ConfigLine() const { return _configLine; }

        template <typename INTERFACE>
        INTERFACE* QueryInterfaceByCallsign(const string& callsign) {
            // Not used directly; AppGateway calls specialized overload below.
            return nullptr;
        }

        // Provide dispatchers on demand by callsign
        void SetDispatcherFor(const string& callsign, PluginHost::IDispatcher* dispatcher) {
            _dispatchers[callsign] = dispatcher;
            if (dispatcher) dispatcher->AddRef();
        }

        // The AppGateway uses QueryInterfaceByCallsign<PluginHost::IDispatcher>
        PluginHost::IDispatcher* QueryInterfaceByCallsign(const string& callsign) {
            auto it = _dispatchers.find(callsign);
            if (it != _dispatchers.end()) {
                if (it->second) it->second->AddRef();
                return it->second;
            }
            return nullptr;
        }

        // Minimal IShell methods used by JSONRPC base
        // For our tests, we only need ConfigLine and QueryInterfaceByCallsign.
        // Provide adapters to satisfy the interface vtable if required by compiler.
        const string& VolatilePath() const override { return _dummy; }
        const string& PersistentPath() const override { return _dummy; }
        const string& DataPath() const override { return _dummy; }
        const string& SystemPath() const override { return _dummy; }
        const string& ProxyStubPath() const override { return _dummy; }
        void* QueryInterface(const uint32_t) override { return nullptr; }
        string ConfigLine() override { return _configLine; }

        template <typename T>
        T* QueryInterfaceByCallsign() { return nullptr; }

        template <>
        PluginHost::IDispatcher* QueryInterfaceByCallsign<PluginHost::IDispatcher>(const string& callsign) {
            return QueryInterfaceByCallsign(callsign);
        }

        // IConnectionServer is not actually used in tests; JSONRPC::Attach receives nullptr.
        IConnectionServer* ConnectionServer() override { return nullptr; }

    private:
        uint32_t _refcount;
        string _configLine;
        string _dummy;
        std::map<string, PluginHost::IDispatcher*> _dispatchers;
    };

    // Helper to build JSON strings
    inline string JsonKV(const string& k, const string& v) {
        return string("\"") + k + "\":\"" + v + "\"";
    }

    inline string JsonObj(std::initializer_list<string> fields) {
        string out = "{";
        bool first = true;
        for (auto& f : fields) {
            if (!first) out += ",";
            out += f;
            first = false;
        }
        out += "}";
        return out;
    }

} // namespace TestStubs
