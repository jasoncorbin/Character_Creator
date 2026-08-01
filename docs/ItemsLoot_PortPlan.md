# Items / Loot / Inventory UI — UE5 Port Plan

**Status:** approved 2026-08-01 — all recommendations taken, to be revisited during testing. Step 1 in progress.

**Decisions locked:** instance model front-loaded to step 2 · hero preview = separate always-idle instance · palette = **Candy Warm** · author ~8 items by hand now · take the arrow-damage-from-item win in step 4 · modular-character direction stays out of scope.
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
| **UMG `WidgetTree` is not Python-exposed in 5.7.** | Every widget layout is hand-built in the designer. This dominates the cost of steps 6–7. |
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

**Expect first-compile errors** — none of this has been through UHT yet. Two classes of mistake were already pre-empted (Blueprint-exposed functions can't return const references, and const object pointers are poor BP params), but assume more.

**After a green build:** reopen the editor, regenerate the 9 items + palette against the C++ classes, and delete the superseded Blueprint assets — `PDA_RPG_Item`, `PDA_RPG_RarityPalette`, `DA_RarityPalette`, the 9 `DA_Item_*`, `BP_RPG_ItemInstance`, `BPC_RPG_Inventory` (which takes the stray `ProbeFunc` with it), and the 5 `E_RPG_*` enums.

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
2. Acquire **Fredoka** and **Nunito** (both SIL Open Font License, free) and import to `Content/RPG/UI/Fonts/`. Without these the UI cannot match the design and we'd be re-deciding type at build time.

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

**Status: STEP 1 COMPLETE (2026-08-01).**
- `PDA_RPG_Item` — 10 fields scripted (`Id`, `DisplayName`, `Description`, `Damage`, `RequiredLevel`, `Icon`, `StaticMeshAsset`, `SkeletalMeshAsset`, `AttachRotation`, `AttachRotationOffHand`) + 5 enum fields authored by the user (`Kind`, `Slot`, `Category`, `MountPoint`, `Rarity`), all verified as real enum types.
- `PDA_RPG_RarityPalette` + `DA_RarityPalette` — both palettes populated, Candy Warm active, hexes sRGB→linear converted.
- 5 enums in `RPG/Data/`, all entry orders verified against spec.
- **9 items** in `RPG/Items/Assets/`: OHS03 Sword, THS01 Sword, Spear01, Shield04, Wand01, Bow01, Mat1/2/3.

**`AttachRotationOffHand` added mid-step (design gap not in the spec):** a one-hand sword is valid in *either* hand — Melee (right) or OffHand for the DoubleSword stance (left) — and the two need different attach rotations, so one `AttachRotation` can't cover it. Items now carry both; step 4's resolver picks by the slot the item is equipped in.

Mesh and rotation values were **read from the live `BP_RPG_PlayerCharacter` CDO** (`StanceRight/LeftMeshes`, `StanceRight/LeftRotations`) rather than retyped, so an equipped item lands identically to the Q-cycle: spear roll 10, shield yaw −180, off-hand sword roll −90/yaw −180, bow yaw 170.

**Outstanding (cosmetic):** `E_RPG_EquipSlot` index 1 is named `Uncommon` — a paste slip from the rarity enum; it should be `OffHand`. The stored index is correct and `DA_Item_Shield04` was authored against index 1, so renaming the entry is display-only and needs no data migration.

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

#### Status — scaffolding COMPLETE (2026-08-01), logic outstanding

Built and saved:
- **`/Game/RPG/Items/BP_RPG_ItemInstance`** (parent `Object`) — `Template` (ref to `PDA_RPG_Item`), `Level`, `BaseDamage`, `DamagePerLevel`. The step-7 fields exist now so nothing has to be re-plumbed later.
- **`/Game/RPG/Blueprints/BPC_RPG_Inventory`** (parent `ActorComponent`) — `GearBag` (array of instance refs), `Equipped` (array of instance refs, **pre-sized to 4**, index = `E_RPG_EquipSlot`), `Materials` (**map** keyed by `PDA_RPG_Item` ref → int), `Capacity` (default **20**).

The `Equipped`-as-array workaround held up: enum-keyed maps need an enum pin type Python can't build, but Enum→Int works, so `Equipped[Slot]` is fine and mirrors the existing `StanceRightMeshes[CurrentStance]` idiom. `Materials` stayed a genuine map because **object-ref keys _are_ buildable** — which matters, since Unity keys material stacks by asset identity too.

**Still to author (Blueprint graphs — node work, not scriptable):** the 9 API functions in §4's table. Empty stubs were deliberately *not* generated: `add_function_graph` can only make parameterless empty graphs, so they'd be clutter rather than a head start.

⚠ A stray empty function graph named **`ProbeFunc`** is left on `BPC_RPG_Inventory` from capability probing. Delete it in the editor — `remove_function_graph` crashes the editor and must never be called.

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
- MCP **cannot set the `InputAction` pin** on an `EnhancedInputAction` node — creating and wiring `IA_RPG_Interact` into `IMC_RPG_Default` is a manual editor step, and unsaved IMC bindings have previously caused phantom regressions (see the dodge-system notes).
- The CC-archive assassinate prompt on `BP_RPG_Enemy` is already broken (its overlap casts `OtherActor → BP_RPG_Enemy`, which can never succeed). This new system supersedes it. **Leaving it alone** unless you say otherwise.

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

---

### Step 6 — Inventory / Character screen · **XL** — the biggest step
*Spec §7. Full layout tree with exact px/hex is in the Unity handoff README, now copied to `docs/design/`.*

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

**Hero preview stays a placeholder** — an open decision in both engines (separate always-idle preview instance vs. the live player model). See §7 below.

**Verify:** PIE — open with I, confirm pause and cursor, pick items up and watch the grid update live, equip from the UI and see the weapon change in the world behind the screen.

**Risks:** `WidgetTree` isn't Python-exposed, so this is entirely hand-authoring in the designer — by far the largest manual chunk in the plan. The reference PNGs show Classic Bright while the build targets Candy Warm; trust the palette asset, not the screenshots.

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
2. **Hero preview** (spec §7, open in both engines): a *separate preview instance* — always idle, well-lit, independent of gameplay state — versus the *live player model*. Recommendation: separate instance, for exactly those reasons. Decide once, build in whichever engine, mirror to the other.
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
| 6 | Inventory screen (+ equip-from-UI) | **XL** | all hand-authored UMG |
| 7 | Weapon instances + Forge | L | none if §2.3 is taken |

Steps 1–3 are additive and touch nothing existing. Step 4 is the integration point and the one to be careful with. Steps 6–7 are the bulk of the calendar time, almost entirely UMG hand-authoring.
