from typing import Iterable, Sequence
import shlex    
import subprocess
import sys

from swup_target_cfg import TargetConfig

def run_command(command: Sequence[str], dry_run: bool = False) -> subprocess.CompletedProcess[str] | None:
	printable = shlex.join(command)
	print(f"$ {printable}")
	if dry_run:
		return None

	completed = subprocess.run(command, check=False, text=True, capture_output=True)
	if completed.stdout:
		print(completed.stdout.rstrip())
	if completed.returncode != 0:
		if completed.stderr:
			print(completed.stderr.rstrip(), file=sys.stderr)
		raise SwupError(f"Command failed with exit code {completed.returncode}: {printable}")
	return completed


def build_ssh_base(target: TargetConfig) -> list[str]:
	command = ["ssh", "-p", str(target.port)]
	if target.identity_file:
		command.extend(["-i", str(target.identity_file)])
	command.append(target.ssh_destination)
	return command


def build_scp_base(target: TargetConfig) -> list[str]:
	command = ["scp", "-P", str(target.port)]
	if target.identity_file:
		command.extend(["-i", str(target.identity_file)])
	return command


def ssh_run(target: TargetConfig, remote_command: str, dry_run: bool = False) -> subprocess.CompletedProcess[str] | None:
	return run_command([*build_ssh_base(target), remote_command], dry_run=dry_run)


def scp_to_target(target: TargetConfig, dry_run: bool = False) -> None:
	run_command(
		[
			*build_scp_base(target),
			str(target.local_binary),
			f"{target.ssh_destination}:{target.remote_tmp_binary}",
		],
		dry_run=dry_run,
	)
