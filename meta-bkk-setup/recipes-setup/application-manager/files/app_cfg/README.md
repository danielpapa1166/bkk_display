# Application Config Format

The configuration file is a JSON object with a single top-level `apps` array. Each element describes one application to be managed.

## Full Example

```json
{
  "apps": [
    {
      "name": "my-app",
      "binary": "/usr/bin/my-app",
      "args": ["--port", "8080"],
      "phases": ["NORMAL"],
      "after_exited": "setup-task",
      "after_started": "logger",
      "folder": "/var/run/my-app",
      "environment": ["MY_VAR=hello", "OTHER_VAR=world"]
    }
  ]
}
```

## Fields

| Field | Type | Required | Description |
|---|---|---|---|
| `name` | string | yes | Unique identifier for this application |
| `binary` | string | yes | Absolute path to the executable |
| `phases` | array of strings | yes | Boot modes in which this app runs. Valid values: `WIFI_CONFIG`, `API_CONFIG`, `NORMAL` |
| `args` | array of strings | no | Command-line arguments passed to the binary |
| `after_exited` | string | no | Name of another app that must have **exited with code 0** before this app is started |
| `after_started` | string | no | Name of another app that must be **running** before this app is started |
| `folder` | string | no | Directory to create (mode 0755) before launching the app; launch fails if creation fails |
| `environment` | array of strings | no | Environment variables in `KEY=VALUE` format, passed via `execve` |

## Dependency Rules

- `after_exited` and `after_started` can be used independently or together; both conditions must be satisfied if both are set.
- If a prerequisite app name is not found in the config, the dependent app is skipped with an `ERR_INVALID_CONFIG` status.
- Apps that do not meet their prerequisites are retried automatically on every subsequent child-exit event.
- Dependency chains of any depth resolve correctly — if app C depends on B which depends on A, all three will be launched in order within the same event cycle once A exits.

## Boot Phases

Each app must declare the phases it participates in. An app is only started if its `phases` list includes the active boot mode. Apps from other phases are not loaded at all.

## Notes

- App names referenced in `after_exited` or `after_started` must be present in the **same config file** and must share at least one common phase, otherwise the reference will not resolve.
- Empty string values for `after_exited` / `after_started` are treated as unset (no dependency).
