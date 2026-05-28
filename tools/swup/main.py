#!/usr/bin/env python3

from __future__ import annotations


import sys

from dataclasses import replace
from pathlib import Path
import json
import shlex
import subprocess
from typing import Sequence
from swup_target_cfg import TargetConfig, load_target
from swup_ssh_handler import ssh_run, scp_to_target, scp_files_to_target
from swup_cli_parser import parse_args, add_target_args
from swup_err import SwupError

# config-server (C replacement for the old Python/Bottle bkk-setup-web)
WEBAPP_FILES_DIR = Path("/data/projects/bkk_display/meta-bkk-setup/recipes-setup/config-server/files")
WEBAPP_FILES = ["www/index.html", "www/styles.css", "www/app.js"]
WEBAPP_REMOTE_DIR = "/usr/share/config-server/www"
WEBAPP_SERVICE = "bkk-setup-web.service"
CONFIGURED_FLAG = "/etc/bkk-display-config/api-configured"
HTTP_TEST_SERVER_BUILD_ROOT = Path("/data/projects/bkk_display/build-rpi/tmp/work")
HTTP_TEST_SERVER_REMOTE_BINARY = "/usr/bin/config-server"
HTTP_TEST_SERVER_REMOTE_WWW_DIR = "/usr/share/config-server/www"
HTTP_TEST_SERVER_WWW_SOURCE_DIR = Path("/data/projects/bkk_display/meta-bkk-setup/recipes-setup/config-server/files/www")

# application-manager
AM_BUILD_ROOT = Path("/data/projects/bkk_display/build-rpi/tmp/work")
AM_REMOTE_BINARY = "/usr/bin/application_manager"
AM_SERVICE = "application-manager.service"
AM_LOCAL_CFG = Path("/data/projects/bkk_display/meta-bkk-setup/recipes-setup/application-manager/files/app_cfg/configuration.json")
AM_REMOTE_CFG_DIR = "/etc/application-manager"
AM_REMOTE_CFG = "/etc/application-manager/configuration.json"

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


def deploy_webapp(target: TargetConfig, dry_run: bool) -> None:
	local_files = [str(WEBAPP_FILES_DIR / f) for f in WEBAPP_FILES]
	for f in local_files:
		if not Path(f).exists():
			raise SwupError(f"Webapp file not found: {f}")

	print(f"Deploying webapp to {target.name} ({target.ssh_destination})")

	ssh_run(target, f"sudo systemctl stop {shlex.quote(WEBAPP_SERVICE)}", dry_run=dry_run)

	tmp_dir = "/tmp/bkk-setup-web.swup"
	ssh_run(target, f"mkdir -p {tmp_dir}", dry_run=dry_run)
	scp_files_to_target(target, local_files, tmp_dir, dry_run=dry_run)

	install_steps = []
	for f in WEBAPP_FILES:
		filename = Path(f).name
		mode = "0755" if f.endswith(".sh") else "0644"
		install_steps.append(
			f"sudo install -m {mode} {tmp_dir}/{shlex.quote(filename)} {shlex.quote(WEBAPP_REMOTE_DIR)}/{shlex.quote(filename)}"
		)
	install_steps.append(f"rm -rf {tmp_dir}")
	ssh_run(target, " && ".join(install_steps), dry_run=dry_run)

	ssh_run(target, f"sudo systemctl start {shlex.quote(WEBAPP_SERVICE)}", dry_run=dry_run)
	ssh_run(target, f"sudo systemctl is-active --quiet {shlex.quote(WEBAPP_SERVICE)}", dry_run=dry_run)
	print("Webapp deploy finished.")


def reset_cfg(target: TargetConfig, dry_run: bool) -> None:
	print(f"Resetting config on {target.name} ({target.ssh_destination})")
	ssh_run(target, f"sudo rm -f {shlex.quote(CONFIGURED_FLAG)}", dry_run=dry_run)
	ssh_run(target, "sudo reboot", dry_run=dry_run)
	print("Config reset done, device is rebooting.")


def resolve_http_test_server_binary(explicit_path: str | None) -> Path:
	if explicit_path:
		candidate = Path(explicit_path).expanduser().resolve()
		if not candidate.exists():
			raise SwupError(f"HTTP test server binary not found: {candidate}")
		return candidate

	# Search for config-server at architecture level to avoid massive recursive glob
	try:
		c_http_dirs = list(HTTP_TEST_SERVER_BUILD_ROOT.glob("*/config-server"))
	except Exception as e:
		raise SwupError(f"Error searching build directory: {e}")

	if not c_http_dirs:
		raise SwupError(
			"No config-server package directory found under build output. "
			"Build it first with bitbake config-server or pass --http-binary <path>."
		)

	# For each config-server package dir, look for the binary
	matches = []
	for pkg_dir in c_http_dirs:
		try:
			binaries = sorted(pkg_dir.glob("*/package/usr/bin/config-server"))
			matches.extend(binaries)
		except Exception:
			continue

	if not matches:
		raise SwupError(
			"No compiled config-server binary found under build output. "
			"Build it first with bitbake config-server or pass --http-binary <path>."
		)

	if len(matches) > 1:
		raise SwupError(
			"Multiple config-server binaries found. Pass --http-binary to choose one explicitly: "
			+ ", ".join(str(path) for path in matches)
		)

	return matches[0].resolve()


def resolve_http_test_server_www_files(binary_path: Path) -> list[Path]:
	www_dir = HTTP_TEST_SERVER_WWW_SOURCE_DIR
	if not www_dir.exists() or not www_dir.is_dir():
		raise SwupError(
			f"HTTP server www directory not found: "
			f"{www_dir}. Check meta-bkk-setup/recipes-setup/config-server/files/www"
		)

	www_files = sorted(path for path in www_dir.glob("*") if path.is_file())
	if not www_files:
		raise SwupError(f"No www files found in: {www_dir}")

	return www_files


def deploy_http_test_server_www(target: TargetConfig, dry_run: bool, www_files: list[Path]) -> None:
	tmp_dir = "/tmp/config-server-www.swup"
	ssh_run(target, f"mkdir -p {tmp_dir}", dry_run=dry_run)
	scp_files_to_target(target, [str(path) for path in www_files], tmp_dir, dry_run=dry_run)

	install_steps = [
		f"sudo mkdir -p {shlex.quote(HTTP_TEST_SERVER_REMOTE_WWW_DIR)}",
		f"sudo rm -f {shlex.quote(HTTP_TEST_SERVER_REMOTE_WWW_DIR)}/*",
	]
	for path in www_files:
		install_steps.append(
			f"sudo install -m 0644 {tmp_dir}/{shlex.quote(path.name)} "
			f"{shlex.quote(HTTP_TEST_SERVER_REMOTE_WWW_DIR)}/{shlex.quote(path.name)}"
		)
	install_steps.append(f"rm -rf {tmp_dir}")

	ssh_run(target, " && ".join(install_steps), dry_run=dry_run)


def resolve_am_binary(explicit_path: str | None) -> Path:
	if explicit_path:
		candidate = Path(explicit_path).expanduser().resolve()
		if not candidate.exists():
			raise SwupError(f"application-manager binary not found: {candidate}")
		return candidate

	try:
		am_pkg_dirs = list(AM_BUILD_ROOT.glob("*/application-manager"))
	except Exception as e:
		raise SwupError(f"Error searching build directory: {e}")

	if not am_pkg_dirs:
		raise SwupError(
			"No application-manager package directory found under build output. "
			"Build it first with bitbake application-manager or pass --am-binary <path>."
		)

	matches = []
	for pkg_dir in am_pkg_dirs:
		try:
			binaries = sorted(pkg_dir.glob("*/package/usr/bin/application_manager"))
			matches.extend(binaries)
		except Exception:
			continue

	if not matches:
		raise SwupError(
			"No compiled application-manager binary found under build output. "
			"Build it first with bitbake application-manager or pass --am-binary <path>."
		)

	if len(matches) > 1:
		raise SwupError(
			"Multiple application-manager binaries found. Pass --am-binary to choose one explicitly: "
			+ ", ".join(str(p) for p in matches)
		)

	return matches[0].resolve()


def deploy_AM(target: TargetConfig, dry_run: bool, explicit_binary_path: str | None) -> None:
	am_binary = resolve_am_binary(explicit_binary_path)
	am_target = replace(
		target,
		local_binary=am_binary,
		remote_binary=AM_REMOTE_BINARY,
		service_name=AM_SERVICE,
	)
	deploy(am_target, dry_run=dry_run, skip_restart=False)

	if not AM_LOCAL_CFG.exists():
		raise SwupError(f"application-manager config not found: {AM_LOCAL_CFG}")
	print(f"Uploading {AM_LOCAL_CFG} to {AM_REMOTE_CFG}")
	scp_files_to_target(target, [str(AM_LOCAL_CFG)], "/tmp", dry_run=dry_run)
	ssh_run(
		target,
		f"sudo mkdir -p {shlex.quote(AM_REMOTE_CFG_DIR)} && "
		f"sudo install -m 0644 /tmp/{shlex.quote(AM_LOCAL_CFG.name)} {shlex.quote(AM_REMOTE_CFG)} && "
		f"rm -f /tmp/{shlex.quote(AM_LOCAL_CFG.name)}",
		dry_run=dry_run,
	)
	print("Config upload finished.")


def deploy_http_test_server(target: TargetConfig, dry_run: bool, explicit_binary_path: str | None) -> None:
	http_binary = resolve_http_test_server_binary(explicit_binary_path)
	www_files = resolve_http_test_server_www_files(http_binary)
	http_target = replace(
		target,
		local_binary=http_binary,
		remote_binary=HTTP_TEST_SERVER_REMOTE_BINARY,
	)

	deploy(http_target, dry_run=dry_run, skip_restart=True)
	deploy_http_test_server_www(http_target, dry_run=dry_run, www_files=www_files)


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
		elif args.command == "deploy_webapp":
			deploy_webapp(target, dry_run=args.dry_run)
		elif args.command == "reset_cfg":
			reset_cfg(target, dry_run=args.dry_run)
		elif args.command == "deploy_http_test_server":
			deploy_http_test_server(
				target,
				dry_run=args.dry_run,
				explicit_binary_path=args.http_binary,
			)
		elif args.command == "deploy_AM":
			deploy_AM(
				target,
				dry_run=args.dry_run,
				explicit_binary_path=args.am_binary,
			)
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
