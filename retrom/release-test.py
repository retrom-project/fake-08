#!/usr/bin/env python3
"""Exercise release acceptance and rejection against the actual built assets."""
import hashlib
import importlib.util
import shutil
import sys
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("release", root / ".github/rpg-runtime/verify-release.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
manifest = module.MANIFEST
repository = manifest["forkRepository"]
commit = manifest["upstreams"][0]["commit"]
tag = "retrom-core-g" + commit[:12] + "-r1"


def rejected(directory, repository=repository, tag=tag, commit=commit):
    try:
        module.describe(directory, repository, tag, commit)
    except ValueError:
        return
    raise AssertionError("invalid release accepted")


with tempfile.TemporaryDirectory() as temporary:
    directory = Path(temporary)
    names = [name for name in manifest["releaseAssets"] if name != "rpg-runtime-release.json"]
    for name in names:
        shutil.copyfile(Path(sys.argv[1]) / name, directory / name)
    metadata = module.describe(directory, repository, tag, commit)
    assert metadata["commit"] == commit and metadata["adapterAbi"] == manifest["adapterAbi"]
    for asset in metadata["assets"]:
        assert hashlib.sha256((directory / asset["filename"]).read_bytes()).hexdigest() == asset["observedSha256"]
    rejected(directory, repository="https://github.com/invalid/core")
    rejected(directory, tag="latest")
    rejected(directory, commit="HEAD")
    extra = directory / "unexpected.bin"
    extra.write_bytes(b"extra")
    rejected(directory)
    extra.unlink()
    wasm = next(directory.glob("*.wasm"))
    original = wasm.read_bytes()
    wasm.write_bytes(b"bad!" + original[4:])
    rejected(directory)
    wasm.write_bytes(original)
    javascript = next(directory.glob("*.mjs"))
    original = javascript.read_bytes()
    javascript.write_bytes(original.replace(b"_retrom_restore", b"_missing_restore"))
    rejected(directory)
    javascript.write_bytes(original)
    javascript.unlink()
    javascript.symlink_to(Path(sys.argv[1]).resolve() / javascript.name)
    rejected(directory)
print("release identity, exact assets, byte hashes, ABI and malformed assets PASS")
