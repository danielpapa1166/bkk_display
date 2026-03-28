from __future__ import annotations

import re
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Optional

@dataclass(frozen=True)
class LogRecord:
    wall_time: datetime
    mono_ms: int
    boot_id: str
    level: str
    component: str
    message: str

    # Parsed optional fields from message patterns:
    duration_ms: Optional[int] = None
    curl_code: Optional[int] = None
    http_code: Optional[int] = None
    arrivals: Optional[int] = None
    status: Optional[str] = None
    event: Optional[str] = None


@dataclass
class LogSession:
    boot_id: str
    records: list[LogRecord]



class LogParser:
    # Example:
    # 2026-03-24T18:35:57.797 [mono_ms=47408] [boot_id=...] [INFO] [BkkApiWrapper] Fetch cycle completed in 399 ms (status=ok, arrivals=9)
    _LINE_RE = re.compile(
        r"^(?P<ts>\S+)\s+"
        r"\[mono_ms=\s*(?P<mono>\d+)\]\s+"
        r"\[boot_id=(?P<boot_id>[^\]]+)\]\s+"
        r"\[(?P<level>[A-Z]+)\]\s+"
        r"\[(?P<component>[^\]]+)\]\s+"
        r"(?P<message>.*)$"
    )

    _FETCH_CYCLE_RE = re.compile(
        r"Fetch cycle completed in (?P<dur>\d+) ms \(status=(?P<status>[^,]+), arrivals=(?P<arr>\d+)\)"
    )
    _FETCH_STATION_RE = re.compile(
        r"Fetched station (?P<station>\S+) in (?P<dur>\d+) ms \((?P<arr>\d+) arrivals\)"
    )
    _ONLINE_OK_RE = re.compile(r"Online check successful in (?P<dur>\d+) ms")
    _ONLINE_FAIL_RE = re.compile(
        r"Online check failed \(curl=(?P<curl>-?\d+), http=(?P<http>-?\d+), total=(?P<dur>\d+) ms\)"
    )

    def __init__(self, log_path: Path | str) -> None:
        self.log_path = Path(log_path)
        self.parse_errors = 0

    def parse(self) -> list[LogSession]:
        sessions: dict[str, list[LogRecord]] = {}

        with self.log_path.open("r", encoding="utf-8", errors="replace") as f:
            # process line by line: 
            for line in f:
                line = line.rstrip("\n")
                if not line:
                    continue

                record = self._parse_line(line)
                if record is None:
                    self.parse_errors += 1
                    continue

                sessions.setdefault(record.boot_id, []).append(record)

        # create LogSession list and sort by mono ms: 
        result = [LogSession(boot_id=bid, records=recs) for bid, recs in sessions.items()]
        result.sort(key=lambda s: s.records[0].mono_ms if s.records else 0)
        return result

    def _parse_line(self, line: str) -> Optional[LogRecord]:
        # parse the line with the main regex: 
        m = self._LINE_RE.match(line)
        if not m:
            return None

        ts_raw = m.group("ts")
        # handle ISO8601 "Z" if it appears.
        if ts_raw.endswith("Z"):
            ts_raw = ts_raw[:-1] + "+00:00"

        try:
            wall_time = datetime.fromisoformat(ts_raw)
        except ValueError:
            return None

        # extract info from the message: 
        message = m.group("message")
        event, duration_ms, status, arrivals, curl_code, http_code = self._parse_message(message)

        return LogRecord(
            wall_time=wall_time,
            mono_ms=int(m.group("mono")),
            boot_id=m.group("boot_id"),
            level=m.group("level"),
            component=m.group("component"),
            message=message,
            event=event,
            duration_ms=duration_ms,
            status=status,
            arrivals=arrivals,
            curl_code=curl_code,
            http_code=http_code,
        )

    def _parse_message(
        self, message: str
    ) -> tuple[str, Optional[int], Optional[str], Optional[int], Optional[int], Optional[int]]:

        # parse message and extract optional fields: 

        if message == "Application started":
            # app started msg: 
            return "app_start", None, None, None, None, None

        # total api fetch time: 
        m = self._FETCH_CYCLE_RE.search(message)
        if m:
            return (
                "api_fetch_cycle",
                int(m.group("dur")),
                m.group("status"),
                int(m.group("arr")),
                None,
                None,
            )

        # fetch time for a single station:
        m = self._FETCH_STATION_RE.search(message)
        if m:
            return "api_station_fetch", int(m.group("dur")), None, int(m.group("arr")), None, None

        # online check ok result: 
        m = self._ONLINE_OK_RE.search(message)
        if m:
            # time duration: 
            return "online_check_ok", int(m.group("dur")), "ok", None, None, None

        # online check fail result:
        m = self._ONLINE_FAIL_RE.search(message)
        if m:
            return (
                "online_check_fail",
                int(m.group("dur")),
                "fail",
                None,
                int(m.group("curl")),
                int(m.group("http")),
            )

        # Fallback for messages not yet categorized.
        return "other", None, None, None, None, None