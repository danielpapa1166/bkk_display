#!/usr/bin/env python3

from __future__ import annotations


import sys

from pathlib import Path
import json
import shlex
import subprocess
from swup_target_cfg import TargetConfig, load_target
from swup_ssh_handler import ssh_run, scp_to_target
from swup_cli_parser import parse_args, add_target_args
from swup_ssh_handler import ssh_run, scp_to_target
from swup_err import SwupError

# def build(target: TargetConfig, dry_run: bool, skip_restart: bool) -> None:



def deploy(target: TargetConfig, dry_run: bool, skip_restart: bool) -> None:
	if not target.local_binary.exists():
		raise SwupError(f"Local binary not found: {target.local_binary}")

	print(f"Deploying {target.local_binary} to {target.name} ({target.ssh_destination})")
	scp_to_target(target, dry_run=dry_run)

	remote_steps = []
	if not skip_restart:
		remote_steps.append(f"sudo systemctl stop {shlex.quote(target.service_name)}")
	remote_steps.extend(
		[
			(
				f"if [ -f {shlex.quote(target.remote_binary)} ]; then "
				f"sudo cp {shlex.quote(target.remote_binary)} {shlex.quote(target.remote_backup_binary)}; fi"
			),
			f"sudo install -m 0755 {shlex.quote(target.remote_tmp_binary)} {shlex.quote(target.remote_binary)}",
			f"rm -f {shlex.quote(target.remote_tmp_binary)}",
		]
	)
	if not skip_restart:
		remote_steps.extend(
			[
				f"sudo systemctl start {shlex.quote(target.service_name)}",
				f"sudo systemctl is-active --quiet {shlex.quote(target.service_name)}",
			]
		)

	ssh_run(target, " && ".join(remote_steps), dry_run=dry_run)
	print("Deploy finished.")


def rollback(target: TargetConfig, dry_run: bool) -> None:
	print(f"Rolling back {target.name} using {target.remote_backup_binary}")
	remote_command = " && ".join(
		[
			f"test -f {shlex.quote(target.remote_backup_binary)}",
			f"sudo systemctl stop {shlex.quote(target.service_name)}",
			f"sudo install -m 0755 {shlex.quote(target.remote_backup_binary)} {shlex.quote(target.remote_binary)}",
			f"sudo systemctl start {shlex.quote(target.service_name)}",
			f"sudo systemctl is-active --quiet {shlex.quote(target.service_name)}",
		]
	)
	ssh_run(target, remote_command, dry_run=dry_run)
	print("Rollback finished.")


def status(target: TargetConfig) -> None:
	remote_command = " && ".join(
		[
			f"echo 'service={shlex.quote(target.service_name)}'",
			f"systemctl is-active {shlex.quote(target.service_name)}",
			f"echo 'binary={shlex.quote(target.remote_binary)}'",
			f"ls -l {shlex.quote(target.remote_binary)}",
			f"echo 'backup={shlex.quote(target.remote_backup_binary)}'",
			f"if [ -f {shlex.quote(target.remote_backup_binary)} ]; then ls -l {shlex.quote(target.remote_backup_binary)}; else echo 'missing'; fi",
		]
	)
	ssh_run(target, remote_command, dry_run=False)


def main(argv: Sequence[str]) -> int:
	args = parse_args(argv)

	try:
		target = load_target(Path(args.config).expanduser(), getattr(args, "target", None))

		if args.command == "deploy":
			deploy(target, dry_run=args.dry_run, skip_restart=args.skip_restart)
		elif args.command == "status":
			status(target)
		elif args.command == "rollback":
			rollback(target, dry_run=args.dry_run)
		else:
			raise SwupError(f"Unsupported command: {args.command}")
	except (KeyError, ValueError, json.JSONDecodeError) as exc:
		print(f"Configuration error: {exc}", file=sys.stderr)
		return 2
	except SwupError as exc:
		print(str(exc), file=sys.stderr)
		return 1

	return 0


if __name__ == "__main__":
	raise SystemExit(main(sys.argv[1:]))
