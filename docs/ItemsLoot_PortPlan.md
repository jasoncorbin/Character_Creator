# Items / Loot / Inventory UI — UE5 Port Plan

**Status:** approved 2026-08-01 — all recommendations taken, to be revisited during testing.
**Steps 1–5 are DONE, in C++, and all are PIE-verified (step 5 completed 2026-08-02).**

**⛔ NEXT IS NOT STEP 6.** The locked decision is that the **UE 5.8 upgrade happens after step 5,
before step 6** — step 6 is the XL Inventory/Character screen and is far better started on the
engine we intend to finish on than migrated mid-build. Two prerequisites before the upgrade, both
on a COPY first: `Plugins/MCPUnreal` must be proven to rebuild against 5.8, and the
`Character_Creator` C++ module must build clean on 5.8. 5.8.1 is already installed at
`E:\UE4 Projects\_UE4\UE_5.8`; this project is on 5.7.4.

**Decisions locked:** instance model front-loaded to step 2 · ~~hero preview = separate always-idle instance~~ **hero preview = LIVE PLAYER MODEL (user, 2026-08-02 — overrides §7's recommendation)** · palette = **Candy Warm** · author ~8 items by hand now · take the arrow-damage-from-item win in step 4 · modular-character direction stays out of scope · **fonts imported before step-6 layout work (user, 2026-08-02 — DONE)**.
**Source spec:** `docs/ItemsLootUI_MechanicsSpec_ForUE5.md` (behaviour + values — the *what*).
**This doc:** the *how* for this project specifically — asset names, integration points, order, risks.
**Grounded against:** live `BP_RPG_PlayerCharacter` / `BP_RPG_Enemy` graphs, `RPG_BuildProgress.md`, and the Unity source at `E:\Unity\Unity_Procedural_Level_Creator\`.

---

## 1. Ground truth — what we're building onto

Facts verified in this project that change the plan versus a naive reading of the spec:

| Fact | Consequence |
|---|---|
| **No C++ module.** `.uproject` declares zero Modules; only the editor-only `MCPUnreal` plugin has code. | Everything is Blueprint. See §2.1. |
| **`Content/RPG/` is the live track**; `Content/CharacterCreator/` is a frozen archive (`.claude/handoff.md`: "don't edit unless explicitly asked"). | All new assets go under `Content/RPG/`. Never extend the CC track. |
| **Zero existing data infrastructure** — no DataTables, no DataAssets, no user structs. One enum (`E_RPG_Stance`), kept only as an index legend. | Greenfield. No migration, nothing to conflict with. |
| **`ApplyStance` reads hard-coded 8-entry arrays** on the CDO (`StanceRightMeshes`, `StanceLeftMeshes`, `StanceRightRotations`, `StanceLeftRotations`, `StanceIsRanged`). Adding a stance today = fill index N, no graph edits. | The item system must *feed* this, not replace it. See §5.4. |
| **The player skeleton has only `Weapon_R`, `Weapon_L`, `Head`, `BackPack` sockets.** There is **no `Shield` socket and no `Bow` socket.** | Spec §6a's "off-hand socket" = the `Weapon_L` StaticMeshComponent. The pack's Shield/Bow rotations in memory belong to `ModularCharacter_BP` (a display prop), not the player. |
| **Bows are SkeletalMesh** on a separate `Bow` component (visibility-toggled on `CurrentStance == 7`); swords/shields/wands are StaticMesh on `Weapon_R`/`Weapon_L`. | An item needs an explicit **mount point**, not an inferred one. See §2.4. |
| **Melee damage is a hard-coded literal `20`** on the `ApplyDamage` node inside `MeleeHit`. No damage variable exists. | Single, clean replacement point. |
| **`BP_Arrow` already has a `Damage` float variable = 30.** | Item-driven ranged damage is nearly free — spec §13 lists it as deferred, we can close it cheaply. |
| **`BP_RPG_Enemy` has NO death event or dispatcher.** Death is inline: `AnyDamage → Decrease Health → Branch(IsPlayerDead) → PlayMontage → Delay 2.0 → DestroyActor`. | We must add an `OnDeath` dispatcher. There is a **2-second window** before the corpse is destroyed — ample. |
| **`BPC_PlayerStats` (the health component) is CC-archive** and has a known `Accessed None` bug the user said not to touch. | Put the dispatcher on `BP_RPG_Enemy`, **not** on the shared stats component. |
| **No `IA_Interact` / `IA_Inventory` exist** in `Content/RPG/Input/`. | Two new Input Actions + `IMC_RPG_Default` edits. Manual (MCP can't set the InputAction pin). |
| ~~**UMG `WidgetTree` is not Python-exposed in 5.7.**~~ **STALE — CORRECTED 2026-08-02.** True of `unreal.WidgetTree` in Python, but the project is on **5.8.1** and Epic's in-editor MCP ships **`UMGToolSet` (23 tools)** — `CreateWidgetBlueprint`, `AddWidget`, `MoveWidget`, `WrapWidgets`, `SetNamedSlotContent`, `BindToEventProperty`, `CompileWidgetBlueprint`… **Enumerated and smoke-tested live**, not assumed. | Widget layout is **largely scriptable**. Steps 6–7 must be **re-scoped before estimating** — the old sizing assumed 100% designer hand-authoring. See handoff § "MCP: run BOTH servers" + memory `epic-mcp-gateway.md`. |
| **Fredoka and Nunito are not imported.** Only `Nanum_Myeongjo` exists (CC archive). | Font acquisition + import is a real task, blocking accurate UI. |
| **The 4 design reference PNGs live only in the Unity repo** and render the **Classic Bright** palette, while **Candy Warm is the chosen one**. | Copy them in as reference, but expect rarity colours to disagree with the target. |

---

## 2. Decisions to make before writing anything

### 2.1 Blueprint-only, no C++ module — ~~recommended~~ **SUPERSEDED 2026-08-01, see §2.6**

> The reasoning below was sound on the information available, but the information was
> incomplete. It assumed Blueprint could express everything the spec needed. It can express
> the *data*, but not the *logic* — Python cannot author graph nodes, and steps 2/4/5/7 are
> mostly logic. Kept for the record; **§2.6 is the live decision.**

The spec is written in C++ vocabulary (`UPrimaryDataAsset`, `DYNAMIC_MULTICAST`, `TMap<UItemData*, int32>`). Every one of those has a Blueprint equivalent that is fully capable here:

| Spec asks for | Blueprint answer |
|---|---|
| `UPrimaryDataAsset` subclass | Blueprint Class with parent `PrimaryDataAsset`; instances via Content Browser → Miscellaneous → Data Asset |
| `USTRUCT` | User-Defined Struct |
| `UENUM` | User-Defined Enum |
| `TMap<UItemData*, int32>` | BP Map with an object-reference key — works |
| `DYNAMIC_MULTICAST_DELEGATE` | Event Dispatcher (multi-param supported) |
| Runtime weapon instances (§8) | Blueprint Class with parent `Object` + `Construct Object from Class` |
| `IInteractable` | Blueprint Interface |

**Why not add C++:** the project has been BP-only for its entire life, the MCP tooling this project relies on is BP-oriented, and a new module means a toolchain dependency and compile cycles for what is ultimately data plumbing. The one thing C++ would genuinely buy is cleaner save/load later — and that isn't in scope.

**Live with this caveat:** UE has **no Int → User-Defined-Enum conversion node** (this is exactly why `CurrentStance` is an int). So: never round-trip a rarity/slot/category through an int. Keep them as enums end-to-end, and where you'd reach for an int-indexed array keyed by an enum, use a **Map keyed by the enum** instead. Enum → Int works fine; the reverse does not.

### 2.2 DataAssets, not DataTables — **recommended**

Both are viable. DataAsset wins because **object identity is load-bearing**: Unity's material stack map is `Dictionary<ItemData, int>` keyed by the asset reference itself, the equipped map is keyed by asset, and §8's weapon instances need to point at a template. A DataTable forces every one of those to become a string/RowName lookup.

Cost: one asset per item (Unity has 57 weapons + 3 materials). Mitigated — these are scriptable via MCP `data_asset_ops` / `execute_script` once the class exists.

Use plain hard references from loot tables and the item catalogue; **do not** register a PrimaryAssetType in `DefaultGame.ini` unless we later want async catalogue scanning. Not needed for anything in scope.

### 2.3 Build the weapon-instance layer at step 2, not step 7 — **recommended, deviates from the spec's order**

Spec §8 warns: *"This is the one change that ripples: the inventory's gear list must hold instances, not shared templates, and every UI read of `Damage`/`Rarity` must come from the instance."* Spec §12 nonetheless puts it last.

Doing it last means rewriting the inventory arrays, the equip map, every UI cell, the detail strip, the mount code and the damage code. Doing it at step 2 costs a thin `BP_RPG_ItemInstance` object (Template + Level) plus two accessor functions — perhaps half an hour.

**Plan:** the gear list holds **instances** from step 2 onward. `GetDamage()` / `GetRarity()` on the instance simply forward to the template until step 7, at which point they start computing from `Level`. Nothing above the instance changes. Materials stay template-keyed stacks, as Unity has them.

*(This is the one place I'd push back on the spec's ordering. Veto-able — if you'd rather follow §12 literally, say so and I'll thread templates through and eat the refactor later.)*

### 2.6 C++ migration — **the live decision (2026-08-01)**

**What forced it.** Probing established exactly what the scripting layer can and cannot do:

| Python **can** | Python **cannot** |
|---|---|
| DataAsset classes, instances, property read/write/save | Create or populate User-Defined **Enums** / **Structs** (no API; `FEnumEditorUtils` is C++-only) |
| Variables: primitives, object refs, class refs, engine structs, arrays, sets, maps | **Enum-typed** or **soft-ref** variables (`EdGraphPinType` fields aren't settable) |
| Create empty function graphs | Author **any graph node** — the actual logic |

That last row is decisive: steps 2, 4, 5 and 7 are predominantly logic, so Blueprint-only meant the user hand-wiring every function. C++ makes all of it text.

**Toolchain (verified present):** Visual Studio Community 2026 with C++ tools, and a full engine install with `Build.bat`, UnrealBuildTool and `Engine/Source`.

> ⚠ **The engine lives at `E:\UE4 Projects\_UE4\UE_5.7`**, not `C:\Program Files\Epic Games\`.
> The MCP `status` tool reports a stale Program Files path — **do not trust it**; that path does not exist.

**What was written** — `Source/Character_Creator/`, module declared in `.uproject`:

| File | Contents |
|---|---|
| `Items/RPGItemTypes.h` | `EItemRarity`, `EItemKind`, `EEquipSlot`, `EWeaponCategory`, `EMountPoint`, `EInteractPriority`, `FRarityColors` |
| `Items/ItemData.h/.cpp` | `UItemData : UPrimaryDataAsset` — soft refs for mesh/icon, both attach rotations, `GetAttachRotationForSlot()` |
| `Items/RarityPalette.h/.cpp` | `URarityPalette` — both palettes as `TMap<EItemRarity, FRarityColors>`, sRGB→linear in code, `ActiveSet` one-field swap |
| `Items/ItemInstance.h/.cpp` | `UItemInstance : UObject` — `GetDamage() = BaseDamage + Level × DamagePerLevel` |
| `Inventory/InventoryComponent.h/.cpp` | `UInventoryComponent` — full §4 API + 3 delegates |
| module + 2 target files | `Character_Creator.Build.cs`, `Character_Creator.Target.cs`, `Character_CreatorEditor.Target.cs` |

**Build (editor MUST be closed — a first module build cannot link while the editor holds the MCPUnreal plugin DLL):**

```powershell
$UE = "E:\UE4 Projects\_UE4\UE_5.7"
$P  = "E:\UE5 Projects\Character_Creator\Character_Creator.uproject"

# 1. regenerate project files
& "$UE\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="$P" -game -rocket -progress

# 2. build the editor target
& "$UE\Engine\Build\BatchFiles\Build.bat" Character_CreatorEditor Win64 Development -project="$P" -waitmutex
```

**BUILD IS GREEN as of 2026-08-01.** `UnrealEditor-Character_Creator.dll` links, all 14 files compile, UHT clean. Step 1 of the block above (project-file regeneration) is only needed when files are **added or removed** — a pure edit just needs step 2.

Four things had to be fixed to get there, all recorded with full reasoning in `.claude/handoff.md` § "First-build traps". The two that will bite again if anyone edits the build files:

- ☠ **`PublicIncludePaths.Add(ModuleDirectory)` in `Character_Creator.Build.cs` is load-bearing.** No Public/Private split + sources in `Items/` and `Inventory/` + `BuildSettingsVersion.V6` (which disables legacy include-path behaviour) means UBT will **not** put the module root on the include path by itself. Delete that line and every cross-folder `#include` dies with C1083.
- **Both target files must stay at `BuildSettingsVersion.V6`.** Lower versions change global compile settings the installed engine was not built with, and UBT rejects that outright for any target sharing build products with `UnrealEditor`.

Also: **UHT runs `-WarningsAsErrors`** here, so every `UENUM` in this module needs an entry at **0**.

**Next:** reopen the editor, regenerate the 9 items + palette against the C++ classes, and delete the superseded Blueprint assets — `PDA_RPG_Item`, `PDA_RPG_RarityPalette`, `DA_RarityPalette`, the 9 `DA_Item_*`, `BP_RPG_ItemInstance`, `BPC_RPG_Inventory` (which takes the stray `ProbeFunc` with it), and the 5 `E_RPG_*` enums.

**One design gap closed during the build pass:** `Equip()` derived its slot from `Template->Slot`, a single value, so a one-hand sword could never be equipped into `OffHand` — making step 4's stance rows 3 and 4 unreachable and `AttachRotationOffHand` dead weight. `EquipToSlot(Instance, Slot)` + `CanEquipToSlot()` added; the off-hand accepts `Shield` or `OHS`. `Unequip()` now refuses when the bag is full rather than destroying the item.

**What C++ buys beyond unblocking logic:** soft object refs as the spec originally wanted, a genuine `TMap<EEquipSlot, UItemInstance*>` instead of the index-array workaround, `OnEquipChanged(Slot, New, Old)`, and the permanent end of the enum/struct hand-off.

### 2.4 Items declare their mount point explicitly

Not covered by the spec, forced by this project: bows are skeletal, everything else is static, and a wand is ranged-but-right-handed. Inferring the mount from `Slot` would be wrong for wands.

```
E_RPG_MountPoint { RightHand, LeftHand, BowRig }
```
- `RightHand` / `LeftHand` → `SetStaticMesh` on `Weapon_R` / `Weapon_L` (+ that item's attach rotation)
- `BowRig` → drive the `Bow` SkeletalMeshComponent and its visibility

Likewise the item carries **both** a `StaticMesh` and a `SkeletalMesh` soft reference; only the one matching its mount point is used.

---

## 3. Asset inventory (naming follows existing project convention)

New folders under `Content/RPG/`: `Items/`, `Items/Assets/`, `Loot/`, `Interaction/`. Existing: `Data/`, `UI/`, `Input/`, `Blueprints/`, `Enemies/`.

| Asset | Kind | Path |
|---|---|---|
| `E_RPG_ItemRarity` | Enum — Common, Uncommon, Rare, Legendary | `RPG/Data/` |
| `E_RPG_ItemKind` | Enum — Gear, Material | `RPG/Data/` |
| `E_RPG_EquipSlot` | Enum — Melee, OffHand, Ranged, Armor | `RPG/Data/` |
| `E_RPG_WeaponCategory` | Enum — None, OHS, THS, Spear, Shield, Bow, Wand, Arrows | `RPG/Data/` |
| `E_RPG_MountPoint` | Enum — RightHand, LeftHand, BowRig | `RPG/Data/` |
| `E_RPG_InteractPriority` | Enum — Pickup, Open, Assassinate | `RPG/Data/` |
| `PDA_RPG_Item` | Class (parent `PrimaryDataAsset`) — the item template | `RPG/Items/` |
| `DA_Item_*` | Instances (one per archetype) | `RPG/Items/Assets/` |
| `PDA_RPG_RarityPalette` + `DA_RarityPalette` | Class + instance | `RPG/Data/` |
| `BP_RPG_ItemInstance` | Class (parent `Object`) — runtime gear instance | `RPG/Items/` |
| `BPC_RPG_Inventory` | ActorComponent — on the player | `RPG/Blueprints/` |
| `BPI_RPG_Interactable` | Blueprint Interface | `RPG/Interaction/` |
| `BPC_RPG_Interactable` | ActorComponent — the interactable side | `RPG/Interaction/` |
| `BPC_RPG_Interactor` | ActorComponent — on the player, holds the registered set | `RPG/Interaction/` |
| `BP_RPG_WorldItem` | Actor — the pickup | `RPG/Interaction/` |
| `WBP_RPG_InteractPrompt` | Widget — `Press [E] {label}` | `RPG/UI/` |
| `S_RPG_LootEntry` | Struct — Item, DropChance, MinCount, MaxCount | `RPG/Data/` |
| `PDA_RPG_LootTable` + `DA_Loot_Grunt` | Class + instance | `RPG/Loot/` |
| `BPC_RPG_LootDropper` | ActorComponent — on enemies | `RPG/Loot/` |
| `WBP_RPG_Inventory`, `WBP_RPG_ItemCell`, `WBP_RPG_EquipSlot` | Widgets | `RPG/UI/` |
| `WBP_RPG_Forge`, `WBP_RPG_ForgeMaterialRow` | Widgets | `RPG/UI/` |
| `IA_RPG_Interact` (E), `IA_RPG_Inventory` (I) | Input Actions + `IMC_RPG_Default` entries | `RPG/Input/` |

---

## 4. Prep task (do first, ~15 min, unblocks steps 6–7)

1. Copy the 4 reference PNGs from `E:\Unity\Unity_Procedural_Level_Creator\Documentation\Asset and inventory UI design\design_handoff_candy_cloud\assets\` into `docs/design/` in this repo (`inventory_candy_cloud.png`, `forge_enough_candy_cloud.png`, `forge_short_candy_cloud.png`, `chibi_hero_reference.png`), plus that folder's `README.md`.
2. ~~Acquire **Fredoka** and **Nunito** (both SIL Open Font License, free) and import to `Content/RPG/UI/Fonts/`.~~ ✅ **DONE 2026-08-02.** Live as **6 `FontFace` assets** in `/Game/RPG/UI/Fonts/` (`{Fredoka,Nunito}_{Regular,SemiBold,Bold}`); TTF sources + OFL licences + README in `Art_Source/Fonts/`. Two traps captured in memory `rpg-ui-fonts.md`:
   - ☠ **Assign the `FontFace` DIRECTLY** to `font.fontObject` — verified round-trip. **Do not try to build `Font` (UFont) typeface assets**: `unreal.Typeface`/`TypefaceEntry`/`FontData` aren't exposed to Python, and Epic's `ObjectTools` doesn't expose `compositeFont.defaultTypeface` either. It is unbuildable from script on both servers.
   - ☠ **Never import the variable TTFs.** Upstream ships variable-only and UE uses just the default instance — **wght 300 (Fredoka) / 200 (Nunito)** — so you'd get Light/ExtraLight labelled as Regular. Statics were cut at 400/600/700 with `fontTools.varLib.instancer`.

---

## 5. The seven steps

Each step is independently testable and leaves the game in a working state. Sizes are relative: **S** ≈ part of a session, **M** ≈ a session, **L** ≈ two, **XL** ≈ several.

### Step 1 — Item data layer + rarity palette · **S**
*Spec §2, §3.*

Create the six enums, then `PDA_RPG_Item` with fields: `Id` (Name), `DisplayName` (Text), `Description` (Text), `Kind`, `Slot`, `Category`, `MountPoint`, `Damage` (int), `RequiredLevel` (int), `Icon` (soft Texture2D), `StaticMeshAsset` / `SkeletalMeshAsset` (soft), `AttachRotation` (Rotator), `Rarity`. Add a pure `IsMaterial` → `Kind == Material`.

`PDA_RPG_RarityPalette`: a struct `S_RPG_RarityColors { Main, Soft, Text }`, a **Map** `E_RPG_ItemRarity → S_RPG_RarityColors` (map, not array — see §2.1), the two non-rarity pairs (empty `#DBD5C9`/`#F1EDE4`, locked `#D9D3C6`/`#F3EFE7`), and a `PaletteSet` enum selector with both Candy Warm and Classic Bright tables authored. One `Get(Rarity)` function every widget calls.

Author **~8 items by hand** covering every code path: an OHS sword, a THS sword, a spear, a shield, a bow, a wand, and Mat 1/2/3. Meshes already exist in `Content/RPGTinyHeroWavePBR/Mesh/Weapon/`. Reuse the exact rotations already tuned in `StanceRightRotations`/`StanceLeftRotations` (e.g. spear roll 10, shield `0,0,-180`, off-hand sword `-90,0,-180`, bow yaw 170).

**Verify:** open each asset, confirm fields populate; a throwaway BP that reads a palette colour and prints it.

**Risk:** low. Nothing is wired to anything yet.

#### Tooling split (probed live 2026-08-01 — applies to every later step too)

| Scriptable by me | Must be authored by hand in the editor |
|---|---|
| BP class parented to `PrimaryDataAsset`; DataAsset instances; reading/writing/saving their properties | **User-Defined Enum assets** — `EnumFactory` creates them empty and there is no entry API at all |
| Variables typed: primitives, **object refs**, class refs, **engine structs**, arrays, sets, maps | **User-Defined Struct assets** — same wall |
| Populating array/struct/colour values on instances | **Enum-typed variables** and **soft-object-ref variables** — `EdGraphPinType` fields aren't settable from Python |

Two consequences baked into the design above:
- **Hard object refs instead of soft refs** for `Icon` / `StaticMeshAsset` / `SkeletalMeshAsset`. Fine at this scale; revisit if the catalogue grows past ~100 items. Tech debt, recorded here.
- **Arrays indexed by rarity instead of maps keyed by rarity** on the palette. Enum→Int works in Blueprint (only Int→Enum is blocked), so this is safe — and it matches the existing `StanceRightMeshes[CurrentStance]` pattern.

`E_RPG_InteractPriority` was **dropped** — priorities are compared numerically (10/50/100), so an int constant does the job and saves an enum.

**Status: STEP 1 COMPLETE IN C++ — regenerated 2026-08-01, second pass.**

The catalogue is now backed by `UItemData` / `URarityPalette`. The Blueprint step-1 assets are
**deleted**. Full record of the pass in **§2.7** below.

- **9 items** in `/Game/RPG/Items/Assets/` — `DA_Item_OHS03_Sword`, `_THS01_Sword`, `_Spear01`,
  `_Shield04`, `_Wand01`, `_Bow01`, `_Mat1/2/3`, all class `UItemData`.
- **`/Game/RPG/Data/DA_RarityPalette`** — class `URarityPalette`. Needed **no** data migration:
  the C++ constructor calls `ResetToDesignDefaults()`, which authors both tables from the design
  hexes and converts sRGB→linear in code. Confirmed the produced values match what the Blueprint
  asset held (`DBD5C9` → `(0.708, 0.665, 0.584)`; Candy Warm Common main `A99B86` →
  `(0.397, 0.328, 0.238)`).
- Every field was read back off each **saved** asset and compared against the intended value
  *before* anything was deleted — 10/10 clean, 0 errors.

**`AttachRotationOffHand` (design gap not in the spec):** a one-hand sword is valid in *either* hand — Melee (right) or OffHand for the DoubleSword stance (left) — and the two need different attach rotations, so one `AttachRotation` can't cover it. Items carry both; step 4's resolver picks by the slot the item is equipped in via `GetAttachRotationForSlot()`.

Mesh and rotation values were **read from the live `BP_RPG_PlayerCharacter` CDO** (`StanceRight/LeftMeshes`, `StanceRight/LeftRotations`) rather than retyped, so an equipped item lands identically to the Q-cycle: spear roll 10, shield yaw −180, off-hand sword roll −90/yaw −180, bow yaw 170.

~~**Outstanding (cosmetic):** `E_RPG_EquipSlot` index 1 is named `Uncommon`~~ — **moot.** That enum
asset is deleted; `EEquipSlot::OffHand` is correctly named in `RPGItemTypes.h`.

---

### 2.7 The regeneration pass (2026-08-01, second session)

**Environment.** Cowork had no `mcp-unreal` connection (see the handoff's environment note), so
this ran as two Unreal-Python scripts driven from the editor's Output Log. Both are checked in at
`Content/Python/` and are re-runnable:

| Script | What it does |
|---|---|
| `cc_step2_dump.py` | **Read-only.** Confirms the C++ types are registered, dumps all 9 Blueprint DataAssets + the palette + the player CDO stance arrays + the referencer graph of every delete candidate → `Saved/CC_Probe/step2_dump.json` |
| `cc_step2_author.py` | Builds → verifies → deletes → moves. Aborts before any delete if verification fails. → `Saved/CC_Probe/step2_author.json` |

**The safety shape is the point, and worth reusing:** build the new assets into a throwaway
`/Game/RPG/_Staging`, read every field back off the *saved* asset, and only then delete the
originals — each delete gated on the asset having no remaining referencer. A failure anywhere in
build or verify leaves the old assets untouched and the new ones parked in `_Staging` for
inspection. Both scripts log per line with an open/close per write, so a crash names its last
surviving step.

**Bug found and fixed in the step-1 data.** `DA_Item_Bow01` carried its yaw 170 in
`AttachRotationOffHand`, with `AttachRotation` at zero. The value is correct — it comes from
`StanceLeftRotations[7]`, which is why it got filed as off-hand — but the bow's slot is `Ranged`,
so `GetAttachRotationForSlot(Ranged)` returns `AttachRotation` and the 170 would never have been
read. The bow would have mounted 170° wrong the moment step 4 wired the resolver, and it would
have presented as a rig bug rather than a data one. `ItemData.h` states `AttachRotation` is the
field used "when mounted in the RIGHT hand (**or on the bow rig**)", so the value now lives there.
Every other item was cross-checked against the CDO and matched.

**Deleted (19 assets, leaf-first, each gated on zero referencers):** `BPC_RPG_Inventory` (which
took the stray `ProbeFunc` with it), `BP_RPG_ItemInstance`, the 9 `DA_Item_*`, `PDA_RPG_Item`, the
old `DA_RarityPalette`, `PDA_RPG_RarityPalette`, and the 5 `E_RPG_*` enums.
**`E_RPG_Stance` was explicitly excluded** — it is the stance legend and is still in use.

`BPC_RPG_Inventory` turned out to have **zero** referencers, confirming it was never attached to
`BP_RPG_PlayerCharacter`. That was the one genuinely risky deletion and the probe settled it
before the fact rather than after.

**`EInteractPriority` — resolved in favour of keeping it.** §5 step 1 dropped it because
hand-authoring user-defined enums in Blueprint was expensive; the C++ pivot removed that cost, so
it stays, with gaps in the 10/50/100 ladder for future tiers.

⚠ **The editor's Git source-control provider ran `git rm` on every deleted asset**, so those
deletions are already **staged in the index**. Run `git status` before committing. The
`Unable to Check Out From Revision Control!` dialogs during the move phase are the same provider
failing to check out files that did not exist yet — benign, the saves all succeeded.

---

### Step 2 — Inventory component + instance model · **M**
*Spec §4, plus §2.3 above.*

`BP_RPG_ItemInstance` (parent `Object`): `Template` (PDA_RPG_Item ref), `Level` (int, 0 for now). Functions `GetDamage`, `GetRarity`, `GetDisplayName` — all forwarding to the template today.

`BPC_RPG_Inventory` on `BP_RPG_PlayerCharacter`, three containers exactly as spec §4:
- `GearBag` — `Array<BP_RPG_ItemInstance>`, capacity **20**
- `Materials` — `Map<PDA_RPG_Item, int>`, uncapped
- `Equipped` — `Map<E_RPG_EquipSlot, BP_RPG_ItemInstance>`

API per spec: `AddItem`, `RemoveItem`, `HasItem`, `GetMaterialCount`, `SpendMaterial`, `Equip`, `Unequip`, `GetEquipped`, `IsSlotEquipped`.

**Implement the spec's two fixes, not Unity's bugs:**
- `Equip` returns the displaced item to the bag instead of dropping it.
- The dispatcher is `OnEquipChanged(Slot, NewInstance, OldInstance)` — Unity's `OnWeaponEquipped(null)` doesn't say which slot emptied.

Also fix the partial-add hazard the Unity source has: `AddItem` with count > capacity-remaining currently reports success and the caller destroys the pickup, losing the remainder. Return the **number actually added** so step 3 can decide.

Do **not** port the Unity singleton — get the component off the pawn.

**Verify:** console commands / a temporary debug key that adds, equips, prints state. No visuals yet.

**Risk:** low-medium. Watch BP Map key behaviour with object refs when assets are hot-reloaded in the editor.

#### Status — DONE IN C++ (2026-08-01). The Blueprint scaffolding above is deleted.

`UInventoryComponent` (`Source/Character_Creator/Inventory/`) supersedes `BPC_RPG_Inventory`
entirely, and `UItemInstance` supersedes `BP_RPG_ItemInstance`. Both Blueprint assets were removed
in the regeneration pass (§2.7) — the stray `ProbeFunc` went with them.

C++ removes the two workarounds the Blueprint version needed: `Equipped` is a genuine
`TMap<EEquipSlot, UItemInstance*>` rather than an index-array, and refs can be soft.

The full §4 API plus the three delegates are written. Two changes beyond the spec, both recorded
in the handoff:

- **`EquipToSlot(Instance, Slot)` is the real entry point**; `Equip()` is a wrapper passing the
  template's default slot. Deriving the slot from `Template->Slot` — a single value — meant a
  one-hand sword authored as `Melee` could never reach `OffHand`, making step 4's stance rows 3
  (sword+shield) and 4 (double sword) unreachable and `AttachRotationOffHand` dead weight.
  `CanEquipToSlot()` is permissive in exactly one place: OffHand also accepts `Shield` or `OHS`.
  `EquipToSlot` clears the instance out of any *other* slot it held, so a sword moved
  Melee→OffHand can't mount in both hands.
- **`Unequip()` refuses** (returns false) when the bag is full, rather than destroying the item.

**Not yet exercised.** The component compiles and links but has never been instantiated — nothing
adds it to `BP_RPG_PlayerCharacter` yet, and no call path reaches `AddItem`/`EquipToSlot`. That
happens in step 3, which is also where the first real test of the API lands.

---

### Step 3 — Interaction system + world pickup · **M**
*Spec §5.*

`BPI_RPG_Interactable` with `GetPriority`, `GetPromptLabel`, `IsEligible(Player) → bool`, `Execute(Player)`.
`BPC_RPG_Interactable`: sphere overlap, self-registers with the player's `BPC_RPG_Interactor` while eligible, owns a `WBP_RPG_InteractPrompt` `WidgetComponent` in screen-facing mode.
`BPC_RPG_Interactor` on the player: holds a Set, recomputes the highest-priority active on register/deregister, dispatches `IA_RPG_Interact` (E) to it.

**Make tie-breaks deterministic** — Unity's `HashSet` iteration order made ties undefined and the source itself flags that as not-to-be-relied-upon.

`BP_RPG_WorldItem`: 1.5 m sphere trigger, priority `Pickup`, label `Pick Up {DisplayName}` (or `… x{count}` for stacked materials), eligible when it has an item, `Execute` → `AddItem` → destroy self. **Auto-equip OFF.** Inventory full → log and abort, pickup stays.

Use `SpawnActorDeferred` → set Item + Count → `FinishSpawning` (the UE analogue of Unity's configure-inactive-then-activate).

**Verify:** hand-place a few `BP_RPG_WorldItem`s in `Lvl_RPG_Test`; walk up, prompt appears, E collects, bag reflects it. Place two overlapping interactables of different priority and confirm arbitration.

**Risks:**
- MCP **cannot set the `InputAction` pin** on an `EnhancedInputAction` node — wiring the node is a manual editor step. *(Creating the IA asset and the IMC mapping turned out to be scriptable — see the status block.)*
- The CC-archive assassinate prompt on `BP_RPG_Enemy` is already broken (its overlap casts `OtherActor → BP_RPG_Enemy`, which can never succeed). This new system supersedes it. **Leaving it alone** unless you say otherwise.

#### Status — DONE IN C++, PIE-CONFIRMED (2026-08-01)

Walking up to a pickup shows a prompt; E collects it and the actor destroys itself.

Built as **8 files in `Source/Character_Creator/Interaction/`** — `IRPGInteractable`,
`UInteractableComponent` (a `USphereComponent` at 150 uu that *is* the trigger and registers
itself), `UInteractorComponent` (arbitration + `TryInteract()`), `AWorldItem`.
Names deviate from §3's Blueprint-era table: `AWorldItem` not `BP_RPG_WorldItem`, no `BPI_/BPC_`
assets. Full detail, traps and design rationale in `.claude/handoff.md`.

**The plan's deterministic-tie-break requirement is met and then some:** priority desc → nearest →
lowest object name. The last term only fires at identical priority *and* distance; it exists so
there is no undefined outcome at all.

**Two deliberate deviations:**
1. **No `WBP_RPG_InteractPrompt` and no `WidgetComponent`.** The prompt is on-screen debug text
   driven by `UInteractorComponent::OnActiveInteractableChanged`. The real widget belongs to
   step 6 and subscribes to that delegate without changing this code. Building the widget
   plumbing now would have blocked all step-3 testing behind hand-built UMG.
2. **A full bag does not hide the prompt** — `IsInteractEligible` ignores capacity and the press
   fails loudly. Hiding it reads as "there is nothing here".

**Scriptability finding that contradicts §5's tooling assumptions:** `UInputMappingContext::MapKey`
and `UnmapKey` are `UFUNCTION(BlueprintCallable)`, so IMC key mappings **are** scriptable. Only the
`EnhancedInputAction` graph node stays manual. Also, `UInputMappingContext::Mappings` is
`UE_DEPRECATED(5.7)` — the live data is `DefaultKeyMappings.Mappings`, which is why Python had
always reported this asset as empty.

**Still untested** (only the happy path ran): priority arbitration across *different* priorities,
the bag-full path, and partial adds. One fix is written but possibly unrun —
`Content/Python/cc_step3_trigger.py`, which gives `IA_RPG_Interact` a `UInputTriggerPressed` so
`Triggered` stops firing every frame the key is held.

---

### Step 4 — Equip consumers · **L** — the fiddliest step
*Spec §6. This is where the new system meets the already-built player.*

Three independent listeners on `OnEquipChanged`, built in this order:

**a) Mesh mount.** Do **not** rewrite `ApplyStance` — refactor it to source meshes through two new pure functions:

```
ResolveRightHandMesh():
    inst = Equipped[Melee]  (or Equipped[Ranged] if its MountPoint == RightHand)
    if inst valid → return inst.Template.StaticMeshAsset, inst.Template.AttachRotation
    else          → return StanceRightMeshes[CurrentStance], StanceRightRotations[CurrentStance]
```

Same shape for the left hand and the bow rig. **Equipped items win; the arrays are the fallback.** This keeps the Q dev-cycle fully working with an empty inventory — which spec §6c explicitly requires ("the Q dev-cycle stays for testing") — and means the change to `ApplyStance` is two `Select`-style branches, not a rewrite.

Note `SetRelativeRotation` is absolute and overrides component defaults, so rotations must come from the item/array, never from the SCS.

**b) Damage source.** Replace the hard-coded `20` literal on the `ApplyDamage` node in `MeleeHit` with a pure `GetMeleeDamage()`:
```
inst = Equipped[Melee];  damage = inst valid ? inst.GetDamage() : 20
```
Pull at swing time, not cached — an equip swap mid-combo then lands on the very next hit with no invalidation logic.

*Cheap bonus (spec §13 lists it as deferred):* `BP_Arrow` already has a `Damage` variable, so set it from `Equipped[Ranged]` at spawn time in the same pass. Roughly one node. Recommend taking it.

**c) Stance bridge.** On any equip/unequip, re-derive `CurrentStance` from the equipped set and apply it **without** re-mounting (a already did that):

| Equipped | Stance |
|---|---|
| Ranged = Bow | 7 |
| Ranged = Wand | 6 |
| Melee empty | 0 |
| Melee = THS | 2 |
| Melee = Spear | 5 |
| Melee = OHS + OffHand Shield | 3 |
| Melee = OHS + OffHand OHS | 4 |
| Melee = OHS, off-hand empty/other | 1 |

Ranged beats melee. **Switch on the item's `Category` field** — never on prefab-name prefixes (spec §6c ⚠, and Unity's own `WeaponTypeResolver` is a documented string-matching hack).

`CurrentStance` stays an **int** — that is deliberate and load-bearing (no int→enum conversion, and the AnimBP's `Blend Poses by Int` consumes it directly). Write the int; do not "improve" it to `E_RPG_Stance`.

**Verify:** equip each of the 8 configurations via a debug key; confirm the correct mesh mounts in the correct hand, the blendspace switches, the reticle shows for stances 6/7, and a swing deals the item's damage. Screenshot each via the SceneCapture2D → render-target route.

**Risks:** highest of any step. Touching `ApplyStance` and `MeleeHit` means touching graphs that took several sessions to get right. **Commit before starting** (you commit manually — this is a flag, not an offer). Known MCP traps in play: `GetArrayItem` fed by `connect_pins` stays Wildcard and fails to compile; `inspect` misreports array types and reports `CurrentStance` as an enum when it's an int.

#### Status — DONE, PIE-VERIFIED (2026-08-01, session 3)

Full record in `.claude/handoff.md` § "Step 4". Summary:

**The plan's "refactor ApplyStance, don't rewrite it" instruction was honoured, but the seam it
was working around got removed instead.** Rather than a Blueprint `Select` between "the equipped
item" and "the stance array", the user chose to **reparent `BP_RPG_PlayerCharacter` onto a new C++
base, `ARPGPlayerCharacter`**, moving the six stance tables into C++ so the resolvers can read them
directly. `ApplyStance` then needed only **one resolver node per hand** — a smaller edit than the
two Selects this plan predicted.

- **a) Mesh mount** — `ResolveRightHandMount` / `ResolveLeftHandMount` / `ResolveBowRigMount`.
  Equipped wins, tables are the fallback, so the Q dev-cycle still works with an empty inventory
  exactly as spec §6c requires. Verified across all 8 stances.
- **b) Damage source** — the hard-coded `20` became `UnarmedMeleeDamage`; `GetMeleeDamage()` and
  `GetRangedDamage()` are pulled at swing time, never cached. **The arrow-damage bonus was taken**
  (`BP_Arrow.Damage` is now `Expose on Spawn`, fed by `GetRangedDamage`).
- **c) Stance bridge** — `DeriveStanceFromEquipment()` implements the 8-row table in text,
  switching on `Category`, ranged beating melee. `CurrentStance` stayed an **int**, as instructed.

**Verified in a live PIE session driven from Python** (no debug key was needed — see the handoff):
stance 1→2→7→2→3→0 as items went on and off, melee damage 20→25→15→20, ranged 30→18, the correct
meshes mounting on the real components, and unequip returning items to the bag.

**Not yet verified:** that a real swing/arrow actually *consumes* those damage values — the source
functions are right and the wiring node-count is consistent, but no live combat check was run.

**Consequence for later steps:** gameplay logic can now be written in C++ text on the player. The
seam that made this step awkward is gone, and steps 5/7 inherit that.

---

### Step 5 — Loot tables + dropper · **M**
*Spec §9.*

`S_RPG_LootEntry` + `PDA_RPG_LootTable` holding an array of them. `DA_Loot_Grunt` with the live values: Mat 1 / 80% / 1–3 · Mat 2 / 30% / 1–2 · Mat 3 / 5% / 1 · THS01 sword / 25% / 1.

Roll algorithm — **each entry rolls independently**, so one kill can drop several or none:
```
for each entry:  if Random01 > DropChance → skip
                 count = RandomIntInRange(Min, Max)   // inclusive
                 emit (item, count)
```

`BPC_RPG_LootDropper` on `BP_RPG_Enemy`, bound to a new `OnDeath` event dispatcher. **Single-fire guard set before the null-table check**, so a re-raised death never double-drops.

**The one edit to `BP_RPG_Enemy`:** add an `OnDeath` dispatcher and call it on the **true branch of the `IsPlayerDead?` Branch, before the 2 s delay**. That gives the dropper a 2-second window before `DestroyActor`. Do **not** put the dispatcher on `BPC_PlayerStats` — it's shared CC-archive code with a known bug the user has said to leave alone.

Placeholder visuals per spec: **cube = gear, sphere = material**, rarity-tinted (the simpler block palette: Common `0.85,0.85,0.85` · Uncommon `0.35,0.82,0.38` · Rare `0.34,0.58,1.0` · Legendary `1.0,0.74,0.20`), scattered in a 0.6 m radius, 0.6 m up, scale 0.3.

**Carry the lesson from spec §9's bug note:** killing with an arrow runs the whole death chain inside a collision callback. Spawn deferred and finish outside it. Unity's fix was to build the block with *no collider at all*; the UE equivalent is to not mutate actors during the overlap event.

Real art later is a one-line swap: spawn the item's mesh instead of the primitive.

**Verify:** kill the enemy repeatedly, confirm drop rates look right over ~20 kills, confirm arrow-kills and melee-kills both drop cleanly, confirm no double-drop.

**Risk:** medium — the arrow-kill-inside-collision path is the known landmine.

#### Status — DONE IN C++, PIE-VERIFIED (2026-08-02, session 4)

Built as C++ rather than the BP assets named above, because user-defined structs cannot be
authored or populated from Python at all — `S_RPG_LootEntry` as a BP struct would have been
hand-built forever. Files: `Source/Character_Creator/Loot/{LootTypes.h, LootTable.h/.cpp,
LootDropperComponent.h/.cpp}`. Module rebuilt clean, first try.

- **`FLootEntry` / `FLootDrop`** — entry is (Item, DropChance, MinCount, MaxCount).
- **`ULootTable::RollLoot()`** — independent per-entry roll, `FMath::RandRange` inclusive both ends,
  guards against an authored `Max < Min`. **Verified empirically over 2000 rolls**, not just
  compiled: Mat1 0.785 / Mat2 0.290 / Mat3 0.050 / THS01 0.257 against spec 0.80 / 0.30 / 0.05 / 0.25.
- **`ULootDropperComponent::DropLoot()`** — single-fire guard set **before** the null-table check, so
  a misconfigured dropper still latches. **Spawning is deferred to the next tick** via
  `SetTimerForNextTick`, which is the fix for the arrow-kill landmine: that path runs the whole
  death chain inside the projectile's overlap callback, and this gets off that stack. Uses the
  existing `AWorldItem::SpawnWorldItem` (already SpawnActorDeferred → configure → FinishSpawning).
- **`DA_Loot_Grunt`** at `/Game/RPG/Data/`.
- **Death hook:** a direct `Drop Loot` call on the TRUE branch of `IsPlayerDead?`, before the 2s
  delay — chosen over the planned `OnDeath` dispatcher purely because it is one node instead of
  three and MCP can author neither.

**⚠ Placeholder meshes are load-bearing, not cosmetic.** `DA_Item_Mat1/2/3` have **no
`StaticMeshAsset`** — materials render only via `AWorldItem::PlaceholderMesh`. Spawning a bare
`AWorldItem` drops them **invisible but collectable**. The dropper sets cube-for-gear /
sphere-for-material explicitly and logs a loud warning if the material mesh is ever unset.

**Also worth knowing:** no Blueprint child of `AWorldItem` exists — the dropper spawns the raw C++
class. The moment per-item VFX via `OnItemVisualsApplied` (a `BlueprintImplementableEvent`) is
wanted, a BP child is needed and `world_item_class` should be repointed at it.

Write-up: the `.claude/task_Step5_LootDropper.md` working doc was deleted after step 5 shipped
(its header still read "one node left for you"). Recover from git history if needed.

---

### Step 6 — Inventory / Character screen · ~~**XL**~~ **L–XL, re-scoped 2026-08-02**
*Spec §7. Full layout tree with exact px/hex is in the Unity handoff README, now copied to `docs/design/`.*

> ## ▶ RE-SCOPE (2026-08-02) — read this before the original text below
>
> The original XL rested on "all hand-authored UMG". That premise is dead: the **full authoring
> loop is proven** (`CreateWidgetBlueprint` → `AddWidget` → `list_properties` → `set_properties` →
> `BindToEventProperty` → `CompileWidgetBlueprint` → `save_assets` → verified on disk), and
> `list_properties` is **per-class cacheable**, so structure and styling are scripted work now.
> Fonts are imported (§4). What did *not* get cheaper: visual judgement, the hero preview, and
> graph logic.
>
> **The governing decision: push logic into C++, keep widget graphs thin.** This project already
> ruled that "gameplay data + logic go in C++; Blueprint stays for actors, UI and glue". Doing that
> here also sidesteps the one unproven tool (`write_graph_dsl`) — if the widgets only ever call
> `UFUNCTION`s and bind events, the graphs stay small enough to build with `BindToEventProperty`
> plus a handful of nodes.
>
> ### ✅ DE-RISKED 2026-08-02 — `write_graph_dsl` WORKS, so **6E is M**
> Proven on a throwaway widget: authored a branch + bool get/set + two `SetText` calls with
> different literals, **compiled clean, saved to disk**, then deleted (git confirmed clean).
> **Graph authoring is scripted — the "user places nodes, Claude wires pins" pattern from sessions
> 1–4 is retired** for anything the DSL can express. Writes are **additive per event**, not
> destructive. Full gotcha list in memory `blueprint-graph-dsl.md`; the load-bearing ones:
> - ☠ **Bool member vars drop the leading `b`** — `bSelected` → `Variables|Default|GetSelected`.
> - ☠ **Widget variables live under `Variables|<BlueprintName>|Get<WidgetName>`**, not `Default`,
>   and are **not** auto-available as bare DSL identifiers — bind the getter first.
> - ☠ **`read_graph_dsl` output is NOT valid `write_graph_dsl` input** (member vars read back as
>   `(|GetbSelected)`, which the writer rejects). No naive read→modify→write; re-derive ids with
>   `find_node_types` (which needs `graph` **and** `context_pins: []`).
>
> ### Build order
>
> | # | Sub-step | Size | Scripted? |
> |---|---|---|---|
> | **6A** | ✅ **DONE 2026-08-02.** `URPGUIStyle` (BP function library) is the single API for colour/type/metrics; new **`DA_RPGUITheme`** holds chrome + a 13-role type scale + layout metrics beside `DA_RarityPalette`'s rarity colour, both resolved through **`URPGUISettings`** (Project Settings → Game → RPG UI). Verified end to end: `IsStyleConfigured()` true, Legendary `#FF9A3D`, ink `#4B57C9`, slot 58 / 5 cols / gap 14, fonts per role. Missing assets render **magenta** by design. Also fixes the old per-actor `Palette` pointer that left `AWorldItem::Palette` unset. Details in memory `rpg-ui-style-foundation.md`. | **S** | ✅ done |
> | **6B** | **`WBP_RPG_ItemCell`.** 58×58, 2px rarity border, rarity soft bg, icon, `×N` stack badge (materials, N>1 only), selected ring. API: `SetItem(instance)`, `SetSelected(bool)`, `OnCellClicked` dispatcher. Icons fall back to name initials — none authored. | **S–M** | ✅ structure + style; selection ring needs an eyeball |
> | **6C** | **`WBP_RPG_EquipSlot`.** Slot + overlapping label chip; locked variant (dimmed icon 30%, 🔒 badge, non-interactive, **never reads inventory state**). 8 shown, 4 wired. | **S** | ✅ fully |
> | **6D** | **`WBP_RPG_Inventory` shell.** 980×600 canvas; title bar (gold pill / `Knight · Lv 8` / ✕), two-panel body (flex 1.25 : 1, gap 14), stats row, Skins/Stats buttons, tabs, 5-col grid container, detail strip, footer. Pure structure + slot layout. | **M** | ✅ fully — this is the bulk of the old "XL" and it is now scripted |
> | **6E** | **Behaviour.** `IA_RPG_Inventory` (I) · `SetGamePaused` + **widgets must tick while paused** · `SetInputModeGameAndUI` + cursor (override `DefaultInput.ini`'s `CapturePermanently_IncludingInitialMouseDown` / `LockOnCapture`) · live refresh on open + tab change (**no snapshot**) · tab filtering · selection → detail strip · stats row from equipped melee + player stats · **equip-from-UI** (the thing Unity never built). | **M** ✅ | ✅ `write_graph_dsl` proven; C++ helpers keep the graphs small |
> | **6F** | **Hero preview — LIVE PLAYER MODEL** (decided, see below). `SceneCaptureComponent2D` → render target → `Image` brush, + drag-to-rotate. Framing/lighting is iteration, not scripting. | **M** | ❌ mostly manual |
> | **6G** | **Polish.** Spacing, shadows, real glyphs (`✓ ✕ × · — ▲ ✦ ➜`), Candy-Warm check against the palette asset. | **M** | ❌ judgement |
>
> **Net: L, down from XL.** 6A–6E are all scripted (S+SM+S+M+M); the remaining manual work is
> **6F hero preview** and **6G polish** — both visual-judgement passes, not authoring. The step is
> no longer dominated by hand-building UMG.
>
> ### Two C++ additions that make 6E cheap
> 1. `UInventoryComponent::GetFilteredItems(ECategory)` — tab filtering in C++, so the widget calls
>    one function instead of building a filter graph.
> 2. A stats accessor returning Damage/Armor/HP/Stamina as one struct — `GetMeleeDamage()` already
>    exists on `ARPGPlayerCharacter`; Armor/HP/Stamina still come from `BPC_PlayerStats`, which is
>    **CC-archive** and ⚠ **only touched when the user asks** (see handoff).
>
> ### Carried-over constraints (unchanged, still binding)
> - **All colours from `DA_RarityPalette`** — the two-palette swap must stay a one-field change.
> - Reference PNGs render **Classic Bright**; the build targets **Candy Warm**. Trust the asset.
> - A full bag must **not** hide interaction — established in step 3, same principle applies to UI.

`WBP_RPG_Inventory` at reference canvas **980 × 600**, plus `WBP_RPG_ItemCell` and `WBP_RPG_EquipSlot` sub-widgets.

- **Toggle:** `IA_RPG_Inventory` (I).
- **Pause:** `SetGamePaused` — and **set the widgets to tick while paused** or the screen freezes.
- **Cursor:** `SetInputModeGameAndUI` + `bShowMouseCursor`. Do **not** port Unity's cursor request-counting, and do not port its always-active-host-with-toggled-child pattern — UE binds input on the controller. Note `DefaultInput.ini` sets `CapturePermanently_IncludingInitialMouseDown` / `LockOnCapture`, which will need overriding on open.
- **Data is live**, refreshed on open and on tab change. No snapshot.
- **Layout:** title bar (gold pill · `Knight · Lv 8` · ✕) / left Character panel flex 1.25 (`[left slots][hero preview][right slots]`, 4-up stats row, Skins/Stats buttons) / right Items Bag panel flex 1 (title · tabs · 5-col grid · detail strip · footer). Panel gap 14, grid gap 8, slot column gap 16, slot 58×58, radii 12–26, touch targets ≥44.
- **8 slots shown, 4 wired** (Melee/OffHand/Ranged/Armor); Head/Chest/Feet/Charm locked, non-interactive, never read inventory state, icon at 30% opacity + lock badge.
- **Tabs:** All · Weapons · Armor · Materials · Potions(disabled). Grid pads to a minimum of 20 cells. `xN` badge on materials only when N > 1. Icons fall back to name initials (no icons authored yet).
- **Stats row** reads Damage from equipped melee, and Armor/HP/Stamina from the player's stats.

**All colours come from `DA_RarityPalette`.** No hex literals in widgets — the two-palette swap must stay a one-field change.

**Use the real glyphs** (`✓ ✕ × · — ▲ ✦ ➜`). Unity swapped them to ASCII only because it hadn't imported the fonts; with Fredoka/Nunito imported (§4) that constraint doesn't exist here.

**Close the loop: implement equip-from-UI.** Clicking a bag cell or an equipment slot equips/unequips. Unity never built this (§13) — UE5 leads here, and it's what makes the whole feature playable rather than a viewer.

~~**Hero preview stays a placeholder**~~ — **DECIDED 2026-08-02: the LIVE PLAYER MODEL.** The user chose this over §7's "separate always-idle instance" recommendation. Consequences to build against, since they're the reasons §7 argued the other way:
- The preview shows the pawn in whatever state gameplay left it — mid-animation, damaged, badly lit. **The game is paused while the screen is open**, so expect a *frozen* pose, not an idle loop. If that reads badly, the fix is a scene-capture framing/lighting pass, **not** re-opening the decision.
- Equip-from-UI updates are free and exactly right — it's the same pawn `ApplyStance` already drives, so no mirroring of equipment state onto a second actor.
- Implementation: `SceneCaptureComponent2D` → render target → `Image` brush. This project has already proven that path (memory `inspecting-blueprints`: remote rendering works via SceneCapture2D→render-target→PNG, and **SceneCapture is not decoupled from the camera the way viewport capture is**).

**Verify:** PIE — open with I, confirm pause and cursor, pick items up and watch the grid update live, equip from the UI and see the weapon change in the world behind the screen.

**Risks:** ~~`WidgetTree` isn't Python-exposed, so this is entirely hand-authoring in the designer — by far the largest manual chunk in the plan.~~
⚠ **THIS RISK NOTE IS STALE — DO NOT SIZE THIS STEP FROM IT (corrected 2026-08-02).** The project is on **UE 5.8.1** and Epic's in-editor MCP server exposes **`UMGToolSet`, 23 tools** — the tree *is* scriptable (`CreateWidgetBlueprint`, `AddWidget`, `MoveWidget`, `WrapWidgets`, `SetNamedSlotContent`, `BindToEventProperty`, `ToggleWidgetAsVariable`, `CompileWidgetBlueprint`, `GetWidgets`, `GetNamedSlots`…). Verified by live enumeration + a `ListWidgetBlueprints("/Game/RPG")` call, not from docs. **Re-scope step 6 before estimating it.**
Two hard constraints when you do: (1) ☠ start the **UE editor before Claude Code** or the tools won't attach at all; (2) `ObjectTools.list_properties(widget)` is **mandatory** before `set_properties` — property names vary per widget class and cannot be guessed, and skipping it fails *silently*. Details: handoff § "MCP: run BOTH servers", memory `epic-mcp-gateway.md`.
Still true: the reference PNGs show Classic Bright while the build targets Candy Warm; trust the palette asset, not the screenshots. Font import (Fredoka/Nunito) is also still a real blocking task.

---

### Step 7 — Weapon instances + Forge screen · **L**
*Spec §8. Blocked in both engines until the instance model exists — which §2.3 front-loads to step 2.*

Flesh out `BP_RPG_ItemInstance`: add `BaseDamage`, `DamagePerLevel`; make `GetDamage()` return `BaseDamage + Level × DamagePerLevel` and `GetRarity()` derive from `Level` by tunable thresholds. Because everything above already reads through these accessors, **nothing else changes.**

Add an upgrade cost table (per level, `material → needed`, escalating in quantity and tier: Mat 1 Common → Mat 2 Rare → Mat 3 Legendary) and the upgrade action: verify all costs → `SpendMaterial` → `Level += 1` → recompute → celebrate.

`WBP_RPG_Forge` at **620 × 460**, driven by a single BP-callable `Show(S_RPG_ForgeViewModel)`. **Keep that one-seam property** — it's the entire integration surface, and it's why the Unity screen renders both states off stub data. View-model: `WeaponName`, `Level`/`Rarity`/`CurrentDamage`, `NextLevel`/`NextRarity`/`NextDamage`, `Materials[]` of `{DisplayName, Rarity, Have, Need}`; derived `CanUpgrade` (every row `Have ≥ Need`) and `Delta`.

Rendering rules are fully specified (state pill, current/next cards, 4-pill rarity ladder at 0.4 alpha for inactive, material rows with status dots fading short rows to 0.55, the two button states). Build it against **stub view-models first** — that's how you get both states rendering without needing real upgrade data — then swap in the real builder.

**Verify:** stub both states (enough / short) and confirm they match the two reference PNGs; then upgrade a real weapon and watch damage, rarity and mesh-in-hand update.

---

## 6. Cross-cutting risks and tooling traps

| Trap | Mitigation |
|---|---|
| MCP cannot set object-reference pins, `InputAction` pins, function-call targets, or AnimGraph internals | Budget manual editor steps for input wiring and any node that takes an asset reference |
| `GetArrayItem` fed by MCP `connect_pins` stays Wildcard → "type undetermined" compile error | Refresh Nodes, or re-drag from the typed pin |
| `blueprint_query inspect` misreports types — arrays as their inner type, `CurrentStance` as an enum when it's an int | Verify via CDO readback or by reading pin types in `get_graph` |
| A BP editor tab open during a Python variable add shows the wrong type | Close and reopen the tab |
| No Int → User-Defined-Enum conversion in BP | Keep enums as enums; use Maps keyed by enum instead of int-indexed arrays |
| Steps 4 and 5 modify graphs that took multiple sessions to stabilise | Commit before each (manual — flagging, not offering) |
| Editor must be running with the MCPUnreal plugin on port 8090 for any live inspection | Confirm `status` before a session; on a connection drop, ask rather than retry blindly |

---

## 7. Open decisions for you

1. **§2.3 — front-load the instance model to step 2?** Recommended. Veto if you'd rather follow spec §12 literally.
2. ~~**Hero preview**~~ ✅ **CLOSED 2026-08-02 — the LIVE PLAYER MODEL.** (The recommendation here was *separate instance*; the user chose live. Recorded at step 6 with the consequences to build against.) ~~a *separate preview instance* — always idle, well-lit, independent of gameplay state — versus the *live player model*. Recommendation: separate instance, for exactly those reasons.~~
3. **Palette:** confirm **Candy Warm** is still the chosen set. Both are authored either way; it's a one-field flip.
4. **Item catalogue scope:** author ~8 items by hand (step 1) and expand later, or script all 57 weapon archetypes up front from the pack meshes? Recommendation: 8 now — every code path is covered, and bulk generation is trivial once the class is proven.
5. **Modular character direction:** the memory note flags `ModularCharacter_BP`'s pre-rotated per-type components (`Weapon_R 0,0,-180`, `Weapon_L -90,0,-180`, `Shield 0,0,-90`, `Bow 90,90,0`) as the chosen path for future multi-character/armor work. That model would obviate per-item attach rotations entirely. **Not in this plan's scope** — but if you want to move to it, step 4 is the natural moment, and it would change §2.4.
6. **Arrow damage from the item** — spec §13 defers it, but `BP_Arrow.Damage` already exists so it's roughly one node during step 4. Take it?

---

## 8. Summary

| Step | What | Size | Chief risk |
|---|---|---|---|
| Prep | Copy design PNGs, import Fredoka + Nunito | S | — |
| 1 | Enums, item data asset, rarity palette, ~8 items | S | none |
| 2 | Inventory component + instance model | M | BP map keys |
| 3 | Interaction system + world pickup | M | manual input wiring |
| 4 | Equip → mesh / damage / stance | **L** | **touches proven graphs** |
| 5 | Loot tables + dropper | M | arrow-kill inside collision callback |
| 6 | Inventory screen (+ equip-from-UI) | ~~**XL**~~ **L** | 6A–6E scripted (UMG tree + graph DSL both proven); remaining manual = hero preview + polish |
| 7 | Weapon instances + Forge | L | none if §2.3 is taken |

Steps 1–3 are additive and touch nothing existing. Step 4 is the integration point and the one to be careful with. ~~Steps 6–7 are the bulk of the calendar time, almost entirely UMG hand-authoring.~~ **Corrected 2026-08-02:** steps 6–7 are still the bulk of the calendar time, but the "almost entirely hand-authoring" premise is dead — Epic's `UMGToolSet` (23 tools) makes most of the tree scriptable. **Both sizes need re-deriving before they mean anything.**
