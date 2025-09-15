# App2AppProvider Thunder Plugin

App2AppProvider is a Thunder/WPEFramework plugin that provides a simple inter-application messaging primitive.

- Clients send messages using the `sendmessage` JSON-RPC method.
- All subscribers to the plugin receive the `message` event and can filter by `targetId`.

This plugin aligns with the AppGateway integration and event expectations, where AppGateway subscribes to the `message` event and forwards it as `appmessage`.

## Configuration

Place a configuration entry for the plugin in the Controller's configuration.

```json
{
  "callsign": "App2AppProvider",
  "classname": "App2AppProvider",
  "locator": "libApp2AppProvider.so",
  "autostart": true,
  "configuration": {
    "allowAny": true,
    "requireRegister": false,
    "maxPayloadBytes": 262144,
    "allowedApps": []
  }
}
```

- `allowAny` (bool): If true, any originId may send messages (default true).
- `requireRegister` (bool): If true, originId must be explicitly registered before sending (default false).
- `maxPayloadBytes` (number): Maximum payload length permitted for sendmessage() (default 262144 bytes).
- `allowedApps` (array[string]): Allowlist of originIds. If `allowAny` is false, only originIds in this list are allowed.

If Thunder security is enabled, the plugin will use a token provided in the `THUNDER_SECURITY_TOKEN` environment variable for cross-plugin calls (not required for this provider).

## Methods

- sendmessage(originId, targetId, payload, type?)
  - Sends an inter-application message.
  - Returns: void

- registerapp(id)
  - Registers an application id. Required if `requireRegister=true`.
  - Returns: void

- unregisterapp(id)
  - Unregisters a previously registered application id.
  - Returns: void

- listregistered()
  - Returns: [string] list of registered application ids.

- getconfig()
  - Returns: { allowAny, requireRegister, maxPayloadBytes, allowedApps }

## Events

- message(originId, targetId, payload, type)
  - Emitted whenever `sendmessage` succeeds.

## Notes

- The plugin does not perform per-subscriber routing; clients are expected to filter the `message` event by `targetId`.
- If `allowAny=false`, the `originId` must be present in `allowedApps` or the call will return `ERROR_PRIVILAGED_REQUEST`.
- If `requireRegister=true`, the `originId` must be registered via `registerapp` prior to sending messages.

## Compatibility

- Designed to conform with AppGateway expectations and Thunder plugin best practices:
  - full plugin lifecycle handling
  - explicit configuration parsing
  - JSON-RPC endpoint registration and typed models
  - event emission through JSON-RPC Notify()
