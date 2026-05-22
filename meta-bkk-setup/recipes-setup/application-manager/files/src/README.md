# Application Manager

A lightweight process supervisor for Linux embedded systems. It launches and monitors a set of applications defined in a JSON configuration file, respecting boot modes and inter-application dependencies.

## Architecture

| File | Responsibility |
|---|---|
| `am_main.c` | Entry point: parses CLI args, loads config, filters apps by boot mode, starts supervisor thread |
| `am_supervisor.c` | Supervisor thread: launches apps, handles SIGCHLD, reaps children, retries pending launches |
| `am_launcher.c` | Forks and execs individual apps; evaluates per-app prerequisites before launching |
| `am_config_parser.c` | Parses CLI arguments and the JSON config file into internal structs |
| `am_boot_mode.c` | Determines boot mode from flag files on disk |
| `am_types.h` | Shared type definitions (`app_config_t`, `app_info_t`, status enums) |

## Boot Modes

Boot mode is determined at startup by the presence of flag files in the `--boot-flags-dir` directory:

| Flag files present | Boot mode |
|---|---|
| neither | `WIFI_CONFIG` |
| `wifi-configured` only | `API_CONFIG` |
| both `wifi-configured` and `api-configured` | `NORMAL` |

Only apps whose `phases` list includes the active boot mode are loaded.

## Supervisor Thread

SIGCHLD is blocked on all threads and consumed exclusively via `sigwaitinfo` in the supervisor thread. On each signal:

1. `reap_children` — waits on all exited children and updates their status.
2. `trigger_app_start` — attempts to launch every not-yet-started app; repeats until a full pass produces no new launches, so that chains of `after_started` dependencies resolve within a single signal cycle.

## CLI Usage

```
application_manager --config <path> [--boot-flags-dir <path>]
```

| Argument | Required | Description |
|---|---|---|
| `--config` | yes | Path to the JSON application config file |
| `--boot-flags-dir` | no | Directory containing boot flag files (default: `/etc/bkk-display-config`) |
