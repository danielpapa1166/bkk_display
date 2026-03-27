# swup

Small deploy helper for pushing a rebuilt app binary to a target over `scp`.

## What it does

- copies one local binary to the target
- optionally stops and restarts the systemd service
- keeps one backup copy on the target for rollback
- shows basic remote status

## Config

Copy `swup.example.json` to `swup.json` and adjust the target details.

Example fields:

- `local_binary`: path to the freshly built binary on the host machine
- `remote_binary`: install path on the target, for example `/usr/bin/bkk-qt-app`
- `service_name`: systemd service to restart after deploy

## Usage

```bash
cd tools/swup
cp swup.example.json swup.json
python3 main.py deploy --dry-run
python3 main.py deploy
python3 main.py status
python3 main.py rollback
```

## Notes

- the target user needs SSH access and sudo permissions to replace the binary in `remote_binary`
- this is intentionally a small first version; it deploys only one executable, not a full app directory or assets
- if your build output path changes often, point `local_binary` at a stable symlink or wrapper path