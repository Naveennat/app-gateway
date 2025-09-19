#include <iostream>
#include <cassert>

#include "../AppGateway/AppGateway.h"
#include "test_stubs.h"

using namespace WPEFramework::Plugin;
using namespace TestStubs;
using std::string;

static string BuildConfig(const string& launchCS, const string& a2aCS, const string& notifCS, bool subscribe = true) {
    // Note: Only fields used by AppGateway are needed.
    string s = "{";
    s += "\"launchDelegateCallsign\":\"" + launchCS + "\",";
    s += "\"app2appProviderCallsign\":\"" + a2aCS + "\",";
    s += "\"appNotificationsCallsign\":\"" + notifCS + "\",";
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
    // Arrange: create AppGateway instance and test shell
    AppGateway gateway;
    ShellMock* shell = new ShellMock();

    // Prepare mock dispatchers
    auto* launch = new DispatcherMock();
    auto* a2a = new DispatcherMock();
    auto* notif = new DispatcherMock();

    // Configure shell to provide dispatchers by callsign
    shell->SetDispatcherFor("LaunchDelegate", launch);
    shell->SetDispatcherFor("App2AppProvider", a2a);
    shell->SetDispatcherFor("AppNotifications", notif);

    // Set config on shell
    shell->SetConfigLine(BuildConfig("LaunchDelegate", "App2AppProvider", "AppNotifications", false));

    // Act: Initialize gateway
    string initRes = gateway.Initialize(shell);
    assert(initRes.empty() && "Initialize should succeed");

    // Verify: getconfig returns configured callsigns and subscribe flag
    JsonData::AppGateway::ConfigResultData cfg;
    auto rcCfg = gateway.endpoint_getconfig(cfg);
    assert(rcCfg == Core::ERROR_NONE);
    assert(cfg.LaunchDelegateCallsign.Value() == "LaunchDelegate");
    assert(cfg.App2AppProviderCallsign.Value() == "App2AppProvider");
    assert(cfg.AppNotificationsCallsign.Value() == "AppNotifications");
    assert(cfg.SubscribeAppEvents.Value() == true || cfg.SubscribeAppEvents.Value() == false); // field set

    // Prepare downstream method return values
    launch->SetInvokeResult("launch", Core::ERROR_NONE);
    launch->SetInvokeResult("stop", Core::ERROR_NONE);
    launch->SetInvokeResult("suspend", Core::ERROR_NONE);
    launch->SetInvokeResult("resume", Core::ERROR_NONE);
    launch->SetInvokeResult("state", Core::ERROR_NONE, "{\"state\":\"running\"}");

    a2a->SetInvokeResult("sendmessage", Core::ERROR_NONE);
    notif->SetInvokeResult("broadcast", Core::ERROR_NONE);

    // Exercise endpoints

    // launch
    {
        JsonData::AppGateway::LaunchParamsData p;
        p.Id = "com.app.demo";
        p.Args = "{\"foo\":1}";
        auto rc = gateway.endpoint_launch(p);
        assert(rc == Core::ERROR_NONE);
        assert(launch->Invocations().size() >= 1);
        auto last = launch->Invocations().back();
        assert(last.method == "launch");
        assert(last.parameters.find("\"id\":\"com.app.demo\"") != string::npos);
        assert(last.parameters.find("\"args\":\"{\\\"foo\\\":1}\"") != string::npos);
    }

    // stop
    {
        JsonData::AppGateway::AppIdParamsData p;
        p.Id = "com.app.demo";
        auto rc = gateway.endpoint_stop(p);
        assert(rc == Core::ERROR_NONE);
        assert(launch->Invocations().back().method == "stop");
    }

    // suspend
    {
        JsonData::AppGateway::AppIdParamsData p;
        p.Id = "com.app.demo";
        auto rc = gateway.endpoint_suspend(p);
        assert(rc == Core::ERROR_NONE);
        assert(launch->Invocations().back().method == "suspend");
    }

    // resume
    {
        JsonData::AppGateway::AppIdParamsData p;
        p.Id = "com.app.demo";
        auto rc = gateway.endpoint_resume(p);
        assert(rc == Core::ERROR_NONE);
        assert(launch->Invocations().back().method == "resume");
    }

    // state
    {
        JsonData::AppGateway::AppIdParamsData p;
        p.Id = "com.app.demo";
        JsonData::AppGateway::StateResultData response;
        auto rc = gateway.endpoint_state(p, response);
        assert(rc == Core::ERROR_NONE);
        assert(response.State.Value() == "running");
        assert(launch->Invocations().back().method == "state");
    }

    // sendmessage
    {
        JsonData::AppGateway::SendmessageParamsData p;
        p.OriginId = "orig";
        p.TargetId = "tgt";
        p.Payload = "{\"p\":true}";
        p.Type = "custom";
        auto rc = gateway.endpoint_sendmessage(p);
        assert(rc == Core::ERROR_NONE);
        assert(a2a->Invocations().size() >= 1);
        auto last = a2a->Invocations().back();
        assert(last.method == "sendmessage");
        assert(last.parameters.find("\"originId\":\"orig\"") != string::npos);
        assert(last.parameters.find("\"targetId\":\"tgt\"") != string::npos);
        assert(last.parameters.find("\"payload\":\"{\\\"p\\\":true}\"") != string::npos);
        assert(last.parameters.find("\"type\":\"custom\"") != string::npos);
    }

    // broadcast
    {
        JsonData::AppGateway::BroadcastParamsData p;
        p.Topic = "topic1";
        p.Payload = "{\"n\":1}";
        p.Type = "system";
        auto rc = gateway.endpoint_broadcast(p);
        assert(rc == Core::ERROR_NONE);
        assert(notif->Invocations().size() >= 1);
        auto last = notif->Invocations().back();
        assert(last.method == "broadcast");
        assert(last.parameters.find("\"topic\":\"topic1\"") != string::npos);
    }

    // Negative paths: missing downstream dispatcher
    {
        // Deinit and re-init with missing callsigns
        gateway.Deinitialize(shell);
        shell->SetConfigLine(BuildConfig("", "", "", false));
        string r2 = gateway.Initialize(shell);
        assert(r2.empty());
        // Try sendmessage without a2a dispatcher
        JsonData::AppGateway::SendmessageParamsData p;
        p.OriginId = "a";
        p.TargetId = "b";
        p.Payload = "x";
        auto rc = gateway.endpoint_sendmessage(p);
        assert(rc == Core::ERROR_UNAVAILABLE);
    }

    // Cleanup
    gateway.Deinitialize(shell);
    shell->Release();
    // dispatchers held by shell mock had AddRef; release them
    launch->Release();
    a2a->Release();
    notif->Release();

    std::cout << "All AppGateway endpoint delegation tests passed.\n";
    return 0;
}
