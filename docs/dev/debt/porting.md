# Porting tracker - fixes lost during upstream syncs

Every entry below is a fix present in a fork or in a pre-sync branch but not necessarily in
`wolverine-bringup`. The fenced `port-manifest` block is the machine-readable form: `python3
tools/port_status.py` reads it and reports, per fix, whether the tree still carries it (by a marker
or by the fraction of its added lines present), then points at the source commit. `--all` lists the
present ones too, `--patch <id>` prints a fix's diff, and `--apply <id>` cherry-picks it and leaves
any conflicts for you to resolve.

Run `python3 tools/port_status.py` after every upstream merge, before a push, and before opening a
PR. A fix that flips from present to missing in that report is a fix the merge dropped.

## Rendering (Astro Bot + Wolverine)

| # | Fix | Source | Status |
|---|-----|--------|--------|
| 1 | CMASK fast clears on colour targets | legacy `812c200ebe` | already in tree |
| 2 | Sampled texture soft limit to half the device budget | legacy `023ef90838` | needs porting |
| 3 | Charge cached sampled textures the bytes their image holds | legacy `f3c934082e` | ported (`d06f56606c`) |
| 4 | Release sampled texture host copies under host pressure | legacy `93faece10b` | ported on the current eviction design: `RunHostReleasePass` releases aged host copies once per present, `APS5_TEXTURE_HOST_MIB` / `APS5_NO_TEXTURE_HOST_RELEASE` |
| 5 | Bind misaligned draw buffer regions in place | legacy `f83abf2a7e` | ported (`3dccf1ef54`) |
| 6 | Skip zero-filling sampled texture snapshots | legacy `fef87ce8f6` | ported (`6df1db86db`) |
| 7 | Guard the reported wait-timeout set against concurrent queues | legacy `3cb2fc3694` | already in tree |
| 8 | Prove storage image refreshes over other pending images | sp4c3b4r8 `88d7bb6211` | needs porting; needs `Texture.hpp`/`Texture.cpp` refresh rework |

## GPU hang (Wolverine)

| # | Fix | Source | Status |
|---|-----|--------|--------|
| 9 | First-use shader and texture work off the queue-0 draw path | biladeiro `0b4545d8b3` | needs the first-use subsystem (`DeferSpecializationScope`, `SpecializationPending`, heap scan bound), which this tree does not have |
| 10 | Skip-listed dispatches return early instead of throwing | biladeiro `a15541ace1` | needs the `APS5_SKIP_SHADERS` feature (`59f7bb248a`, xxHash), which this tree does not have |
| 11 | Batch cap 256 recorded draws/dispatches | biladeiro `0e47407146` | ported (`f931bbf018`) |
| 12 | Fewer black frames and hitches from first-use work | biladeiro `8508ebf053` | partial: pipeline-cache save interval and texture-creation log gating are in; `SpecializedStored`, `APS5_REGISTERED_SKIP_MS` and the `firstSkip` bound need the first-use subsystem |

## Shader (Potter PS5)

| # | Fix | Source | Status |
|---|-----|--------|--------|
| 13 | Null T# binding as null image (TYPE 0/1) | upstream #5828 | already in tree |
| 14 | Cube T# through DIM 2D as face 0 | upstream #5509 | already in tree |
| 15 | 1D runtime mode for a 3D image bound as a flat volume (descriptor format 20) | local `af923a61b8` | ported, `verifyFlatVolumeLineMode` |

## PS4

| # | Fix | Source | Status |
|---|-----|--------|--------|
| 16 | PS4 GNM writers | local `4eda89a099` | done |
| 17 | INDEX_TYPE decode | local `77fbd10ac7` | done |
| 18 | NGS2 PS4 rack layout | local (refactored) | done |
| 19 | PS4 audio buzz | none yet | open: trace NGS2 output format/rate and AudioOut port handle |

## Pinning for the next sync

The recurring pain is that `wolverine-bringup-legacy` and the fork branches carry work the synced
branch dropped. The report above makes each loss visible; the commands below find the rest:

1. `python3 tools/port_status.py --all` - every tracked fix and whether it is still present.
2. `git log --oneline wolverine-bringup-legacy --not HEAD` - legacy commits not carried.
3. `git log --oneline biladeiro/wolverine-main --not HEAD` - biladeiro's unported commits.
4. `git log --oneline dean/astrobot --not HEAD` - dean's unported commits.
5. `git log --oneline sp4c3b4r8/astrobot-windows --not HEAD` - sp4c3b4r8's unported commits.
6. For a commit whose subject looks lost, run `git cherry-pick -x <sha>` on a scratch branch: a
   clean pick is fast; a conflict means the surrounding subsystem was rewritten and the fix has to
   be re-expressed on the current design (add it here with `--patch <id>` as the reference).

```port-manifest
{
  "fixes": [
    {"id": "cmask-fast-clears", "source": "812c200ebe", "status": "already", "paths": ["core/libs/prx/libSceAgcDriver/Graphics/src/TextureDetiler.cpp", "core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp"], "note": "CMASK fast clears on colour targets"},
    {"id": "sampled-soft-limit-half", "source": "023ef90838", "status": "missing", "paths": ["core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp", "core/libs/prx/libSceAgcDriver/Graphics/src/Texture.cpp"], "note": "sampled texture soft limit to half the device budget"},
    {"id": "charge-texture-bytes", "source": "f3c934082e", "status": "ported", "paths": ["core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp"], "marker": "ChargedTextureBytes", "note": "charge cached sampled textures the bytes their image holds"},
    {"id": "host-copy-release", "source": "93faece10b", "status": "partial", "paths": ["core/libs/prx/libSceAgcDriver/Graphics/include/TextureResidency.hpp", "core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp"], "marker": "RunHostReleasePass", "note": "release sampled texture host copies under host pressure"},
    {"id": "misaligned-draw-buffers", "source": "f83abf2a7e", "status": "ported", "paths": ["core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp"], "note": "bind misaligned draw buffer regions in place"},
    {"id": "skip-zero-fill-snapshots", "source": "fef87ce8f6", "status": "ported", "paths": ["core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp"], "marker": "RawTextureSnapshots", "note": "skip zero-filling sampled texture snapshots"},
    {"id": "wait-timeout-guard", "source": "3cb2fc3694", "status": "already", "paths": ["core/libs/prx/libSceAgcDriver/Execution/src/Driver/Synchronization/WaitMemory.cpp"], "note": "guard the reported wait-timeout set against concurrent queues"},
    {"id": "refresh-proof-pending", "source": "88d7bb6211", "status": "missing", "paths": ["core/libs/prx/libSceAgcDriver/Graphics/include/Texture.hpp", "core/libs/prx/libSceAgcDriver/Graphics/src/Texture.cpp"], "marker": "othersHold", "note": "prove storage image refreshes over other pending images"},
    {"id": "first-use-off-draw-path", "source": "0b4545d8b3", "status": "missing", "paths": ["core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/DrawCapture.cpp", "core/libs/prx/libSceAgcDriver/Execution/src/Driver/Shaders/ShaderRegistry.cpp", "core/libs/prx/libSceAgcDriver/Execution/src/ShaderMemory.cpp", "core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp", "core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp", "core/shader/recompiler/Recompiler.cpp", "core/shader/recompiler/Recompiler.hpp"], "marker": "deferAtUse", "note": "keep first-use shader and texture work off the queue-0 draw path"},
    {"id": "skip-listed-dispatches", "source": "a15541ace1", "status": "missing", "paths": ["core/libs/prx/libSceAgcDriver/Execution/src/Driver/Dispatch/Dispatch.cpp", "core/libs/prx/libSceAgcDriver/Execution/src/Driver/Dispatch/IndirectDispatch.cpp", "core/libs/prx/libSceAgcDriver/Execution/src/Driver/Shaders/ShaderRegistry.cpp", "core/libs/prx/libSceAgcDriver/Execution/include/Driver/Shaders/ShaderRegistry.hpp"], "marker": "SkipListed", "note": "skip-listed dispatches return early instead of throwing"},
    {"id": "batch-cap-256", "source": "0e47407146", "status": "ported", "paths": ["core/libs/prx/libSceAgcDriver/Execution/src/Driver/Synchronization/DeferredLabels.cpp"], "note": "batch cap 256 recorded draws/dispatches"},
    {"id": "fewer-black-frames", "source": "8508ebf053", "status": "partial", "paths": ["core/libs/prx/libSceAgcDriver/Graphics/include/PipelineCache.hpp", "core/libs/prx/libSceAgcDriver/Graphics/src/PipelineCache.cpp", "core/libs/prx/libSceAgcDriver/Graphics/src/Texture.cpp", "core/shader/recompiler/Recompiler.cpp", "core/shader/recompiler/ShaderDiskCache.cpp", "core/shader/recompiler/ShaderDiskCache.hpp"], "marker": "SpecializedStored", "note": "fewer black frames and hitches; save interval and texture log gating are in, the rest needs the first-use subsystem"},
    {"id": "null-t-binding", "source": "upstream", "status": "already", "paths": ["core/libs/prx/libSceAgcDriver/Graphics/src/GuestTextureResource.cpp", "core/shader/recompiler/Optimization/src/ResourceMaterializer.cpp"], "marker": "return base40 == 0 || type <= 1;", "note": "null T# binding as null image (TYPE 0/1), upstream #5828"},
    {"id": "cube-t-through-2d", "source": "upstream", "status": "already", "paths": ["core/shader/recompiler/SpirvBackend/src/SpirvImageEmitter.cpp", "core/shader/recompiler/Optimization/src/ResourceMaterializer.cpp"], "marker": "cubeFace = cube && access.mem.imageDimension", "note": "cube T# through DIM 2D as face 0, upstream #5509"},
    {"id": "potter-flat-volume-mode", "source": "af923a61b8", "status": "ported", "paths": ["core/shader/recompiler/Optimization/src/ResourceMaterializer.cpp", "core/libs/prx/libSceAgcDriver/tests/ShaderMemory.cpp"], "marker": "did not prepare a 1D Uint runtime mode", "note": "1D runtime mode for a 3D image bound as a flat volume (Potter PPSA22946)"},
    {"id": "ps4-gnm-writers", "source": "4eda89a099", "status": "done", "paths": ["core/libs/prx/libSceGnmDriver/Export.cpp"], "note": "PS4 GNM writers"},
    {"id": "index-type-decode", "source": "77fbd10ac7", "status": "done", "paths": ["core/libs/prx/libSceAgcDriver/Execution/src/Pm4.cpp"], "note": "INDEX_TYPE decode fix"},
    {"id": "ngs2-ps4-rack", "source": null, "status": "done", "paths": ["core/libs/prx/libSceNgs2.native"], "note": "NGS2 PS4 rack layout"},
    {"id": "ps4-audio-buzz", "source": null, "status": "open", "paths": ["core/libs/prx/libSceNgs2.native", "core/libs/prx/libSceAudioOut/src/AudioOut.cpp"], "note": "PS4 audio buzz: trace NGS2 output format/rate and AudioOut port handle"}
  ]
}
```
