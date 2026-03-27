import argparse
from typing import Iterable, Sequence


def add_target_args(parser: argparse.ArgumentParser) -> None:
	parser.add_argument("--target", help="Target name from the config. Uses default_target when omitted.")



def parse_args(argv: Sequence[str]) -> argparse.Namespace:
	parser = argparse.ArgumentParser(
		prog="swup",
		description="Small deploy helper for copying an updated app binary to a target over scp.",
	)
	parser.add_argument(
		"--config",
		default="/data/projects/bkk_display/tools/swup/swup.json",
		help="Path to swup JSON config file. Default: %(default)s",
	)

	subparsers = parser.add_subparsers(dest="command", required=True)

	deploy_parser = subparsers.add_parser("deploy", help="Copy a local binary to the target and restart the service.")
	add_target_args(deploy_parser)
	deploy_parser.add_argument("--dry-run", action="store_true", help="Print commands without executing them.")
	deploy_parser.add_argument(
		"--skip-restart",
		action="store_true",
		help="Copy the binary but do not stop or start the systemd service.",
	)

	status_parser = subparsers.add_parser("status", help="Show remote service and binary status.")
	add_target_args(status_parser)

	rollback_parser = subparsers.add_parser("rollback", help="Restore the last backup binary and restart the service.")
	add_target_args(rollback_parser)
	rollback_parser.add_argument("--dry-run", action="store_true", help="Print commands without executing them.")

	return parser.parse_args(argv)