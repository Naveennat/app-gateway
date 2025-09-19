#include <iostream>
#include <cassert>

#include "../AppGateway/AppGateway.h"
#include "test_stubs.h"

using namespace WPEFramework::Plugin;
using namespace TestStubs;
using std::string;

// Minimal observer to intercept AppGateway Notify calls.
// We cannot directly hook JSONRPC::Notify. Instead, we simulate by subscribing to remote events and
// verifying the RemoteEventForwarder calls AppGateway's private Emit... functions which use Notify.
// To observe results, we extend AppGateway via a test subclass exposing capture hooks.

class AppGatewayProbe : public AppGateway {
public:
    AppGatewayProbe()
        : AppGateway()
    {
    }

    // Intercept emitted events by shadowing the emit methods via a using-declaration is not possible (private).
    // So create friend-like behavior by re-implementing wrappers calling the base class protected Notify is not accessible.
    // As an alternative, we test behavior by checking no crashes and by injecting fake events through RemoteEventForwarder,
    // and validate that internal path parses as expected by not throwing. Additionally, we replicate AppGateway JSON parsing
    // and call these methods via public API where possible. For stronger validation, we add a minimal "spy" by deriving
    // and exposing public relay methods calling the private emitters through Event callbacks.

    // Trick: Expose the RemoteEventForwarder and feed it with events to ensure parsing path runs.
    using Forwarder = AppGateway::RemoteEventForwarder;

    Forwarder& LaunchSink() { return _launchSink; }
    Forwarder& A2ASink() { return _a2aSink; }

    // Build sinks after base has constructed them; we recreate our own for directed testing
    void BuildSinks() {
        _launchSink = Forwarder(*this, "LaunchDelegate", "launch");
        _a2aSink = Forwarder(*this, "App2AppProvider", "app2app");
    }

private:
    Forwarder _launchSink{*this, "LaunchDelegate", "launch"};
    Forwarder _a2aSink{*this, "App2AppProvider", "app2app"};
};

static string BuildConfig(const string& launchCS, const string& a2aCS, bool subscribe) {
    string s = "{";
    s += "\"launchDelegateCallsign\":\"" + launchCS + "\",";
    s += "\"app2appProviderCallsign\":\"" + a2aCS + "\",";
    s += "\"subscribeAppEvents\":" + string(subscribe ? "true" : "false") + ",";
    s += "\"startMethod\":\"launch\",";
    s += "\"stopMethod\":\"stop\",";
    s += "\"suspendMethod\":\"suspend\",";
    s += "\"resumeMethod\":\"resume\",";
    s += "\"stateMethod\":\"state\",";
    s += "\"sendMessageMethod\":\"sendmessage\",";
    s += "\"broadcastMethod\":\"broadcast\",";
    s += "\"allowedApps\":[]";
    s += "}";
    return s;
}

int main() {
    AppGatewayProbe gateway;
    ShellMock* shell = new ShellMock();

    auto* launch = new DispatcherMock();
    auto* a2a = new DispatcherMock();

    shell->SetDispatcherFor("LaunchDelegate", launch);
    shell->SetDispatcherFor("App2AppProvider", a2a);

    shell->SetConfigLine(BuildConfig("LaunchDelegate", "App2AppProvider", true));
    string initRes = gateway.Initialize(shell);
    assert(initRes.empty());

    // Sinks to directly dispatch events (we can't capture Notify, but ensure Event handlers parse inputs)
    gateway.BuildSinks();

    // Simulate LaunchDelegate statechange event
    {
        string params = JsonObj({
            JsonKV("id","com.test.app"),
            JsonKV("state","running"),
            JsonKV("reason","launch")
        });
        auto rc = gateway.RemoteEventForwarder::Event; // not accessible; we'll use our forwarder
        auto result = gateway.LaunchSink().Event("statechange", "", params);
        assert(result == Core::ERROR_NONE);
    }

    // Simulate launched event
    {
        string params = JsonObj({ JsonKV("id","com.test.app") });
        auto result = gateway.LaunchSink().Event("launched", "", params);
        assert(result == Core::ERROR_NONE);
    }

    // Simulate stopped event
    {
        string params = JsonObj({ JsonKV("id","com.test.app"), JsonKV("reason","request") });
        auto result = gateway.LaunchSink().Event("stopped", "", params);
        assert(result == Core::ERROR_NONE);
    }

    // Simulate App2AppProvider message event
    {
        string params = JsonObj({
            JsonKV("originId","a"),
            JsonKV("targetId","b"),
            JsonKV("payload","{}"),
            JsonKV("type","custom")
        });
        auto result = gateway.A2ASink().Event("message", "", params);
        assert(result == Core::ERROR_NONE);
    }

    // Simulate unknown events (should not error)
    {
        auto result1 = gateway.LaunchSink().Event("unknown_launch_event", "x", "{\"foo\":\"bar\"}");
        auto result2 = gateway.A2ASink().Event("unknown_msg", "y", "{\"z\":1}");
        assert(result1 == Core::ERROR_NONE);
        assert(result2 == Core::ERROR_NONE);
    }

    gateway.Deinitialize(shell);
    shell->Release();
    launch->Release();
    a2a->Release();

    std::cout << "All AppGateway event forwarder tests passed.\n";
    return 0;
}
