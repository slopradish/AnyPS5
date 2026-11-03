import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

MANIFEST = Path("docs/dev/debt/porting.md")
BLOCK = re.compile(r"```port-manifest\s*\n(.*?)\n```", re.S)


def git(repo, *args, check=True):
    result = subprocess.run(["git", "-C", str(repo), *args], capture_output=True, text=True, encoding="utf-8", errors="replace")
    if check and result.returncode:
        raise RuntimeError(f"git {' '.join(args[:3])} failed: {result.stderr.strip()}")
    return result


def load(repo):
    text = (repo / MANIFEST).read_text(encoding="utf-8", errors="replace")
    match = BLOCK.search(text)
    if not match:
        raise RuntimeError(f"{MANIFEST} has no ```port-manifest block")
    data = json.loads(match.group(1))
    return data["fixes"]


def tracked_text(repo, paths):
    """Concatenated text of the fix's paths, and the number of paths that exist."""
    pieces = []
    found = 0
    for name in paths:
        file = repo / name
        if not file.is_file():
            continue
        found += 1
        try:
            pieces.append(file.read_text(encoding="utf-8", errors="replace"))
        except OSError:
            continue
    return "\n".join(pieces), found


def added_lines(repo, commit, paths):
    args = ["show", "--format=", "--unified=0", "-U0", commit]
    if paths:
        args += ["--", *paths]
    out = git(repo, *args, check=False).stdout
    lines = []
    for line in out.splitlines():
        if not line.startswith("+") or line.startswith("+++"):
            continue
        text = line[1:].strip()
        if len(text) < 8:
            continue
        if text.startswith("//") or text.startswith("#") or text.startswith("/*") or text.startswith("*"):
            continue
        lines.append(text)
    return lines


def presence(repo, fix):
    marker = fix.get("marker")
    if marker:
        text, _ = tracked_text(repo, fix.get("paths", []))
        if not fix.get("paths"):
            result = git(repo, "grep", "-F", "-l", marker, "--", "core", check=False)
            return marker in result.stdout, None, "marker"
        return marker in text, None, "marker"
    source = fix.get("source")
    if not source or source == "upstream":
        return None, None, "no-source"
    lines = added_lines(repo, source, fix.get("paths", []))
    if not lines:
        return None, None, "no-lines"
    text, found = tracked_text(repo, fix.get("paths", []))
    if not fix.get("paths"):
        return None, None, "no-paths"
    current = {line.strip() for line in text.splitlines()}
    matched = sum(1 for line in lines if line in current)
    return matched / len(lines), matched / len(lines), f"{matched}/{len(lines)}"


def apply_fix(repo, fix):
    source = fix.get("source")
    if not source or source == "upstream":
        raise RuntimeError(f"{fix['id']} has no local source commit")
    if git(repo, "merge-base", "--is-ancestor", source, "HEAD", check=False).returncode == 0:
        raise RuntimeError(f"{fix['id']} is already in HEAD")
    return git(repo, "cherry-pick", "-x", source, check=False)


def main(argv=None):
    parser = argparse.ArgumentParser(description="report which fixes from the porting manifest are in the tree, and apply or print one")
    parser.add_argument("--repo", type=Path, default=Path("."))
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--all", action="store_true", help="list every fix, not only the ones that need attention")
    parser.add_argument("--apply", metavar="ID", help="cherry-pick the fix's source commit")
    parser.add_argument("--patch", metavar="ID", help="print the fix's source diff")
    args = parser.parse_args(argv)
    repo = git(args.repo, "rev-parse", "--show-toplevel").stdout.strip()
    repo = Path(repo)
    fixes = load(repo)
    if args.patch:
        fix = next((f for f in fixes if f["id"] == args.patch), None)
        if fix is None:
            parser.error(f"no fix named {args.patch}")
        sys.stdout.write(git(repo, "show", "--format=", fix["source"]).stdout)
        return 0
    if args.apply:
        fix = next((f for f in fixes if f["id"] == args.apply), None)
        if fix is None:
            parser.error(f"no fix named {args.apply}")
        result = apply_fix(repo, fix)
        sys.stdout.write(result.stdout)
        sys.stderr.write(result.stderr)
        if result.returncode:
            print(f"{fix['id']}: cherry-pick stopped (resolve the conflicts, then commit)", file=sys.stderr)
        return result.returncode
    rows = []
    for fix in fixes:
        present, ratio, detail = presence(repo, fix)
        if present is None:
            label = "unknown"
        elif present is True:
            label = "present"
        elif present is False:
            label = "missing"
        elif ratio is not None and ratio >= 0.5:
            label = "present"
        elif ratio:
            label = "partial"
        else:
            label = "missing"
        rows.append({"id": fix["id"], "declared": fix.get("status", ""), "detected": label, "ratio": ratio, "detail": detail, "source": fix.get("source"), "note": fix.get("note", "")})
    if args.json:
        print(json.dumps(rows, indent=2))
    else:
        width = max(len(row["id"]) for row in rows)
        for row in rows:
            if not args.all and row["detected"] == "present":
                continue
            ratio = f" {row['ratio']:.0%}" if isinstance(row["ratio"], float) else ""
            source = row["source"] or "-"
            print(f"{row['id']:<{width}}  {row['detected']:<8}{ratio:>5}  {source[:12]:<12}  {row['note']}")
        missing = [row for row in rows if row["detected"] in ("missing", "partial")]
        print(f"\n{len(missing)} of {len(rows)} fixes need attention")
    return 0


if __name__ == "__main__":
    sys.exit(main())
