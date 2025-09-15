# AppNotifications Thunder Plugin

AppNotifications is a Thunder/WPEFramework plugin that provides a topic-based broadcast notification primitive.

- Clients broadcast using the `broadcast` JSON-RPC method.
- All subscribers to the plugin receive the `notification` event and can filter by `topic`.

This plugin aligns with the AppGateway integration, where AppGateway invokes `broadcast` on the `AppNotifications` callsign.

## Configuration

Place a configuration entry for the plugin in the Controller's configuration.

```json
{
  "callsign": "AppNotifications",
  "classname": "AppNotifications",
  "locator": "libAppNotifications.so",
  "autostart": true,
  "configuration": {
    "allowAnyTopics": true,
    "maxPayloadBytes": 262144,
    "allowedTopics": []
  }
}
```

- `allowAnyTopics` (bool): If true, any topic may be broadcast (default true).
- `maxPayloadBytes` (number): Maximum payload length permitted for `broadcast()` (default 262144 bytes).
- `allowedTopics` (array[string]): Allowlist of topics. If `allowAnyTopics` is false, only topics in this list are allowed.

If Thunder security is enabled, the plugin will use a token provided in the `THUNDER_SECURITY_TOKEN` environment variable for cross-plugin calls (not required for this provider).

## Methods

- broadcast(topic, payload, type?)
  - Broadcasts a notification.
  - Returns: void

- allowtopic(topic)
  - Adds a topic to the allowlist at runtime.
  - Returns: void

- disallowtopic(topic)
  - Removes a topic from the allowlist at runtime.
  - Returns: void

- listtopics()
  - Returns: [string] list of allowed topics (runtime snapshot).

- getconfig()
  - Returns: { allowAnyTopics, maxPayloadBytes, allowedTopics }

## Events

- notification(topic, payload, type)
  - Emitted whenever `broadcast` succeeds.

## Notes

- The plugin does not perform per-subscriber routing; clients are expected to filter the `notification` event by `topic`.
- If `allowAnyTopics=false`, the `topic` must be present in `allowedTopics` or the call will return `ERROR_PRIVILAGED_REQUEST`.
- Payload is validated against `maxPayloadBytes`.

## Compatibility

- Designed to conform with AppGateway expectations and Thunder plugin best practices:
  - full plugin lifecycle handling
  - explicit configuration parsing
  - JSON-RPC endpoint registration and typed models
  - event emission through JSON-RPC Notify()
