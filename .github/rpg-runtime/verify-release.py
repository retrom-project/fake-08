#!/usr/bin/env python3
"""Validate fixed-identity Web assets and emit the runtime release descriptor."""
import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = json.loads((ROOT / "retrom-fork.json").read_text(encoding="utf-8"))


def describe(output, repository, tag, commit):
    if repository != MANIFEST["forkRepository"] or not re.fullmatch(MANIFEST["releaseTagPattern"], tag) or not re.fullmatch(r"[0-9a-f]{40}", commit):
        raise ValueError("RETROM_RELEASE_IDENTITY_INVALID")
    names = [name for name in MANIFEST["releaseAssets"] if name != "rpg-runtime-release.json"]
    if set(path.name for path in output.iterdir()) - {"rpg-runtime-release.json"} != set(names):
        raise ValueError("RETROM_RELEASE_ASSET_SET_INVALID")
    assets = []
    for name in names:
        path = output / name
        if path.is_symlink() or not path.is_file() or path.stat().st_size < 1024:
            raise ValueError("RETROM_RELEASE_ASSET_INVALID")
        contents = path.read_bytes()
        if name.endswith(".wasm") and contents[:8] != b"\x00asm\x01\x00\x00\x00":
            raise ValueError("RETROM_RELEASE_WASM_INVALID")
        if name.endswith(".mjs") and any(marker.encode() not in contents for marker in (
            "_retrom_abi", "_retrom_ready", "_retrom_load", "_retrom_step", "_retrom_stop",
            "_retrom_pixels", "_retrom_audio", "_retrom_audio_count", "_retrom_state",
            "_retrom_state_size", "_retrom_restore")):
            raise ValueError("RETROM_RELEASE_ABI_INVALID")
        assets.append({"filename": name, "observedSha256": hashlib.sha256(contents).hexdigest(), "sizeBytes": len(contents)})
    return {"schemaVersion": 1, "repository": repository, "tag": tag, "commit": commit,
            "adapterAbi": MANIFEST["adapterAbi"], "digestPolicy": "OBSERVED_CACHE_INTEGRITY_ONLY",
            "sourceCommits": {value["role"]: value["commit"] for value in MANIFEST["upstreams"]}, "assets": assets}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    for key in ("repository", "tag", "commit"):
        parser.add_argument("--" + key, required=True)
    args = parser.parse_args()
    metadata = describe(args.output, args.repository, args.tag, args.commit)
    (args.output / "rpg-runtime-release.json").write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8")
