#!/usr/bin/env python3
"""Verify tag validation after checkout flattens a local annotated tag."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import textwrap

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / "retrom-fork.json").read_text())
workflow = (root / ".github/workflows/retrom-release.yml").read_text()
validation = workflow.split("      - name: Validate annotated maintenance-line tag\n        run: |\n", 1)[1].split("      - name:", 1)[0]
validation = textwrap.dedent(validation)

with tempfile.TemporaryDirectory() as temporary:
    directory = Path(temporary)
    def git(*args, cwd=directory):
        return subprocess.check_output(["git", *args], cwd=cwd, stderr=subprocess.DEVNULL, text=True).strip()
    git("init", "--bare", "origin.git")
    git("init", "source")
    source = directory / "source"
    git("config", "user.email", "test@example.invalid", cwd=source)
    git("config", "user.name", "Release Test", cwd=source)
    git("commit", "--allow-empty", "-m", "baseline", cwd=source)
    baseline = git("rev-parse", "HEAD", cwd=source)
    git("switch", "-c", manifest["defaultBranch"], cwd=source)
    git("commit", "--allow-empty", "-m", "adapter", cwd=source)
    commit = git("rev-parse", "HEAD", cwd=source)
    git("tag", "-a", "release-test", "-m", "annotated release", cwd=source)
    git("tag", "lightweight-test", cwd=source)
    git("remote", "add", "origin", str(directory / "origin.git"), cwd=source)
    git("push", "origin", "HEAD", "--tags", cwd=source)
    git("clone", "--branch", manifest["defaultBranch"], str(directory / "origin.git"), "checkout")
    checkout = directory / "checkout"
    git("update-ref", "refs/tags/release-test", commit, cwd=checkout)
    assert git("cat-file", "-t", "refs/tags/release-test", cwd=checkout) == "commit"
    validation = validation.replace(manifest["upstreams"][0]["commit"], baseline)
    def validate(tag, sha):
        env = dict(os.environ, GITHUB_REF_NAME=tag, GITHUB_SHA=sha)
        return subprocess.run(["bash", "-c", validation], cwd=checkout, env=env,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE).returncode
    assert validate("release-test", commit) == 0, "remote annotated tag must survive checkout flattening"
    assert git("cat-file", "-t", "refs/tags/release-test", cwd=checkout) == "tag"
    assert validate("lightweight-test", commit) != 0, "remote lightweight tags must be rejected"
    assert validate("release-test", baseline) != 0, "mismatched tag/checkout commit must be rejected"
print("annotated tag recovery, lightweight rejection and commit identity PASS")
