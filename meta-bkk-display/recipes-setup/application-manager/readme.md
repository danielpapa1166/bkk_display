# Application Manager

`application-manager` is the device lifecycle supervisor. It selects the
current boot phase, launches the applications configured for that phase,
respects dependencies between them, and monitors child processes. It is the
systemd-enabled entry point that coordinates setup mode and normal display
operation.

## Architecture

The main executable parses `configuration.json`, determines the boot mode from
setup flag files, and runs a supervisor thread. SIGCHLD is consumed by that
thread with `sigwaitinfo`; it reaps children and retries pending applications
until dependency chains resolve. Applications can depend on another application
having started or having exited successfully.

The recipe also builds helper executables:

- `wpa_helper` writes wpa_supplicant and systemd-networkd configuration for the
  active setup mode.
- `logger_check` and `networkd_check` provide prerequisite checks used by the
  managed application graph.
- The HTTP module and bundled web assets provide the manager's control and
  health interface.

The detailed source and configuration formats are documented in the nested
`files/src/` and `files/app_cfg/` READMEs.

## Integration

`application-manager_0.1.bb` builds the supervisor with rbuflogd, cJSON,
chttp, and systemd support. It installs the binaries in `/usr/bin`, the
configuration in `/etc/application-manager/configuration.json`, web assets in
`/usr/share/application-manager/www`, and
`application-manager.service`. The unit is enabled by default, making this
component the runtime orchestrator for the packages named in its JSON
configuration.