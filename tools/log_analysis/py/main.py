"""BKK Qt App - log analysis entry point."""

import sys
from pathlib import Path

from log_analyzer import LogAnalyzer
from log_fetcher import LogFetcher
from log_parser import LogParser


DATA_DIR = Path(__file__).parent.parent / "data"


def main() -> int:
    fetcher = LogFetcher(
        host="192.168.0.50",
        user="root",
        remote_path="/.local/share/bkk-qt-app/bkk-qt-app.log",
        output_dir=DATA_DIR,
    )

    ls_res = fetcher.do_ls("/.local/share/bkk-qt-app/")
    print(f"Remote file check successful:\n{ls_res}\nProceeding to fetch the log...")

    log_path = fetcher.fetch()

    # ------------------------------------------------------------------ parse
    parser = LogParser(log_path)
    sessions = parser.parse()          # list of Session objects, one per boot_id

    if not sessions:
        print("No log sessions found.", file=sys.stderr)
        return 1
    else: 
        print(f"Parsed {len(sessions)} session(s) from the log with {parser.parse_errors} parse errors.")

    # --------------------------------------------------------------- analyse
    analyzer = LogAnalyzer(sessions)
    analyzer.print_summary()
    analyzer.plot_online_check_success_rate()
    return 0


if __name__ == "__main__":
    sys.exit(main())
