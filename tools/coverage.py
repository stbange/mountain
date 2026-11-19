"""Gate gcov line coverage for every engine, renderer and platform-library C file."""

from pathlib import Path

import argparse, json, subprocess, sys

p = argparse.ArgumentParser()
p.add_argument("build", type=Path)
p.add_argument("--minimum", type=float, default=95)
args = p.parse_args()
root = Path(__file__).resolve().parents[1]
files = {f.resolve(): {} for f in (root / "src").glob("*.c")}
for obj in args.build.resolve().rglob("*.gcno"):
    expected = {
        "main.c": "mountain.dir",
        "app.c": "mountain_app.dir",
        "render.c": "mountain_display.dir",
    }
    filename = obj.name.removesuffix(".gcno")
    if expected.get(filename, "mountain_engine.dir") not in obj.parts:
        continue
    if filename not in {f.name for f in files}:
        continue
    proc = subprocess.run(
        ["gcov", "--json-format", "--stdout", str(obj)],
        capture_output=True,
        text=True,
        check=True,
    )
    result = json.loads(proc.stdout)
    for entry in result["files"]:
        f = Path(entry["file"]).resolve()
        if f not in files:
            continue
        for line in entry["lines"]:
            n = line["line_number"]
            files[f][n] = files[f].get(n, False) or line["count"] > 0
hit = total = 0
for f, counts in sorted(files.items()):
    if not counts:
        sys.exit("No instrumentation for " + str(f))
    n = sum(counts.values())
    hit += n
    total += len(counts)
    missed = [str(k) for k, v in counts.items() if not v]
    print(
        f"{f.name:15s} {n:3}/{len(counts):3}  {100 * n / len(counts):6.2f}%"
        + ("  uncovered " + ",".join(missed) if missed else "")
    )
ratio = 100 * hit / total
print(
    f"Production C sources: {hit}/{total} executable lines ({ratio:.2f}%). Required: {args.minimum:g}%."
)
sys.exit(0 if ratio >= args.minimum else 1)
