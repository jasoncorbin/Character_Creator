# Task — UE 5.7.4 → 5.8.1 upgrade (real project)

**Validated on a full copy first** (`E:\UE5 Projects\_58Trial\Character_Creator`) — build, editor
launch, asset integrity and PIE all green before anything here was touched.

**Rollback point:** commit `f6c017d "Done 2.0"`, working tree clean at start.

---

## 1. Trial results — what we already know works on 5.8.1

| Check | Result |
|---|---|
| `Character_Creator` C++ module builds | ✅ clean, first try |
| `MCPUnreal` plugin builds | ✅ `UnrealEditor-MCPUnreal.dll` linked |
| MCPUnreal runs live on 5.8.1 | ✅ drove the whole verification session |
| All 8 C++ classes registered | ✅ |
| `BP_RPG_PlayerCharacter` → `ARPGPlayerCharacter` reparent | ✅ survived |
| `BPC_PlayerStats` Branch/IsValid rewire | ✅ topology intact, no `Select` nodes |
| `LootDropper` + 5 configured properties | ✅ intact |
| `DA_Loot_Grunt` 4 entries | ✅ exact |
| PIE (melee kill, arrow kill, loot, pickup) | ✅ green |

Engine reported by the trial editor: `5.8.1-56057345+++UE5+Release-5.8`.

---

## 2. The ONE required code change

`DefaultBuildSettings` must go **V6 → V7** in **both** target files. This is not optional —
5.8 hard-rejects V6 for a target that shares build products with the installed engine:

```
Exception while creating build target for Character_CreatorEditor:
  modifies the values of properties:
  [ UnreachableCodeWarningLevel: Off != Error,
    ReturnTypeWarningLevel:      Off != Error,
    DanglingWarningLevel:        Off != Error ]
```

Per `UnrealBuildTool/Configuration/Rules/TargetRules.cs`, **V7 = the UE 5.8 defaults**:

| Setting | V6 | V7 |
|---|---|---|
| `ReturnTypeWarningLevel` | Off | **Error** |
| `DanglingWarningLevel` | Off | **Error** |
| `UnreachableCodeWarningLevel` | Off | **Error** |
| `FPSemantics` | Imprecise | **Precise** (Editor/Program on MSVC only; Game/Client/Server unchanged) |

Our code compiles clean with all three as errors — verified in the trial.

Files: `Source/Character_Creator.Target.cs`, `Source/Character_CreatorEditor.Target.cs`.

---

## 3. Steps

### Reversible (done by Claude — pure text + build artefacts)
1. `DefaultBuildSettings = BuildSettingsVersion.V7` in both `.Target.cs`
2. `"EngineAssociation": "5.8"` in `Character_Creator.uproject`
3. Regenerate project files with the **5.8** UBT (files were added since last regen)
4. Build `Character_CreatorEditor Win64 Development` with the **5.8** toolchain

Undo for all of the above:
```
git checkout -- Character_Creator.uproject Source/Character_Creator.Target.cs Source/Character_CreatorEditor.Target.cs
```

### ⚠ NOT reversible (done by the user)
5. **Open the project in the 5.8 editor.** This is the point of no return — UE upgrades and
   resaves assets on load/save, and **assets resaved by 5.8 cannot be opened by 5.7 again.**
   Git is the only way back after this, and only for committed assets.

Close the trial editor first (PID was 9564) so the two don't fight over ports 8090/8000.

---

## ✅ UPGRADE COMPLETE — 2026-08-02

Real project is live on **5.8.1-56057345+++UE5+Release-5.8**. Verified after the first 5.8 load:

| Check | Result |
|---|---|
| All 8 C++ classes registered | ✅ |
| `BP_RPG_PlayerCharacter` → `RPGPlayerCharacter` | ✅ (194-node EventGraph, ApplyStance 33) |
| `BPC_PlayerStats` `Decrease Health` | ✅ 38 nodes, matches the trial |
| `DA_Loot_Grunt` 4 entries | ✅ exact |
| `LootDropper` + 4 asset refs | ✅ all resolved |
| `RollLoot()` re-verified on 5.8 (500 rolls) | ✅ 0.796 / 0.294 / 0.050 / 0.266 |
| Epic MCP (8000) + MCPUnreal (8090) | ✅ both listening, same process |

**⚠ Verification gotcha worth remembering:** the first probe right after the editor opened reported
`DA_Loot_Grunt` unloadable and `LootDropper` missing. **Both were false** — the asset registry was
still scanning. `EditorAssetLibrary.load_asset` returns `None` for a package the registry hasn't
discovered yet, with no error. Check `AssetRegistry.is_loading_assets()` before trusting a negative
result after an editor start, and prefer `unreal.load_asset` over `EditorAssetLibrary.load_asset`.

### The GameFeatureData warning (asked about 2026-08-02)
> *"Asset Manager settings do not include an entry for assets of type GameFeatureData…"*

**Unrelated to the upgrade.** Chain is `AllToolsets` → `GameFeaturesToolset` → GameFeatures plugin,
which wants a `PrimaryAssetTypesToScan` entry. Harmless — this project uses no Game Feature Plugins.
Accept it (one line into `Config/DefaultGame.ini`) or ignore it.

**Related, and actually useful:** `Config/DefaultGame.ini` has **no `PrimaryAssetTypesToScan`
entries at all**, yet `UItemData` and `ULootTable` are `UPrimaryDataAsset` with custom
`GetPrimaryAssetId()` overrides. The IDs are authored but nothing scans for them, so items cannot be
loaded by ID at runtime. Irrelevant today (everything uses hard refs), but it blocks any save system
that wants to persist `"ohs03_sword"` instead of an object pointer — see step 7.

---

## 4. Post-upgrade verification

- [ ] Editor opens, no missing-asset or failed-compile spam in the log
- [ ] `BP_RPG_PlayerCharacter` still parents to `RPGPlayerCharacter`
- [ ] PIE: move, attack, stance switch (Q), bow charge/release, dodge, target lock
- [ ] PIE: kill an enemy melee **and** with an arrow → loot drops, materials visible as spheres
- [ ] Pick loot up → enters inventory
- [ ] Zero `Accessed None` from `BPC_PlayerStats`
- [ ] `git status` — review the (large) set of resaved assets before committing

---

## 5. Rollback

**Before opening the 5.8 editor:** `git checkout --` the three files above. Nothing else changed.

**After opening the 5.8 editor:**
```
git reset --hard f6c017d
git clean -fd            # review with -n first
```
Then delete `Intermediate/`, `Binaries/`, `DerivedDataCache/` and rebuild against 5.7.
Anything not committed at `f6c017d` is gone — the tree was clean, so nothing is at risk.

---

## 6. MCP situation after the upgrade

Both servers coexist in one editor process — **Epic's on 8000, MCPUnreal on 8090** — verified in
the trial. Keep both:

- **Epic's `ModelContextProtocol`** is far richer for authoring: `BlueprintTools` (53 tools,
  including a graph DSL, configured `create_node`, `set_node_position`, event dispatchers, typed
  variables) and `UMGToolSet` (23 tools). **This largely obsoletes the plan's biggest step-6 risk
  note** — that `WidgetTree` isn't scriptable and the Inventory screen must be hand-authored.
- **MCPUnreal keeps one thing Epic's does not have: arbitrary `unreal` Python.** Epic's
  `ProgrammaticToolset` is a sandbox limited to `json, math, datetime, copy, re, time` — it
  orchestrates registered tools only. Its `DataAssetTools` has exactly one tool (`create`), so
  data-asset population, CDO reads, `SubobjectDataSubsystem` component work and disk-level save
  verification all still need MCPUnreal.

To enable Epic's on the real project: add `ModelContextProtocol` + `AllToolsets` to the
`.uproject` plugins, restart the editor, then *Editor Preferences > General > Model Context
Protocol > Auto Start Server* (or console `ModelContextProtocol.StartServer`). The `unreal-mcp`
HTTP entry already exists in `.mcp.json`; **Claude Code must be restarted** for it to attach.
