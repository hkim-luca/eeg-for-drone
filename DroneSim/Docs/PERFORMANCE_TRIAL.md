# Reversible Environment Performance Trial — 2026-09-05

## Scope and state

UE 5.7.4 CL 51494982, `/Game/Maps/Daejeon_PCG_Work`. The accepted river geometry and 3,475,293 saved vegetation instances are preserved. This is a user-evaluated rendering trial, not a claim of measured optimization or packaged readiness. Requested items 2–4 have been applied within the safe scope below; item 5 was investigated but not deployed. FPS limiting and spatial loading/unloading were not requested and were not introduced.

| Item | Pre-trial | Active trial | Tradeoff |
|---|---|---|---|
| Shadow, GI, reflection, post-process | Epic / 3 | High / 2 | Lighting, reflection and shadow appearance may differ |
| Special-region grass end-cull | 2200 m | 1000 m; start-cull metadata 800 m | Less distant grass; moving-camera transition needs user review |
| All tree WPO disable | 1000 m | 300 m | Distant foliage stops wind/WPO animation |
| Tree shadows | Existing cast flags | Same flags; High scalability and shorter WPO range | No blanket removal of near-tree shadows |
| River LOD / distant vegetation proxies | Existing production assets | Unchanged | Seam/contact safety not established for proposed LOD |

Texture quality, foliage quality, resolution, t.MaxFPS=0 and hardware ray tracing remain unchanged. The four quality groups are native, shared **UE 5.7 Editor user settings**, persisted in `C:/Users/kdyde/AppData/Local/UnrealEngine/5.7/Saved/Config/WindowsEditor/EditorSettings.ini`. They can affect other UE 5.7 projects on this account. Source Config and the .uproject were not edited, so this is not a packaged-build quality policy. Quality groups were changed together for this trial; individual performance contributions were not benchmarked separately.

## Exact distance scope

318,202 grass instances retain their positions/density but render over a shorter distance:

| Region | Lower-layer actor | Affected grass instances |
|---|---|---:|
| X02_Y01 | PCG_ENV_RiparianLowerLayer_Y01_X02 | 58,369 |
| X03_Y01 | PCG_ENV_RiparianLowerLayer_Y01_X03 | 82,362 |
| X04_Y04 | PCG_ENV_RiparianLowerLayer_Y04_X04 | 102,075 |
| X07_Y05 | PCG_ENV_RiparianLowerLayer_Y05_X07 | 75,396 |

Other grass keeps its existing 500/700 m end-cull and 250 m WPO cutoff. A start-cull value alone does not guarantee a smooth material fade; no grass materials were altered. Main trees retain 5 km end-cull, young alder retains 1 km. The WPO change covers 475,062 forest trees, 35,442 legacy riparian trees and 6,171 young alder (516,675 total). Shadow, collision, ray-tracing participation and material assignments remain unchanged.

The seven active graph spawner descriptors are synchronized with the saved component policies, without regenerating PCG:

- PCG_ENV_ForestHighSuitabilityOpenWater
- PCG_ENV_ForestHighSuitabilityProduction
- PCG_ENV_RiparianRiverBankDenseY00X02
- PCG_ENV_RiparianRiverBankProduction
- PCG_ENV_RiparianTrees
- PCG_ENV_RiparianUnifiedBank
- PCG_ENV_RiparianUnifiedBankDenseX02

All are in `/Game/Environment/PCG/Graphs/`. A total of 616 saved ISM component policies changed. No actors, instances, meshes, masks, textures or materials were deleted or regenerated.

## Why river LOD was withheld

A native, unsaved in-memory reduction trial on X07_Y05 reduced 71,232 triangles to 35,615. Boundary edges changed from 4,440 to 3,505, with 1,237 new/changed boundary coordinate vertices. The largest distance in 32 sampled candidate boundary points was 0.8292 cm to the original boundary segments, exceeding the requested 0.5 cm MaxDeviation. This is a sample, not a full-mesh maximum or proof of a visible crack. The installed QuadricMeshReduction implementation does not pass that property as a hard geometric stopping bound to Simplify. It therefore did not establish the boundary/contact guarantee needed for these previously sensitive river meshes. Nothing from the probe was saved. All 27 river assets and 2,746,176 LOD0 triangles remain unchanged. No new HLOD/proxy or Nanite conversion was introduced.

## Backup and guarded rollback

Trial evidence root:
`C:/Users/kdyde/Documents/Codex/2026-08-08/ue-5-7-4-pcg-agents-2/work/performance_trial_20260905/`

The `baseline/` directory and `baseline_manifest.json` are the **exact pre-trial restore point**, not the older pre-commit-cleanup backup. 137 files / 1,110,800,677 bytes were copied and SHA-256 verified: the working map, Environment content, project Config, Docs, .uproject and project Editor ini files. The four original Epic values are also backed up in `global_EditorSettings_before.ini`.

The adjacent `../performance_trial_restore.ps1` defaults to a read-only dry run. With the Editor closed:

```powershell
& 'C:/Users/kdyde/Documents/Codex/2026-08-08/ue-5-7-4-pcg-agents-2/work/performance_trial_restore.ps1'
# Only when rollback is requested:
& 'C:/Users/kdyde/Documents/Codex/2026-08-08/ue-5-7-4-pcg-agents-2/work/performance_trial_restore.ps1' -Restore
```

Rollback restores only files listed in `changed_files.json`, plus the four original Editor quality values. It verifies baseline/current hashes, refuses to overwrite newer user edits, and copies the trial files into a recovery directory before restoring. This new trial document is removed on rollback only after making that recovery copy. Other Editor preferences are preserved. If the user makes newer edits, inspect and back those up before adapting the rollback; do not bypass the guard blindly. The full destructive restore was not executed merely to test it; only its preflight/dry run is used in this trial.

## Validation and manual acceptance

- Native before/apply/fresh-load snapshots: 192 actors and all 3,475,293 instance transform hashes match. Graph descriptors persist. No PCG regeneration occurred.
- Native Map Check: zero errors, zero warnings for the working map. Pre-existing incompatible Huckleberry Oak supplied-library custom-version messages remain unresolved outside this scope.
- Five fixed-view before/after render pairs: X07_Y05 close, X04_Y04 near/mid/far, X02_Y01 near, each 1440 x 810. The baseline properties were replayed in memory for the before shots without saving. Near placement/grass patterns were preserved; far grass was reduced. No new river holes/breaks were identified in these views. Dynamic water/sky differ and this is not exhaustive fly-through or transition validation.
- `changed_files.json` records the exact changed-file hashes and protected inventory check. No Git staging/commit was performed. Supplied libraries, original map, source/config/plugins and SourceData are not optimization targets.
- No controlled frame-time, RAM or VRAM reduction has yet been measured. All saved instances remain loaded; these changes primarily reduce rendering work, not base scene residency. Uncapped rendering can still keep GPU utilization high. Compare FPS/frame time at the same resolution, camera, quality state and elapsed warm-up time rather than utilization percent alone.
- Manually inspect X04_Y04 and X02_Y01 up close and while moving out through 800–1000 m; inspect tree wind transition around 300 m and river reflections/shadows. Repeat a fixed travel path after warm-up, checking `stat fps` and `stat unit` plus RAM/VRAM. Report visible popping or unacceptable loss of distant bank coverage before reducing distances further.
- Cook, packaged standalone, target-PC performance and offline packaged behavior are not validated by this Editor trial.

## Files changed

Working map, the seven PCG graphs above, PCG_ARCHITECTURE.md, PCG_PARAMETERS.md, PCG_TEST_REPORT.md, this document, and four native Editor user quality values. River and vegetation asset files, masks, source Config and .uproject are unchanged. External backup/inspection/restore scripts and evidence remain outside the project.
