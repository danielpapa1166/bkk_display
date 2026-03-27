from dataclasses import dataclass
from pathlib import Path
import json
from swup_err import SwupError


@dataclass(frozen=True)
class TargetConfig:
	name: str
	host: str
	user: str
	port: int
	local_binary: Path
	remote_binary: str
	service_name: str
	identity_file: Path | None = None

	@property
	def ssh_destination(self) -> str:
		return f"{self.user}@{self.host}"

	@property
	def remote_tmp_binary(self) -> str:
		return f"/tmp/{Path(self.remote_binary).name}.swup.new"

	@property
	def remote_backup_binary(self) -> str:
		return f"{self.remote_binary}.swup.bak"


def load_target(config_path: Path, target_name: str | None) -> TargetConfig:
	if not config_path.exists():
		raise SwupError(
			f"Config file not found: {config_path}. Copy tools/swup/swup.example.json to swup.json and adjust it."
		)

	with config_path.open("r", encoding="utf-8") as handle:
		raw = json.load(handle)

	selected_name = target_name or raw.get("default_target")
	if not selected_name:
		raise SwupError("No target selected. Pass --target or set default_target in the config.")

	targets = raw.get("targets", {})
	if selected_name not in targets:
		raise SwupError(f"Target '{selected_name}' not found in {config_path}.")

	target = targets[selected_name]
	identity_file = target.get("identity_file")

	return TargetConfig(
		name=selected_name,
		host=target["host"],
		user=target.get("user", "root"),
		port=int(target.get("port", 22)),
		local_binary=Path(target["local_binary"]).expanduser().resolve(),
		remote_binary=target["remote_binary"],
		service_name=target["service_name"],
		identity_file=Path(identity_file).expanduser().resolve() if identity_file else None,
	)
