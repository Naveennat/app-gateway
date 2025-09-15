# LaunchDelegate Thunder Plugin

LaunchDelegate is a Thunder/WPEFramework plugin that provides application lifecycle management.

- Control lifecycle with JSON-RPC methods: `launch`, `stop`, `suspend`, `resume`, and query `state`.
- Emits lifecycle events: `statechange`, `launched`, `stopped`, `suspended`, `resumed`.

Designed to integrate with AppGateway. AppGateway subscribes to these events (via `statechange`, `launched`, `stopped`, `suspended`, `resumed`) and forwards normalized events to clients.

## Configuration

Add the plugin configuration in the Controller:

```json
{
  "callsign": "LaunchDelegate",
  "classname": "LaunchDelegate",
  "locator": "libLaunchDelegate.so",
  "autostart": true,
  "configuration": {
    "allowAnyApps": true,
    "allowedApps": []
  }
}
```

- `allowAnyApps` (bool): If true, any app id may be controlled (default true).
- `allowedApps` (array[string]): Allowlist of app ids. If `allowAnyApps` is false, only ids present here can be managed.

If Thunder security is enabled, the plugin will use a token provided in the `THUNDER_SECURITY_TOKEN` environment variable when interacting over dispatcher interfaces (not required for this delegate in isolation).

## Methods

- launch(id, args?)
  - Launches an application (transitions state to "running").
  - Returns: void

- stop(id)
  - Stops an application (transitions state to "stopped").
  - Returns: void

- suspend(id)
  - Suspends an application (transitions state to "suspended").
  - Returns: void

- resume(id)
  - Resumes an application (transitions to "running").
  - Returns: void

- state(id) -> { state }
  - Returns the current state of the application. If an app has not been seen before, "stopped" is returned.

- getconfig() -> { allowAnyApps, allowedApps }
  - Returns the current configuration view.

- listapps() -> [string]
  - When `allowAnyApps=false`, returns the configured allowlist.
  - When `allowAnyApps=true`, returns the list of app ids seen so far in this runtime.

## Events

- statechange(id, state, reason)
  - Emitted on any lifecycle transition (e.g., "running", "suspended", "stopped").
  - `reason` is an implementation-defined string (e.g., "launch", "request").

- launched(id)
  - Emitted after a successful launch.

- stopped(id, reason)
  - Emitted after a stop request is processed.

- suspended(id)
  - Emitted after a suspend request is processed.

- resumed(id)
  - Emitted after a resume request is processed.

## Notes

- This delegate maintains in-memory state transitions. In a full system, this module would integrate with the platform-specific app runtime.
- AppGateway can call these methods via Dispatcher using the `LaunchDelegate` callsign and subscribe to the events.
- Event names intentionally match AppGateway's expectations:
  - `statechange`, `launched`, `stopped`, `suspended`, `resumed`.

## Compatibility

- Aligned with peer Thunder plugins in this repository:
  - full lifecycle handling
  - configuration parsing and allowlist checks
  - JSON-RPC endpoint registration with typed models
  - event emission through `Notify()`
