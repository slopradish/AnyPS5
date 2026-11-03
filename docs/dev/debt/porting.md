# Porting tracker — fixes lost during the 2026-11-03 sync

Every entry below is a fix present in a fork or pre-sync branch but **not** in `wolverine-bringup` (`e3b3b01776`). On the next rebase/merge, check each one.

## Rendering (Astro Bot + Wolverine)

| # | Fix | Source | Key files | Status |
|---|-----|--------|-----------|--------|
| 1 | CMASK fast clears on colour targets | dean/astrobot `812c200ebe` + legacy | `Graphics/src/Draw.cpp`, `Graphics/src/State.cpp`, `Graphics/src/TextureDetiler.cpp`, `Graphics/shaders/CmaskClear.comp` | Partial (TextureDetiler missing `cmaskErrors`, `cmaskDescriptorPools`) |
| 2 | Sampled texture soft limit to half device budget | dean/astrobot `023ef90838` | `Graphics/src/Texture.cpp` (or sampled texture budget code) | Not ported |
| 3 | Charge cached sampled textures actual bytes | dean/astrobot `f3c934082e` | `Graphics/src/Texture.cpp` | Not ported |
| 4 | Release sampled texture host copies under host pressure | dean/astrobot `93faece10b` | `Graphics/src/Texture.cpp` or `ShaderMemory.cpp` | Not ported |
| 5 | Bind misaligned draw buffer regions in place | dean/astrobot `f83abf2a7e` | `Graphics/src/Draw.cpp` | Not ported |
| 6 | Skip zero-filling sampled texture snapshots | dean/astrobot `fef87ce8f6` | `Graphics/src/ShaderResources.cpp` | **Ported** (`6df1db86db`) |
| 7 | Guard wait-timeout set against concurrent queues | dean/astrobot `3cb2fc3694` | `Execution/src/Driver/Synchronization/WaitMemory.cpp` | Already in tree |
| 8 | Prove storage image refreshes over other pending images | sp4c3b4r8 `88d7bb6211` | `Graphics/src/Texture.cpp` or `GuestTextureResource.cpp` | Not ported |

## GPU hang (Wolverine)

| # | Fix | Source | Key files | Status |
|---|-----|--------|-----------|--------|
| 9 | First-use shader/texture work off queue-0 draw path | biladeiro/wolverine-main `0b4545d8b3` | `Graphics/src/Draw.cpp`, `Execution/src/Driver/Shaders/ShaderRegistry.cpp`, `Execution/src/ShaderMemory.cpp`, `Graphics/src/Recorder.cpp`, `Recompiler.cpp/hpp` | Not ported (BindlessHeapTable deleted) |
| 10 | Skip-listed dispatches return early instead of throwing | biladeiro/wolverine-main `a15541ace1` | `Graphics/src/Draw.cpp` or dispatch path | Not ported |
| 11 | Batch cap 256 recorded draws/dispatches (was 64) | biladeiro/wolverine-main `0e47407146` | `Graphics/src/Recorder.cpp` | Not ported |
| 12 | Fewer black frames/hitches from first-use work | biladeiro/wolverine-main `8508ebf053` | `Graphics/src/Draw.cpp` | Not ported |

## Shader (Potter PS5)

| # | Fix | Source | Key files | Status |
|---|-----|--------|-----------|--------|
| 13 | Null T# binding as null image (TYPE 0/1) | upstream #5828 | `ResourceMaterializer.cpp`, `DescriptorBindingBuilder.cpp`, `ShaderResources.cpp`, `GuestTextureResource.cpp` | Unknown |
| 14 | Cube T# through DIM 2D as face 0 | upstream #5509 | `SpirvImageEmitter.cpp` or `ResourceMaterializer.cpp` | Unknown |
| 15 | Prepared mode for compute shader descriptor format 20 (class 2, dim 1) | New issue | `ResourceMaterializer.cpp:TryRuntimeImageMode` | Not started |

## PS4

| # | Fix | Source | Key files | Status |
|---|-----|--------|-----------|--------|
| 16 | PS4 GNM writers (already done) | Our own `4eda89a099` | `libSceGnmDriver/Export.cpp` | Done |
| 17 | INDEX_TYPE decode fix (already done) | Our own `77fbd10ac7` | `libSceGnmDriver/Export.cpp` | Done |
| 18 | NGS2 PS4 rack layout | Our own (refactored) | `libSceNgs2.native` | Done |
| 19 | PS4 audio buzz | TBD | NGS2 → AudioOut | Not started |

## Pinning for rebase

On the next `up/main` sync:
1. Run `git log --oneline wolverine-bringup-legacy --not wolverine-bringup` to find unported legacy fixes
2. Run `git log --oneline biladeiro/wolverine-main --not wolverine-bringup` to find unported biladeiro fixes
3. Run `git log --oneline dean/astrobot --not wolverine-bringup` to find unported dean fixes
4. Run `git log --oneline sp4c3b4r8/astrobot-windows --not wolverine-bringup` to find unported sp4c3b4r8 fixes
5. Check each commit against the current tree using the line-presence audit method
6. Port any that are missing
path: C:\Users\Meta\Documents\GitHub\AnyPS5\docs\dev\debt\porting.md