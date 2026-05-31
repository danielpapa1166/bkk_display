# submodules

External and internal git repositories used by the bkk_display project.

## Contents

| Submodule | Description |
|-----------|-------------|
| `bkk_api` | BKK (Budapest public transport) open-data API client. Fetches real-time departure and trip data and exposes it to the Qt application. |
| `chttp` | Lightweight embedded HTTP/1.1 server library (no dependencies). Used by `config-server` (Wi-Fi setup UI) and `application-manager` (runtime control interface). |
| `cJSON` | Ultralightweight JSON parser in ANSI C (third-party, [DaveGamble/cJSON](https://github.com/DaveGamble/cJSON)). Used across multiple components for JSON serialisation/deserialisation. |
| `rbuflogd` | Lightweight logging daemon that uses a POSIX shared memory ring buffer. Client processes write logs via `librbuflogd_producer`; the daemon flushes them to stdout or a file. |

## Why these repos are here

The Yocto recipes for `bkk_api`, `chttp`, and `rbuflogd` use `SRC_URI` with `protocol=file`
pointing to the local clones, so the submodules are **required to be present** for the build to work —
Yocto treats them as local git sources, not remote fetches.
`cJSON` is fetched directly from GitHub by its recipe and does not strictly need the local clone.

Beyond the build, having all repos in one place is useful for:

- **clangd / IDE integration** — `compile_commands.json` can reference headers and sources
  across submodules, giving accurate code navigation and completion in the workspace.
- **Development** — changes can be made and tested locally before pushing upstream.
