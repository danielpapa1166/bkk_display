# submodules

External and internal git repositories used by the bkk_display project.

## Contents

| Submodule | Description |
|-----------|-------------|
| `bkk_api` | BKK (Budapest public transport) open-data API client. Fetches real-time departure and trip data and exposes it to the Qt application. |
| `chttp` | Lightweight embedded HTTP/1.1 server library (no dependencies). Used by `config-server` (Wi-Fi setup UI) and `application-manager` (runtime monitoring interface). |
| `cJSON` | Ultralightweight JSON parser in ANSI C (third-party, [DaveGamble/cJSON](https://github.com/DaveGamble/cJSON)). Used across multiple components for JSON serialisation/deserialisation. |
| `rbuflogd` | Lightweight logging daemon that uses a POSIX shared memory ring buffer. Client processes write logs via `librbuflogd_producer`; the daemon flushes them to stdout or a file. |