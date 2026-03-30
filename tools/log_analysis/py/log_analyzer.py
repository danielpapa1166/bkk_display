from __future__ import annotations

from statistics import mean

from log_parser import LogSession


class LogAnalyzer:
    def __init__(self, sessions: list[LogSession]) -> None:
        self._sessions = sessions

    def calculate_average_api_response_time(self) -> float | None:
        durations = [
            record.duration_ms
            for session in self._sessions
            for record in session.records
            if record.component == "BkkApiWorker"
            and record.event == "api_fetch_cycle"
            and record.duration_ms is not None
        ]

        if not durations:
            return None

        return mean(durations)

    def print_summary(self) -> None:
        avg_api_response_time = self.calculate_average_api_response_time()

        print("Log analysis summary")
        print(f"Sessions: {len(self._sessions)}")

        if avg_api_response_time is None:
            print("Average API response time: no fetch cycle entries found")
        else:
            print(f"Average API response time: {avg_api_response_time:.1f} ms")

    def plot_online_check_success_rate(self) -> None:
        try:
            import matplotlib.pyplot as plt
        except ImportError as exc:
            raise RuntimeError("matplotlib is required to render the online check pie chart") from exc

        success_count = 0
        failure_count = 0

        for session in self._sessions:
            for record in session.records:
                if record.component != "OnlineChecker":
                    continue

                if record.event == "online_check_ok":
                    success_count += 1
                elif record.event == "online_check_fail":
                    failure_count += 1

        total = success_count + failure_count
        if total == 0:
            print("No OnlineChecker events found; skipping pie chart.")
            return

        labels = ["Success", "Failure"]
        sizes = [success_count, failure_count]
        colors = ["#4caf50", "#f44336"]
        explode = [0.02, 0.08]

        fig, ax = plt.subplots()
        ax.pie(
            sizes,
            labels=labels,
            colors=colors,
            explode=explode,
            autopct="%1.1f%%",
            startangle=90,
            wedgeprops={"linewidth": 1, "edgecolor": "white"},
        )
        ax.set_title(f"Online Checker Success Rate ({success_count}/{total} successful)")
        ax.axis("equal")
        plt.tight_layout()
        plt.show()