"""Collect license texts from the pinned source and its dependencies."""
from pathlib import Path
import sys
root = Path(__file__).resolve().parents[1]
paths = sorted(p for p in root.rglob("*") if p.is_file()
    and not any(part.startswith(".") for part in p.relative_to(root).parts)
    and (p.name.lower().startswith(("license", "copying")) or p.name == "THIRD_PARTY_LICENSES.md"))
with Path(sys.argv[1]).open("w") as output:
    for path in paths:
        output.write(f"\n--- {path.relative_to(root)} ---\n")
        output.write(path.read_text(errors="replace"))
