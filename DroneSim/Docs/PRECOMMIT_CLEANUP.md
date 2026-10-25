# Pre-commit Environment Cleanup

Date: 2026-09-04. UE 5.7.4 CL 51494982. Map: `/Game/Maps/Daejeon_PCG_Work`.

## Result and scope

The accepted V72 river geometry, UVs, flow data and vegetation placement were retained. Only our untracked PCG/river work, working map and documentation were edited. Existing Git-tracked modifications were preserved exactly as found. No staging or commit was performed.

| Operation | Result |
|---|---:|
| Unused native assets deleted after reference checks | 57 |
| Breakdown | 29 obsolete river meshes, 15 textures, 9 graphs, 3 material instances, 1 material |
| Active assets moved/renamed using Unreal AssetTools | 87 |
| Redirectors removed after owned references were saved | 53 |
| Obsolete raw mask PNGs removed | 7 |
| Active PNGs renamed without changing bytes | 2 |
| Superseded standalone version documents removed | 5; backed up, current docs contain consolidated guidance/history |
| Empty PCG/River directories removed | 44 |
| Remaining Environment assets | 118 |
| Deleted obsolete native-asset bytes | 40,949,436; not a measured runtime-memory saving |

No Actor or PCGVolume was deleted: all 192 current actors have a role. In particular, `PCG_EXCL_Manual_Central_01` is referenced through `PCG_Exclude_Vegetation`; the four empty riparian production cells remain part of the deterministic grid. `OpenWaterPrototype` was an active regional variant, so it was renamed to `OpenWater`, not discarded. Regional density variants and the seam-safe shared flow atlas remain separate functional assets.

## Canonical organization

```text
Content/Environment/
  PCG/
    Graphs/
    Subgraphs/
    Data/Mask/
  River/
    Production/Meshes/
    Materials/Instances/Flow/
    Textures/Production/Flow/
```

Current mesh names are `SM_ENV_RiverSurface_Daejeon_X##_Y##`. There are no remaining version/prototype names in active PCG/River asset paths. Six PCG mask assets and three retained source PNGs remain. The two renamed PNGs have valid updated import metadata; texture data was not reimported or resampled.

### Follow-up audit: Content/Environment/PCG

After the full-map validation, a separate fresh-process, read-only audit checked every file in this folder. Within PCG alone, cleanup had removed 22 obsolete native assets (9 graphs/subgraphs and 13 masks) and renamed 20 active assets.

| Remaining folder | Native assets | Other files |
|---|---:|---:|
| Graphs | 7 production graphs, used by 155 volumes | 0 |
| Subgraphs | 13, reachable from the production graphs | 0 |
| Data/Mask | 6, reachable from the production graphs | 3 PNG import sources |

All 26 assets are reachable from the current live map's seven assigned production graphs. All 29 files are accounted for; no unused asset, redirector, unexpected file type or empty directory was found. All six mask import sources exist; the three in-project PNGs are actual current import sources, not redundant runtime asset copies. Functional density and water-region variants are retained to preserve the accepted output. This is an asset-level dependency audit, not an attempt to rewrite working graph internals. Evidence: `precommit_pcg_folder_audit.json`.

External `work/` generation scripts, versioned source exports and recovery records were deliberately retained outside the project. Some are still texture reimport sources or contain the reproducible authoring history. Historical scripts refer to former package names and are not safe to reapply against canonical assets without adapting their paths. This cleanup does not introduce a new full regeneration pipeline.

## Narrow performance correction

The former V55 distance-optimization script handled forest and lower-layer volumes but omitted the separate legacy riparian-tree group. Its 35,442 trees had neither finite end-culling nor a WPO distance cutoff.

- Updated only `PCG_ENV_RiparianTrees`'s four spawner descriptors and 237 saved ISM components in the working map.
- Matched existing forest distances: start-cull metadata 350,000 cm; end-cull 500,000 cm; WPO disable 100,000 cm.
- Tradeoff: riparian trees stop rendering beyond 5 km, and leaf motion is disabled beyond 1 km. Start-cull metadata does not alone prove a gradual material fade.
- No regeneration, instance deletion, density reduction, transform, material, collision or shadow changes.
- The deliberate 2200 m grass culling in four unified-bank regional variants is preserved; other grass retains 500/700 m and WPO cutoff 250 m.
- Inspected vegetation is NoCollision with no overlap events; inspected mesh-component ticks are disabled. All 155 production PCG components are GenerateOnDemand/non-partitioned.

The number of vegetation instances lacking a finite end-cull setting fell from 35,442 to 0. This is a verified policy improvement, not an FPS gain measurement or proof of globally optimal performance.

## Final verification

- Fresh native map load: 192 actors, 27 rivers, 2,746,176 river triangles.
- All 27 native river OBJ geometry/UV/normal payload hashes exactly match the accepted baseline.
- Flow texture export hashes and material parameter values match after path normalization.
- All surviving Environment dependency relationships match after rename mapping; no missing/unexpected Environment packages.
- PCG actor-by-actor counts match accepted output: forest 475,062; legacy riparian trees 35,442; lower layer 2,964,789; total 3,475,293.
- All 35,442 optimized tree world transforms were hashed before/after and after reload: identical.
- Fresh readback confirms all four graph descriptors and 237 component policies. Other inspected component rendering properties are unchanged.
- Five rendered QA views: two X07/Y05 close-ups, one X07/Y05 overview and the two added inlets. No new visible holes/disconnections in those views. Animated water/sky prevent treating screenshots as pixel-equality checks.
- Native Map Check: 0 errors, 0 warnings.
- 526 tracked-file hashes, Git HEAD and index are unchanged. The three previously modified tracked files remain modified exactly as before this cleanup.
- 5,198 protected external file paths, sizes and modification times unchanged; 25 preserved Environment Vegetation/Rocks imports also match backup SHA-256.

The earlier contact trace was not repeated: exact LOD0 geometry equality preserves the basis of the accepted result (17,970,194 sampled entries, minimum 7.9986778103 cm, none below 7.99 cm, shared-edge delta 0 cm). This does not prove every possible Landscape LOD/camera configuration.

## Backup and evidence

Verified full pre-cleanup backup: 195 files, 1,151,515,354 bytes.

`C:/Users/kdyde/Documents/Codex/2026-08-08/ue-5-7-4-pcg-agents-2/work/precommit_cleanup_backup/`

`manifest.json` contains original hashes and protection baselines. `optimization_manifest.json` contains the additional pre-optimization canonical map/graph backup. Deleted items can be recovered from these backups. Restore only with Unreal closed, keeping the map and matching Environment references together; do not mix old masks and new mesh versions.

Adjacent evidence: `precommit_plan.json` (exact old/new/deleted asset paths), `precommit_cleanup_result.json`, `precommit_files_result.json`, `precommit_validation.json`, `precommit_tree_distance.json`, `precommit_final_native.json`, `precommit_final_files.json`, and `river_v72_capture_final/`.

## Remaining checks before distribution

No Cook/Package/Standalone/offline packaged test or representative drone-flight FPS/CPU/GPU/VRAM benchmark was performed in this cleanup. Existing Huckleberry Oak library assets still emit newer-engine custom-version errors during registry scan; original libraries were protected, so this issue was not changed. Native Map Check passing does not negate those separate library/cook risks.

The most useful next validation is a fixed drone flight route measured before further visual reductions, followed by packaging/offline validation. Avoid blaming all viewport stalls on grass count without profiling streaming, shader compilation, GPU and memory behavior.

The existing Git ignore rules ignore `Docs/`. They were not modified. Review how to include the intended current documentation when preparing the commit; avoid staging unrelated imported libraries or existing configuration changes indiscriminately.
