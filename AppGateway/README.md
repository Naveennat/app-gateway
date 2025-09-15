# AppGateway Thunder Plugin

AppGateway is a Thunder/WPEFramework plugin that unifies:
- LaunchDelegate (application lifecycle operations),
- App2AppProvider (inter-application messages), and
- AppNotifications (broadcast notifications)

into a single JSON-RPC API.

## Configuration

Place this plugin config (excerpt) under the Controller's plugins section:

```json
{
  "callsign": "AppGateway",
  "classname": "AppGateway",
  "locator": "libAppGateway.so",
  "autostart": true,
  "configuration": {
    "launchDelegateCallsign": "LaunchDelegate",
    "app2appProviderCallsign": "App2AppProvider",
    "appNotificationsCallsign": "AppNotifications",
    "subscribeAppEvents": true,

    "startMethod": "launch",
    "stopMethod": "stop",
    "suspendMethod": "suspend",
    "resumeMethod": "resume",
    "stateMethod": "state",

    "sendMessageMethod": "sendmessage",
    "broadcastMethod": "broadcast",

    "allowedApps": ["com.example.app1", "com.example.app2"]
  }
}
```

If Thunder security is enabled, a token can be supplied via environment variable `THUNDER_SECURITY_TOKEN`.

## Methods
- launch(id, args?)
- stop(id)
- suspend(id)
- resume(id)
- state(id) -> { state }
- sendmessage(originId, targetId, payload, type?)
- broadcast(topic, payload, type?)
- getconfig() -> subset of configuration
- listapps() -> [appId]

## Events
- appstatechanged(id, state, reason)
- appmessage(originId, targetId, payload, type)
- appstarted(id)
- appstopped(id, reason)

## Notes
- AppGateway uses Thunder's IDispatcher to invoke and subscribe to the configured downstream plugins.
- Event names are normalized and forwarded for easier client consumption.
- If a downstream plugin is not present, JSON-RPC calls return `ERROR_UNAVAILABLE`.
