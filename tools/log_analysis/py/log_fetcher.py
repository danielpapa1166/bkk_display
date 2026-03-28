"""
LogFetcher – downloads the bkk-qt-app log from the device via SCP.

Uses the system `scp` binary so no extra Python dependencies are required.
The caller is responsible for SSH key setup (passwordless auth recommended
for an embedded device).

Example
-------
    fetcher = LogFetcher(
        host="192.168.1.50",
        user="root",
        remote_path="/var/log/bkk-qt-app.log",
        output_dir=Path("../data"),
    )
    local_path = fetcher.fetch()
"""

import subprocess
from datetime import datetime
from pathlib import Path


class LogFetchError(Exception):
    """Raised when the SCP transfer fails."""


class LogFetcher:
    def __init__(
        self,
        host: str,
        user: str = "root",
        remote_path: str = "/.local/share/bkk-qt-app/bkk-qt-app.log",
        output_dir: Path = Path("../data"),
        port: int = 22,
    ) -> None:

        print(f"Initialized LogFetcher with host={host}, user={user}, remote_path={remote_path}, output_dir={output_dir}, port={port}")
        self._host = host
        self._user = user
        self._remote_path = remote_path
        self._output_dir = Path(output_dir)
        self._port = port

    def do_ls(self, remote_path: str) -> str:
        """Run an `ls` command on the remote path to check if the file exists and is readable."""
        cmd = [
            "ssh",
            "-o", "BatchMode=yes",
            "-o", "StrictHostKeyChecking=accept-new",
            f"{self._user}@{self._host}",
            f"ls -l {remote_path}",
        ]

        print(f"Checking remote file with: {' '.join(cmd)}")
        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            raise LogFetchError(
                f"Remote file check failed (exit {result.returncode}): {result.stderr.strip()}"
            )

        return result.stdout.strip()


    def fetch(self) -> Path:
        """Download the remote log and return the path to the local copy.

        The file is saved as:
            <output_dir>/<host>_bkk-qt-app_<YYYYMMDD_HHMMSS>.log

        so repeated fetches never overwrite earlier captures.
        """
        self._output_dir.mkdir(parents=True, exist_ok=True)

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        remote_filename = Path(self._remote_path).name
        stem = Path(remote_filename).stem
        local_path = self._output_dir / f"{self._host}_{stem}_{timestamp}.log"

        source = f"{self._user}@{self._host}:{self._remote_path}"

        cmd = [
            "scp",
            "-P", str(self._port),
            "-q",               # suppress progress meter
            "-o", "BatchMode=yes",          # never prompt for passwords
            "-o", "StrictHostKeyChecking=accept-new",  # auto-accept on first connect
            source,
            str(local_path),
        ]

        print(f"Fetching {source} → {local_path}")
        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            raise LogFetchError(
                f"SCP failed (exit {result.returncode}): {result.stderr.strip()}"
            )

        print(f"Saved {local_path.stat().st_size / 1024:.1f} KB")
        return local_path
