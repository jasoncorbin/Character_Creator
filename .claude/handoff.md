# Handoff — RPG Character project

> ## ▶ START HERE (next session, 2026-08-01)
>
> **The active track is the Items / Loot / Inventory-UI port, and it is mid-migration to C++.**
> The C++ module is written but **has never been compiled**. Do these in order:
>
> 1. **Confirm the Unreal editor is CLOSED.** (`Get-Process UnrealEditor`). The first build of a
>    new module cannot link while the editor holds the MCPUnreal plugin DLL.
> 2. **Generate project files, then build** the `Character_CreatorEditor` target — see
>    `docs/ItemsLoot_PortPlan.md` § "C++ migration" for the exact commands and paths.
>    ⚠ The engine is at **`E:\UE4 Projects\_UE4\UE_5.7`**, NOT `C:\Program Files\Epic Games\`
>    (the MCP `status` tool reports a stale Program Files path — do not trust it).
> 3. **Expect first-compile errors** and fix them — nothing here has been through UHT yet.
> 4. Have the user reopen the editor, then **regenerate the 9 items + rarity palette** as
>    C++-backed assets and **delete the superseded Blueprint step-1 assets** (list in the plan doc).
> 5. Resume at **step 3** of the port plan (interaction + world pickup).
>
> Steps 1–2 of the port plan are otherwise DONE. Full detail in the 2026-08-01 section below.

When the user says **"let's go" / "time to get started" / "continue"** (or anything like it), read these first, in order, before doing anything:

1. **`docs/ItemsLoot_PortPlan.md`** — **the active plan.** 7 steps, current status, decisions locked, tooling limits. Its companion spec is `docs/ItemsLootUI_MechanicsSpec_ForUE5.md` (the *what*; the plan is the *how*).
2. **`docs/RPG_BuildProgress.md`** — the running build log for the (complete) player-character track. Start at the **▶ RESUME HERE** block, then skim the latest entries.
3. **`docs/RPG_CharacterArchitecture.md`** — the approved architecture (the plan we build against).
3. **Auto-memory** `MEMORY.md` (loaded automatically) — especially:
   - `rpg-bow-setup.md` — **the ranged/bow track — most relevant right now** (charge/release, shoulder cam, what's next)
   - `rpg-stance-switch.md` — the 8-stance cycle + ApplyStance (`CurrentStance` is an int, by design)
   - `rpg-combat-combo.md`, `target-lock-system.md`, `rpg-dodge-system.md` — SingleSword systems
   - `mcp-blueprint-editing.md` — **MCP tooling limits/traps; read before any Blueprint edits**
   - `inspecting-blueprints.md` — how to inspect live (subagent for big graphs)
4. **`.claude/bugs_to_fix.md`** — deferred issues (currently all CC-track, set to IGNORE until CC work resumes).

## Prereqs to check at start
- **UE editor must be running with the MCPUnreal plugin (port 8090).** Call `mcp__mcp-unreal__status` first to confirm `editor_online` + `plugin_online`. If offline, ask the user to open the editor.
  - **EXCEPTION right now:** the pending C++ build needs the editor **closed**. Do the build first (see ▶ START HERE), then have them reopen.
  - **If the MCP tools are missing but port 8090 IS listening**, only this session's stdio server died — drive the plugin directly over HTTP instead of blocking the user. See memory `connection-drop-verify`.
- Project: `BP_RPG_PlayerCharacter`, `ABP_RPG_Player`, assets under `/Game/RPG/`. CC assets (`BP_CC_Character`, etc.) are an untouched archive — don't edit unless explicitly asked.
- **This is no longer a Blueprint-only project.** A `Character_Creator` C++ game module was added 2026-08-01 (`Source/Character_Creator/`). Gameplay data + logic go in C++ now; Blueprint stays for actors, UI and glue.
- ☠ **Never call `BlueprintEditorLibrary.remove_function_graph`** — it hard-crashes the editor. See memory `mcp-blueprint-editing`.

## Where we are (2026-08-01 — ITEMS/LOOT TRACK STARTED, C++ MIGRATION PENDING FIRST BUILD)

**New track.** The player-character work is complete; this session opened the **Items / Loot / Inventory-UI** port to reach parity with the Unity project. Planning doc: **`docs/ItemsLoot_PortPlan.md`** (7 steps, sized, with per-step integration points and risks). Read it before touching anything.

**Decisions locked by the user (revisit during testing, not before):** instance model front-loaded to step 2 · hero preview = separate always-idle instance · palette = **Candy Warm** · ~9 items authored by hand now · take the arrow-damage-from-item win in step 4 · modular-character direction out of scope · **UE 5.8 upgrade deferred to after step 5, before step 6** (needs MCPUnreal to rebuild against 5.8 — test on a COPY first).

**⚠ THE BIG CHANGE — this project now has C++.** Mid-session we measured the real cost of Blueprint-only and the user chose to migrate the data + logic layer:
- Python **can** author DataAsset classes, typed/object/array/map variables, and DataAsset instances.
- Python **cannot** create or populate User-Defined **Enums** or **Structs** (no API exists — `FEnumEditorUtils` is C++-only), cannot make **enum-typed** or **soft-ref** variables, and cannot author **graph nodes**. That last one is fatal for steps 2/4/5/7, which are mostly logic.
- Toolchain verified present: **Visual Studio Community 2026** + full engine w/ `Build.bat` + UBT at `E:\UE4 Projects\_UE4\UE_5.7`.
- → `Source/Character_Creator/` written (14 files): `RPGItemTypes.h` (5 enums + `FRarityColors`), `ItemData`, `RarityPalette`, `ItemInstance`, `InventoryComponent`, module + target files. `.uproject` declares the module. **NOT YET COMPILED — that is the next action.**

**Step 1 (data layer) — DONE in Blueprint, being re-done in C++.** Before the pivot: 5 enums (user-authored), `PDA_RPG_Item`, `PDA_RPG_RarityPalette` + `DA_RarityPalette` (both palettes, sRGB→linear), and **9 items** in `/Game/RPG/Items/Assets/` (OHS03 Sword, THS01 Sword, Spear01, Shield04, Wand01, Bow01, Mat1/2/3). All to be **deleted and regenerated** against the C++ classes.
- Mesh + rotation values were read **off the live `BP_RPG_PlayerCharacter` CDO** (`StanceRight/LeftMeshes`, `StanceRight/LeftRotations`) so equipped items land identically to the Q-cycle. Do the same when regenerating: spear roll 10, shield yaw −180, off-hand sword roll −90/yaw −180, bow yaw 170.
- **Design gap found and closed:** a one-hand sword is valid in either hand (Melee right, OffHand left for DoubleSword) and the off-hand bone is mirrored, so items carry **both** `AttachRotation` and `AttachRotationOffHand`. Not in the spec — comes from the rig.

**Step 2 (inventory) — scaffolded in BP, superseded by the C++ `UInventoryComponent`.** The C++ version fixes both of Unity's documented bugs: `Equip` moves the instance out of the bag and **returns the displaced item to it**, and `OnEquipChanged(Slot, New, Old)` carries the slot. `AddItem` returns the **count actually added** (not a bool) so a pickup can't silently lose the remainder of a partial add.

**Traps hit this session (all now in memory `mcp-blueprint-editing`):**
- ☠ **`BlueprintEditorLibrary.remove_function_graph` CRASHES the editor** (access violation in `python311.dll`). Found by elimination after a 9-op script killed the editor. **Never call it.** Treat `rename_graph`/`remove_graph` as equally unproven.
- **Batching is what made that expensive** — no incremental logging meant no signal about where it died. Pattern that works: **one risky op per `execute_script` call**, plus a `log()` helper doing `with open(RESULT,"a")` per line (open/close per write = guaranteed flush) so a crash pinpoints the last surviving step. Run probes ALONE.
- MCP `blueprint_modify add_variable` only ever sets `PinCategory` (source: `BlueprintRoutes.cpp:309-311`) — that is *why* it fakes enums as int. Use the Python `BlueprintEditorLibrary` factories instead, always.
- `data_asset_ops` is **DataTables-only** despite the name.
- `execute_script` returns no stdout — have the script write a result file and Read it.

**Also this session:** `docs/design/` now holds the Candy Cloud handoff (README, 3 mock HTMLs, 4 reference PNGs) copied from the Unity repo at `E:\Unity\Unity_Procedural_Level_Creator\`. Note the PNGs render **Classic Bright**, not the chosen Candy Warm — trust the palette asset, not the screenshots. `.mcp.json` also gained an `unreal-mcp` HTTP entry pointing at `127.0.0.1:8000/mcp`; that is Epic's **UE 5.8** in-editor MCP server and will not connect until the engine is upgraded. Harmless until then.

## Where we are (2026-07-18, session 3 — RANGED CROSSHAIR + AIM-PARALLAX FIX COMPLETE)
**PIE-confirmed by user:** crosshair shows/hides correctly across all stances, free-aim arrows land where the crosshair points at close range, lock-on unaffected. One cosmetic deferred (below).

**Reticle — DONE:**
- **`WBP_RangedReticle`** (`/Game/RPG/UI/`): simple UMG crosshair (CanvasPanel + 5 Images = 4 bars + center dot, anchored center). **Built by USER in the UMG designer** — `unreal.WidgetTree` is NOT Python-exposed in 5.7 (can create the empty shell via `WidgetBlueprintFactory`, but can't author the tree from script). I created the empty asset + the `ReticleWidget` (UserWidget-ref) member var via Python.
- **Show/hide wired in `ApplyStance`** (appended after the Bow `Set Visibility` tail `7CC6F08D`): `IsValid(ReticleWidget)` → not-valid: `CreateWidget(WBP_RangedReticle, GetPlayerController 0)` → `Set ReticleWidget` → `AddToViewport` → Branch; valid → Branch directly (exec merge). `Branch(StanceIsRanged[CurrentStance])` → then `SetVisibility(Visible)` / else `SetVisibility(Collapsed)`. **Visibility-toggle (not add/remove)** so cycling 6↔7 doesn't double-add. User placed the 12 nodes; I wired 8 exec + 8 data pins via MCP.
- **TRAP:** `GetArrayItem` fed by `connect_pins` stays **Wildcard** (same as promotable operators) → "type undetermined" compile error; user resolved by Refresh Nodes / re-drag from the typed pin. And `blueprint_query inspect` reports arrays by inner type (StanceIsRanged shows "bool" but is bool[8] — verify via CDO).

**Aim parallax — FIXED (was the must-fix):** free-aim SpawnActor `FD0B40CD` now aims muzzle→camera-ray hit. Spliced `LineTraceByChannel`(Visibility, bIgnoreSelf) into the exec before the free-aim spawn: Start = camera `GetWorldLocation 12950C33`, End = existing camera-forward `51961C77` (CamLoc+Fwd*5000); `OutHit → BreakHitResult CABF2223 → Select CF3F8A15 (bBlockingHit ? ImpactPoint : TraceEnd) → FindLookAt E12AA98A.Target` (replaced the old direct `51961C77→Target`). New node GUIDs: LineTrace `0BF25653`, Break `CABF2223`, Select `CF3F8A15`. User placed the 3 nodes + internal drags; I wired the interface. Lock-on path (`ActorToTargetLock`) untouched.

**DEFERRED — USER DECISION 2026-07-18 "leave as-is for now":** while LOCKED-ON in a ranged stance, the center crosshair sits at the enemy's **feet** (lock cam aims `enemy−100Z` `AC98CB28`, by design) while the arrow homes to the body — cosmetic mismatch, everything else fine. Later fix = hide the reticle while locked (gate on `ranged AND !IsValid(ActorToTargetLock)`, re-evaluated in the lock/unlock handlers since `ApplyStance` only runs on BeginPlay+Q) OR a dedicated on-target lock bracket. **Do NOT touch the −100Z lock framing.**

**Both assets saved to disk, uncommitted on `main`** (user commits manually).

## Where we are (2026-07-16, session 2 — RANGED CHARGE/RELEASE + SHOULDER CAM COMPLETE)
**All PIE-confirmed by user: "all items are working as intended"** — bow + wand charge/release, single discrete shots, shot animation plays, melee stances 1-5 unaffected, no cross-stance misfire.

**Ranged rework (combos removed from stances 6+7) — DONE:**
- New vars: **`StanceIsRanged` bool[8]** ([6],[7]=true) + **`bIsCharging`** bool. Made via Python `add_member_variable` + `get_array_type(get_basic_type_by_name("bool"))`, values on CDO.
- **`IA_RPG_Attack`.Started → Branch `74CBC81F`** (cond = `StanceIsRanged[CurrentStance]`, GET `9F96A354` ← Get StanceIsRanged `0D80AE95`): **then** → PlayAnimation `2CBA2077` (bow-mesh draw) → Set bIsCharging=true `893F60CE`; **else** → combo Branch `0C4E4079` (melee path UNCHANGED).
- **`.Completed` → Branch `C01342A0`** (cond = Get bIsCharging `F9965AC7`) → **pasted Play Montage** (a copy of light `FC66BA61` + its StanceComboLight[CurrentStance] GET chain + Get Mesh + MeleeHit call) → Set bIsCharging=false `4AA26FF6`.
- **Arrow fires from the montage's `Hit` notify** (pasted PlayMontage.OnNotifyBegin → MeleeHit → gate → IsValid `5F083F66` → SpawnActor), NOT from `BowFire` on release — gives frame-accurate release timing. **`BowFire` (`AF52F272` / call `16CD1799`) is now ORPHANED dead code** — kept deliberately, not deleted.
- **MeleeHit gate generalized:** Equal(==7) `711C1B9F` DELETED; Branch `767BFFD5`.Condition ← `StanceIsRanged[CurrentStance]`; its **True→IsValid wire KEPT** (that's the spawn). else = melee overlap, unchanged.
- **Pasted montage's StartingSection left UNCONNECTED** = starts at Combo01. Deliberately does NOT carry the ComboIndex Select `89BB3C17` (that Select is SHARED by light+heavy — don't rewire it). No section auto-chaining observed in PIE.
- **No AND node on release** — `bIsCharging` is only ever set true on the ranged path, so it already implies ranged. Dodges the promotable-operator wildcard trap.

**Over-the-shoulder camera — DONE.** User chose **permanent, all stances** ("true RPG fashion") — NOT RMB-toggle, NOT charge-only. So it's a pure component default: `CameraBoom.socket_offset = (0,60,25)` via Python template + save. Arm 350, FOV 90, lag 15 unchanged. **This supersedes the old RMB `IA_Aim` plan — no new Input Action needed.** Tuning: Y=right, Z=height, target_arm_length=tightness.

**Traps hit this session (read before similar work):**
- **STALE EDITOR:** a BP editor open during a Python var-add shows the new var with the WRONG type (bool array read as plain bool, no "Get (a copy)" option). Close+reopen the BP tab. Check this FIRST when a Python-made var "won't connect".
- **Set-bool literal slips:** `Set bIsCharging` `4AA26FF6` was left at **true** → flag latched, every release fired an arrow in any stance. Fixed via MCP `set_pin_value`. Check both Set-bool checkboxes first when charge/release misbehaves.
- **IMC reads:** Python reports `IMC_RPG_Default` as 0 mappings (API doesn't round-trip). **Use MCP `input_ops get_bindings`** — it reads all 10 correctly. Neither exposes per-mapping triggers. `IA_RPG_Attack` has 0 triggers/0 modifiers → default behavior fires `Completed` on release (confirmed in PIE).

**(Previous, still true) Bow flying-arrow polish COMPLETE, PIE-confirmed working.** Ranged (bow) now fires a real `BP_Arrow` projectile that damages enemies and credits the player.
- `BP_Arrow` (`/Game/RPG/Blueprints/BP_Arrow`): CollisionSphere root + ArrowMesh + ProjectileMovement (speed 6000, gravity 0.4). Hit graph: `OnComponentHit → ApplyDamage(OtherActor) → DestroyActor`. On **BeginPlay** it now calls `CollisionSphere.IgnoreActorWhenMoving(GetInstigator, true)` → **no self-hit**.
- Player BP fires arrows from `MeleeHit` → stance Branch → `IsValid(ActorToTargetLock)`: valid → SpawnActor aimed at locked target (`296912CA…`), else → SpawnActor down camera-forward (`FD0B40CD…`). Both spawn with **Instigator = Self** (`A863D9B5…`) so kills are credited and the arrow knows who to ignore. **(NB: that Branch's condition was `CurrentStance==7` when this was written — session 2 replaced it with `StanceIsRanged[CurrentStance]`. See the session-2 section above.)**
- **Target-lock was accidentally broken and re-fixed this session:** `AC98CB28` (Tick FindLookAt → SetControlRotation) is the LOCK-ON look-at, NOT a bow node. Its `Start` must be the **camera** world location (`8119523F`), `Target` = enemy(−100Z). Do not wire bow muzzle/end into it.
- **Pre-existing bug — USER DECISION 2026-07-16: "leave for now, will fix later." Do NOT touch it unprompted.** `BPC_PlayerStats.Decrease Health` throws `Accessed None … As Dummy` whenever the player takes damage from a non-"Dummy" causer (was spamming because of the old self-hit).

## ~~Next up — CROSSHAIR / RETICLE for ranged stances~~ ✅ DONE 2026-07-18 (see session-3 block at top)
_(kept for reference — the reticle + aim-parallax fix described below were completed in session 3; only the lock-on-crosshair cosmetic is deferred.)_
**Goal:** show a crosshair while in a ranged stance (6 wand / 7 bow) now that the shoulder cam makes one readable.

**Must-fix as part of this pass — AIM PARALLAX (flagged, unfixed):** free-aim arrows spawn at the bow muzzle (`NockArrow`) but travel along **camera-forward**, and `CameraBoom.socket_offset` now sits **60u right**. So a center-screen crosshair is **NOT** where arrows land at close range — they converge toward it with distance. Proper fix: **line-trace from the camera** to find the real aim point, then `FindLookAtRotation(muzzle, hitPoint)` for the free-aim SpawnActor (`FD0B40CD`) instead of the camera-forward End node `51961C77` (= CamLoc + Fwd*5000). Do this WITH the reticle, not after — it's what makes the crosshair honest.

**Sketch:** `WBP_RangedReticle` UMG widget; add/remove to viewport in `ApplyStance` gated on `StanceIsRanged[CurrentStance]` (the array already exists and ApplyStance already runs on BeginPlay + every Q switch — the natural hook). Lock-on path already aims at `ActorToTargetLock` and is unaffected by parallax.

**Deferred / v2 ideas (agreed, not scheduled):**
- **True held-draw pose:** currently the shot montage plays on RELEASE; there is no hold pose while charging (pack ships only full shot-cycle combos, no draw/hold/release clips). Real charge feel = user authors each ranged montage into **Draw(holds, no next-link) + Release(has fire notify)** sections, then wire `Montage_JumpToSection("Release")` on release. Montage section editing is manual/user-side.
- **Charge-dependent power** (draw time → damage/speed) — `bIsCharging` + a timer would carry it; nothing built.
- **Stance-switch mid-charge:** hold LMB in bow, press Q to a melee stance, release → `bIsCharging` still true. Harmless now (release just plays the melee montage) but a `Set bIsCharging=false` in `ApplyStance` would tidy it.
- **Wand muzzle** still = placeholder (fires `BP_Arrow` from the bow's `NockArrow` at the left hand); wants its own cast VFX/projectile.

**Key attack-area GUIDs (player EventGraph), for fast pickup — UPDATED 2026-07-16 session 2:**
- Input `IA_RPG_Attack` = `F6312A9641926A64D95583B102DCDF36` — **Started** → ranged Branch `74CBC81F49C89ECEDC52BDB1DCC45F0A`; **Completed** → release Branch `C01342A04B8C2CC5B74BBCB15C0B9E9D`. (0 triggers on the IA → Completed fires on release.)
- **Ranged gate** Branch `74CBC81F…`: cond ← GET `9F96A3544102182C04FAAF85932C13C3` (Array ← Get StanceIsRanged `0D80AE9547159D501C6C07BB9FF5F337`, Index ← Get CurrentStance `1635B8EC435D0B8B1D4AE9869ED3942B`). then → PlayAnimation `2CBA2077411669089513359B7FB05B0F` (bow draw; self ← Get Bow `38F289DB4CFEF45714233D830D374388`, anim ← GET `49C6813A4D5204B42218F393DB98180B` = BowDrawAnims[ComboIndex]) → Set bIsCharging=TRUE `893F60CE40477E21C9901BBA57C94339`. else → melee combo Branch `0C4E407944CA7027E5F1BB97AD88B54F`.
- **Release gate** Branch `C01342A0…`: cond ← Get bIsCharging `F9965AC74DD3220BBD3DF9AA0FFDB2AC`. then → the **pasted ranged Play Montage** (GUID not captured — it's the montage node fed by this Branch) → Set bIsCharging=FALSE `4AA26FF64B29BB2D2B4A1FA52662F300`.
- Melee combo split Branch(ComboIndex<3) = `0C4E4079…` (cond ← `integer<integer` `5CA21790…`, A ← Get ComboIndex `11E20236…`, B=3); Set ComboIndex(+1%5) = `D4158D41435DB88FF09188A553056271`
- Light PlayMontage = `FC66BA6142F7FC20A94DA890D3F78DE9` (MontageToPlay ← GET `DF3F6244…` ← Get StanceComboLight `47E3EB61…` + Get CurrentStance `0707FDC8…`; mesh ← `F8916CCA…`; OnNotifyBegin → MeleeHit call `257B602C…`; then → Set ComboIndex `D4158D41…`). Heavy = `718691F04BE128413D42A4A5588F45D7` (MontageToPlay ← GET `B1B71583…` ← Get StanceComboHeavy `4168B27D…`).
- **StartingSection Select `89BB3C1745CA1F01AEE05B9AD530341B` is SHARED by light+heavy** (options Combo01-05, Index ← Get ComboIndex `BDB7A2F2…`), via Reroute `CB708035…` on the light side. **Don't rewire it**; the ranged montage copy intentionally omits it.
- `MeleeHit` event = `A045B4434142618DCA0833A9BC990EB7`; its stance Branch = `767BFFD54B1BE7F590EC02A92745A493` — **cond is now `StanceIsRanged[CurrentStance]`; the old Equal(==7) `711C1B9F…` is DELETED.** then → `IsValid` `5F083F664519D19135419CBD710F6E81` (KEPT — this is the arrow spawn); else → Sphere Overlap `ADD4D005…` → ForEach `3B94B516…` → Branch `56E525D7…` → ApplyDamage `82D20EED…`.
- `IsValid` `5F083F66…` (InputObject ← Get ActorToTargetLock `172FCECE…`) → Is Valid → Reroute `659BF5B4…` → SpawnActor lock=`296912CA4C545D5A0671E39CA02811D9`; Is Not Valid → Reroute `AC71C7BD…` → SpawnActor free=`FD0B40CD423334D26AB531892CF2EC16`. Self(instigator)=`A863D9B546055D97865B62BF10CBB00E`. Camera-forward End math = `vector+vector` `51961C77…` (**this is what the parallax fix replaces**).
- `BowFire` event = `AF52F2724BB2F09B91E7B6AF33D1A612`, call node `16CD17994C0B84080768589B6EFDBE89` — **now ORPHANED/dead** (release fires via the montage notify instead). Kept on purpose.
- Target-lock (DON'T confuse with bow): `AC98CB28…` FindLookAt→SetControlRotation is the **lock-on Tick look-at**; Start MUST be camera world loc `8119523F…`, Target = enemy(−100Z).

**Not committed to git** — user commits manually (never offer to). BP_Arrow + player BP changes are saved to disk, uncommitted on `main`.

## Working pattern that works here
Build automatable parts via MCP/Python (variables, array data, node spawning, pin connects); **guide the user through manual graph wiring** for things MCP can't configure (object-ref pins, EnhancedInputAction nodes, function-call node targets, custom-function calls, AnimGraph node internals); then **verify via `blueprint_query` (use a subagent for big graphs)** before the user PIE-tests. Confirm odd-looking BP choices with the user before assuming they're bugs.
