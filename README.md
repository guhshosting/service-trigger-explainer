# service-trigger-explainer

> Runtime prerequisite for the exact qualified EXE: Microsoft Visual C++ x64 Redistributable is required when the imported runtime DLLs are absent. This supersedes any static-CRT/no-redistributable description of a source build below.


> Release context: see [qualified platforms, runtime requirements and package scope](RELEASE-QUALIFICATION.md).


Version 1.0.0. A native Unicode Windows command-line auditor that explains a local service's configured start/stop triggers. Triggers allow Windows to react to events rather than starting every service at boot.

```text
service-trigger-explainer W32Time
service-trigger-explainer --json W32Time
service-trigger-explainer --help
service-trigger-explainer --version
```

Supply a canonical service name (for example `W32Time`), not its display name. No display-name lookup, remote inspection, or enumeration command is provided.

Human output reports the name, display name, account, configured start type, delayed auto-start flag, current state, and trigger count. Each trigger has its numeric action/type, GUID, symbolic subtype when recognized, explanation, and data. Strings are quoted and control characters escaped to prevent terminal control injection. A service with no triggers reports `No service triggers are configured.` and exits successfully.

For example, a domain-join trigger is displayed as follows (configuration varies by installation):

```text
Trigger 1
  Action: Start (1)
  Type: Domain join/leave (3)
  Subtype GUID: {1CE20ABA-9851-4421-9430-1DDEB766E809}
  Subtype name: DOMAIN_JOIN_GUID
  Meaning: Start when the computer joins a domain.
```

## Decoding

The SDK categories are device-interface arrival, IP-address availability, domain join/leave, firewall-port events, group-policy changes, network endpoint events, custom system-state change, custom ETW events, and aggregate. Known GUIDs from `winsvc.h` identify first/last IP address, domain join/leave, firewall open/close, machine/user policy changes, RPC interface requests, named-pipe open requests, and custom system-state change. GUID names are checked against their expected category before using a specific explanation. Device class and ETW provider GUIDs have no universal SCM name database; unknown GUIDs are printed faithfully without invented names. Aggregate and custom system-state payload semantics are intentionally left opaque.

All five SDK data types are supported: binary hex, bounded UTF-16 strings/multi-strings, one-byte ETW level, and unsigned 64-bit keyword-any/all masks. Raw hex is always retained. Firewall strings contain a port/protocol and optional executable/user information; network endpoint strings identify the endpoint/interface; device strings filter hardware/compatible IDs. Custom payloads are provider-defined. Malformed lengths, missing terminators, invalid Unicode, unknown types/actions, and subtype mismatches are reported. Invalid SCM pointer ranges fail inspection safely.

Semantics follow Microsoft's [SERVICE_TRIGGER reference](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/ns-winsvc-service_trigger), [data item reference](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/ns-winsvc-service_trigger_specific_data_item), and [service trigger overview](https://learn.microsoft.com/en-us/windows/win32/services/service-trigger-events).

## JSON and errors

`--json` writes only UTF-8 JSON to stdout, including on failure. Successful responses use integer `schema_version: 1`, `application_version: "1.0.0"`, `service`, `trigger_count`, `triggers`, and `warnings`. See [schema](docs/schema-v1.json). Numeric SDK values and canonical GUIDs accompany decoded names. Unknown subtype names, unavailable state, and unavailable delayed-start configuration are null. Unsigned data values are decimal strings to preserve 64-bit precision in consumers. Errors contain `error.exit_code`, `error.win32_code` (null for non-Win32 errors), and `error.message`. Warnings are attached at result, trigger, or data-item level.

Exit codes: 0 success (including no triggers), 2 usage, 3 service not found, 4 access denied, 5 Windows inspection failure, 6 unexpected internal/output failure. Human errors go to stderr. Disappearing services and bounded sizing retries retain the actual Win32 error.

## Safety, privileges, and limitations

Application code only queries local SCM configuration and status. It requests `SC_MANAGER_CONNECT`, `SERVICE_QUERY_CONFIG`, and separately `SERVICE_QUERY_STATUS`. Ordinary users can inspect services whose ACLs allow those queries. It never elevates automatically. It contains no service mutation calls, registry writes, sockets, endpoint probes, trigger simulation, or intentional network communication. Windows may perform its own internal activity while servicing native queries; this guarantee describes application behavior.

The service account can be an empty string for driver services. Query failures for optional current-state or delayed-start information produce warnings and null JSON fields rather than implying a known value.

Configuration and state are separate snapshots and can race with administrative changes. Windows does not return trigger history, a reason for the last start/stop, current condition satisfaction, or a complete explanation of custom providers. A configured action is not a promise that Windows will perform it: disabled configuration, running dependents, resource shortages, and other service policy affect execution. The tool does not test any conditions or remediate problems.

## Compatibility and build

Targets Windows 10, Windows 11, and corresponding supported Windows Server generations. `_WIN32_WINNT=0x0A00`; no Server 2025-only APIs, Windows 8.1 support, scripting runtime, or external command dependency. Built with C++17, MSVC, CMake 3.24+, Windows SDK, and Ninja. The default MSVC runtime is dynamic; deployed systems need the matching supported Visual C++ redistributable.

Run from an x64 Native Tools developer shell:

```text
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
ctest --test-dir build/debug --output-on-failure
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
ctest --test-dir build/release --output-on-failure
cmake --install build/release --prefix stage
```

Equivalent `debug`, `release`, and `asan` CMake build/test presets are included. `scripts/validate-build.ps1 -Preset release` saves build and test logs under `artifacts`. In the developer shell, `scripts/static-analysis.ps1` runs clang-tidy's analyzer, bugprone, and performance checks plus Cppcheck. That script uses this development environment's LLVM/Cppcheck installation paths; adjust them for other installations. PowerShell 7 is optional for development-only independent JSON/schema tests; the application has no PowerShell dependency. Run `cpack --config build/release/CPackConfig.cmake -B artifacts` to create a ZIP from the install tree. ASan builds copy their compiler runtime DLL beside the test executables automatically; distribute the ordinary Release build.

All project targets use `/W4 /WX`. Set `STE_ANALYZE=ON` for `/analyze`; use a separate RelWithDebInfo tree with `STE_ASAN=ON` for MSVC ASan. CTest integration enumerates and queries existing local services with read-only rights, skips if the environment cannot provide a readable service, and never creates a fixture. Synthetic tests exercise malformed buffers and data independently of SCM. See [validation report](VALIDATION.md) for actual platform/tool results and pending release checks.


