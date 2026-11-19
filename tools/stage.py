"""Stage loose assets beside the native executable or JS/Wasm pair."""

from pathlib import Path
import json
import shutil
import sys

root, dest = map(Path, sys.argv[1:3])
dest.mkdir(parents=True, exist_ok=True)


def copy_tree(src, dst):
    for path in src.rglob("*"):
        if path.is_file():
            target = dst / path.relative_to(src)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)


copy_tree(root / "assets", dest / "assets")
for f in ["LICENSE", "THIRD-PARTY.txt"]:
    shutil.copy2(root / f, dest / f)
if sys.argv[3] == "Emscripten":
    for f in (root / "web").iterdir():
        if f.name != "preload.js":
            if f.is_dir():
                copy_tree(f, dest / f.name)
            else:
                shutil.copy2(f, dest / f.name)
    manifest = sorted(
        p.relative_to(dest).as_posix()
        for p in (dest / "assets").rglob("*")
        if p.is_file()
    )
    (dest / "assets.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (dest / ".nojekyll").touch()
