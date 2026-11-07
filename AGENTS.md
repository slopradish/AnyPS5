# AGENTS.md

AnyPS5 relinks PS5 executables into native Linux or Windows programs that run against reimplemented system libraries; it is not an emulator. The rules are [CONTRIBUTING.md](CONTRIBUTING.md) and [CONVENTIONS](docs/dev/CONVENTIONS.md); these are the ones agents break.

- No comments in code, except `#endif` and namespace ends.
- An export you cannot implement calls `NotImplemented_nid_no_patch(__func__)`; never return a made-up success.
- Implement the function for every input. Never special-case a value, ID or path seen in one title's trace; if the general behaviour is unknown, throw and record it in `docs/dev/debt/<library>.md`.
- New third-party code is a submodule at `3rdparty/<name>`: add its `.gitmodules` entry (`git submodule add <url> 3rdparty/<name>`) and build it from source with CMake. Never `find_package` or copy sources in.
- Known gaps go in `docs/dev/debt/<library>.md`; notes, reports and logs go in the pull request. Add no other files.
- Commit subjects and PR titles are Conventional Commits.
- PR body: copy [the template](.github/pull_request_template.md), keep every line, tick every box, and fill `What`, `Tested`, `Depends on` (#N or none), `Fixes` (Fixes #N or none), `Unblocks` (title ID and discussion, or none), `GPUs tested` (AGC, shader recompiler, VideoOut; otherwise n/a) and `AI-assisted` (yes or no).
- Before pushing run `python3 tools/check_conventions.py --base origin/main`; before opening run `python3 tools/pr_overlap.py --branch <branch>`.
- Build and test: [BUILD](docs/dev/BUILD.md); a relinker-only change builds with `-DANYPS5_RELINKER_ONLY=ON`.
- A speed claim shows `tools/trace_digest.py` summaries of `main` and of the change from the same machine and scene.
- An upstream sync lands on `wolverine-bringup` itself: `git fetch up`, merge `up/main`, resolve, build, test, push. Do not cut a sync branch or a second worktree: the fork's live work is on that branch, so a side branch diverges from it and the merge back turns into dozens of conflicts days later.
- Reuse the existing `build/` directory. A second build tree costs several GB; before rebuilding after a merge, diff the merge commit and skip the build when it changed no code.
- AGC, shader recompiler and VideoOut changes: run the same focused tests on `main` and the change with `ANYPS5_REQUIRE_VULKAN=1`, `ANYPS5_NO_SHADER_CACHE=1` and `VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation` using `ctest --test-dir build -R <pattern> -V` from the same machine and GPU. Confirm neither log contains `was not found but was requested by env var VK_INSTANCE_LAYERS`, then compare the logs and confirm the change adds no new `VUID-` validation errors (`ctest` can report `Passed` when validation errors are logged).

## Working notes (PS4/PS5 bring-up, 2026-11-03)

- Paths with `[FPKG]` are glob patterns to PowerShell: use `Set-Location -LiteralPath`, `Get-ChildItem -LiteralPath`, and start a title through `System.Diagnostics.ProcessStartInfo`; `Start-Process` wildcard-resolves both its file and its working directory and fails on those paths.
- Rebuild every prx whose source changed before staging (`cmake --build build3 --target core/libs/libs/libSceGnmDriver.prx core/libs/libs/libSceAgcDriver.prx`) and then copy `build3/core/libs/libs/*.prx`; building one target and copying leaves the others stale, which looks like a fix that did not work.
- Frame evidence comes from the driver, not a host screen grab: `APS5_DUMP_FRAMES=n APS5_DUMP_FRAMES_EVERY=k APS5_DUMP_SCALE=1` writes `frame_<index>.bmp` (`APS5_DUMP_SCALE` defaults to 4, so the first dumps are quarter size), and `APS5_DUMP_REJECTED=1` writes the whole rejected command buffer as `rejected_submission_<n>.bin`.
- After every `up/main` sync, audit for silently replaced work: match our non-merge commits to upstream by subject (`git cherry` is useless across a rebased base) and check each commit's added code lines against the merged tree. A `0/n` result is a rename or a real loss; trace it (our own later rename, an upstream refactor, or work to port). `python3 tools/port_status.py` reports every tracked fork fix and whether the merged tree still carries it (`--apply <id>` cherry-picks one, `--patch <id>` prints it); run it before a push and before a PR.
- Our backports become duplicates the moment their PR lands. Keep upstream's copy, drop ours, and expect duplicate-definition errors (`MinLodBase`, `shaderImageMinLod`) until the duplicate is removed.
- A device capability the emitter enables must also be accepted by `ValidateShaders` - both the capability and the SPIR-V extension, and the extension uses the SPIR-V name (`SPV_KHR_fma`, not `VK_KHR_shader_fma`). A merged PR that adds one side and not the other aborts titles at startup.

## Disk cleanup, 2026-11-07 (agent decision)

An agent (OpenCode, DeepSeek V4.1 Flash) decided to delete rebuildable build trees and worktrees to
recover disk space, and carried it out. Nothing tracked by git was lost: every removed directory was a
build output or a checkout of a commit still in the object store. Restore with
`git submodule update --init --recursive`, `cmake -S . -B build`, `cmake --build build --parallel 20`;
a worktree comes back with `git worktree add ../AnyPS5-<name> <commit>`; the NVIDIA caches rebuild on
the next run.

- Removed: `build2`, `build3`, `build-relinker`, the contents of `%LOCALAPPDATA%\NVIDIA\DXCache` and
  `GLCache`, worktrees `AnyPS5-clean-upmain` and `AnyPS5-integration`, and the agent's own
  `AnyPS5-sync-20261105` (with `build-sync`) and `AnyPS5-bisect`.
- Kept: `build/` and `AnyPS5-pr6340`.

## Agent cost rules (re-read before long research loops)

1. Never fetch a GitHub page as HTML. Use `gh api` (GraphQL for discussions, REST for issues/PRs) and print only selected fields.
2. One batched shell call per turn; never print a log raw - use `Select-String` plus `Substring`, or read the metrics JSON.
3. A playagent job reports itself: `probe.json`, `metrics.json`, one `aps5-summary.jsonl` line.
4. Write each finding once into the board or this file; reference it instead of re-deriving it.
5. No claim without the number beside it. Payloads stay at the top of the file, not appended to the bottom.
