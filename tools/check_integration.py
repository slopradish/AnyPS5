import argparse
import subprocess
import sys
from pathlib import Path


def git(repo, *args, check=True):
    result = subprocess.run(["git", "-C", str(repo), *args], capture_output=True, text=True, encoding="utf-8", errors="replace")
    if check and result.returncode:
        raise RuntimeError(f"git {' '.join(args[:2])} failed: {result.stderr.strip()}")
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description="fail when HEAD does not descend from the long-lived integration branch, which is what a rebuild from upstream looks like")
    parser.add_argument("--repo", type=Path, default=Path("."))
    parser.add_argument("--integration", required=True, help="the branch that carries every feature commit (never rebuilt)")
    parser.add_argument("--head", default="HEAD")
    args = parser.parse_args(argv)
    repo = git(args.repo, "rev-parse", "--show-toplevel").stdout.strip()
    if git(repo, "rev-parse", "--verify", "--quiet", f"{args.integration}^{{commit}}", check=False).returncode:
        parser.error(f"{args.integration} is not a commit")
    if git(repo, "merge-base", "--is-ancestor", args.integration, args.head, check=False).returncode == 0:
        print(f"{args.head} descends from {args.integration}")
        return 0
    missing = git(repo, "rev-list", "--count", f"{args.head}..{args.integration}").stdout.strip()
    print(f"{args.head} does NOT descend from {args.integration}: {missing} integration commits are missing", file=sys.stderr)
    print("This is what a rebuild from upstream looks like. Merge or rebase the integration branch instead of replaying selected commits.", file=sys.stderr)
    subjects = git(repo, "log", "--oneline", f"{args.head}..{args.integration}").stdout.splitlines()
    for line in subjects[:20]:
        print(f"  {line}", file=sys.stderr)
    if len(subjects) > 20:
        print(f"  ... and {len(subjects) - 20} more", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
