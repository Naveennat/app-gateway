App-Gateway Tests

This folder contains unit/integration-style tests for the AppGateway plugin using lightweight C++ mocks instead of real Thunder runtime.

What is covered:
- Endpoint delegation to downstream plugins via IDispatcher (launch/stop/suspend/resume/state/sendmessage/broadcast).
- Config getter coverage.
- RemoteEventForwarder behavior for LaunchDelegate and App2AppProvider events (statechange/launched/stopped and message).

How it works:
- ShellMock simulates PluginHost::IShell and provides configured IDispatcher stubs per callsign.
- DispatcherMock implements Invoke/Subscribe/Unsubscribe and allows injecting remote events and capturing invocations.

Build and run (example):
- Integrate this CMakeLists.txt into parent build or run:
  mkdir -p build && cd build
  cmake ..
  make appgateway_tests
  ctest -R appgateway_tests --output-on-failure

No external Thunder libraries are required; we only compile against headers already present in this repo.
