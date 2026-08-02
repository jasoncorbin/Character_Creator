# Handoff — RPG Character project

> ## ▶ START HERE (session 4 — written end of 2026-08-01, session 3)
>
> **STEPS 1–4 ARE DONE.** Step 4 — the step the plan calls the fiddliest and highest-risk — is
> complete and **PIE-verified end to end**. `BP_RPG_PlayerCharacter` now inherits from a C++ base.
>
> ### ⛔ FIRST TASK: the `BPC_PlayerStats` health-bar bug — diagnosed, approach agreed, NOT started
>
> Root cause is **fully established** (don't re-investigate, go straight to the fix).
>
> **Symptom:** every time an RPG enemy takes damage, PIE logs 6 errors —
> `Accessed None trying to read (real) property As Dummy in BPC_PlayerStats_C`, on nodes
> `Set Percent` and `SetText (Text)` in `Decrease Health`.
>
> **Root cause:** `Decrease Health` picks its target widget with a **`Select`** node keyed on
> `isPlayer`:
> - Option 0 (false) ← `As Dummy` → `Health Bar UI` → `HealthBar` / `HelathText`
> - Option 1 (true) ← `BP CC Character` → `Hud Widget` → `Health_Bar` / `HelathText`
>
> `BPC_PlayerStats` is owned by **three** actors: `BP_CC_Character` (isPlayer=true),
> `Dummy` (As Dummy set), and **`BP_RPG_Enemy` — which sets NEITHER**, because it is not a
> `Dummy`. So for an RPG enemy the Select resolves to Option 0 and dereferences a null three
> times (`As Dummy` → `Health Bar UI` → `HealthBar`) = 3 errors × 2 nodes = the 6 per hit.
> NB `BP_RPG_PlayerCharacter` does **not** have this component at all — the player is not involved.
>
> **This is not just log spam: `BP_RPG_Enemy`'s health bar has never updated.** It owns a
> `Health Bar Wigget` WidgetComponent and a `Health Bar UI` variable, but this code can only
> talk to a `Dummy`. ⚠ Ask the user to confirm they've never seen an RPG enemy bar move.
>
> **Second, latent defect:** a Blueprint `Select` evaluates **ALL** option chains eagerly, so the
> Dummy chain is dereferenced even when `isPlayer` is true. **A `Select` can never be a null guard.**
>
> **Agreed fix (user approved the approach, not yet the go-ahead to edit):** stop switching on
> concrete owner classes. Both `Dummy` and `BP_RPG_Enemy` already carry a `Health Bar Wigget`
> **WidgetComponent**, so on BeginPlay have the component ask its **owner** for that widget and
> cache the `ProgressBar` + `TextBlock` refs. Then `Decrease Health` uses a **Branch** (not a
> Select) + `IsValid`, so only one path is ever evaluated.
> Fixes all three owners at once, future enemies work for free, and **all edits stay inside
> `BPC_PlayerStats`** — no changes to `Dummy`, `BP_CC_Character` or `BP_RPG_Enemy`.
> Cost: variables addable via Python; the graph restructure needs the user to place ~6 nodes
> (Branch ×2, IsValid, duplicate `Set Percent`/`SetText` for the enemy path), then I wire pins.
>
> ⚠ `BPC_PlayerStats` is **CC-archive** code the user had previously frozen
> ("leave for now" 2026-07-16). It is in scope now **only because the user asked** — do not
> extend the edit beyond this fix.
>
> ### Then: step 5 — loot tables + dropper
> Needs an `OnDeath` dispatcher on `BP_RPG_Enemy` (fire on the TRUE branch of `IsPlayerDead?`,
> **before** the 2 s delay). Known landmine: an arrow kill runs the whole death chain inside a
> collision callback — spawn deferred, don't mutate actors during the overlap.
>
> ---
>
> **The module builds green.**
>
> ### ☠ Read this before trusting any "verified" claim in this file
> Session 2 recorded step 3's E binding as PIE-confirmed. **It was gone at the start of session 3**
> and the pickup path was silently dead. Root cause: `cc_step3_mapkey.py` called
> `save_loaded_asset`, then "verified" by re-reading the **in-memory `UObject`**, which of course
> still held the change. The save never reached disk (that session was throwing *"Unable to Check
> Out From Revision Control!"* dialogs) and the return value was never checked.
> **A read-back proves nothing unless it comes from disk.** Confirm asset writes three ways:
> `EditorLoadingAndSavingUtils.get_dirty_content_packages()` empty · `.uasset` mtime/size changed ·
> `git status` shows it modified. (`EditorAssetLibrary` has **no `reload_asset`** in 5.7, and
> `unreal.Package` has no `is_dirty()` — the file/git checks are the only ground truth.)
>
> ### Verified 2026-08-01 session 3 — all green
> - **E → `IA_RPG_Interact`** mapped, stray `IA_Interact` gone, `InputTriggerPressed` present.
>   Confirmed via disk mtime + git + zero dirty packages, then **PIE-confirmed one press = one
>   pickup** by the user.
> - **`UInventoryComponent` API: 32/32 checks pass** (`Saved/CC_Probe/inv_api_test.json`). This
>   closes two of the three untested paths — **partial add** (ask 5 with 2 free → returns 2) and
>   **bag full** (returns 0, `Unequip` refuses rather than destroying gear). Also covers materials
>   uncapped/spend/key-removal-at-zero, equip moves the instance out of the bag, the displaced item
>   returns to the bag, the same instance can't hold two slots, the OffHand exception accepts an
>   OHS sword (stance 4), and a bow is rejected from Melee.
>   Re-run it any time — it uses **transient objects, touches no level and dirties nothing**.
>
> ### The one path still unverified — priority arbitration
> **`AWorldItem::GetInteractPriority_Implementation()` returns a hard-coded `Pickup`**
> (`WorldItem.cpp:176`), so *every interactable that currently exists is priority 10*. The priority
> term of the arbitration cannot be exercised by shipping content at all — it only starts to matter
> when a second interactable kind lands (a chest at `Open`, an assassinate prompt).
> A test prop is **already placed and waiting in `Lvl_RPG_Test`**: actor `ZZ_ArbTest_Priority50`
> at `(360, 0, 75)`, a plain Actor carrying a bare `UInteractableComponent` at `DefaultPriority=Open(50)`,
> radius 150, sitting 60 uu from the sword pickup (priority 10) — so both overlap the player at once
> and priority must beat distance. Expected: the prompt reads
> **"ARB TEST - priority 50 should win"**, and E fails honestly (a bare component has no
> implementer, so `ResolveInteract` returns false by design).
> **It is UNSAVED — do not save the level.** Delete the actor when done.
> ⚠ If that prompt never appears, the instance component did not survive world duplication into
> PIE; redo the prop as a small Blueprint class instead of an instance component.
>
> ### Cosmetic, still open
> `AWorldItem::Palette` is unset on all 4 placed pickups, so `OnItemVisualsApplied` gets white.
> Only matters once something consumes the rarity tint.
>
> ### Then: **step 4 — equip consumers.** The plan calls it the fiddliest step. Read
> `docs/ItemsLoot_PortPlan.md` §5 step 4 before starting; it touches the already-working
> `ApplyStance`, and the rule there is *refactor to source through resolver functions, do not
> rewrite* — the Q dev-cycle must keep working with an empty inventory.
>
> ⚠ The engine is at **`E:\UE4 Projects\_UE4\UE_5.7`**, NOT `C:\Program Files\Epic Games\`.
> That folder is a connected Cowork folder — **read engine headers rather than inferring APIs.**
> It paid for itself twice in one session (see "Engine-API corrections" below).
>
> **Rebuild command.** Editor must be CLOSED. Add the project-file regeneration line only when
> files are added or removed.
> ```powershell
> $UE = "E:\UE4 Projects\_UE4\UE_5.7"
> $P  = "E:\UE5 Projects\Character_Creator\Character_Creator.uproject"
> & "$UE\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="$P" -game -rocket -progress   # only if files added/removed
> & "$UE\Engine\Build\BatchFiles\Build.bat" Character_CreatorEditor Win64 Development -project="$P" -waitmutex
> ```
>
> ⚠ **Uncommitted, and the editor's Git provider has already STAGED 19 asset deletions**
> (`git rm`) from the regeneration pass. Run `git status` before committing. User commits
> manually — never offer to.

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
  - **If the MCP tools are missing but port 8090 IS listening**, only this session's stdio server died — drive the plugin directly over HTTP instead of blocking the user. See memory `connection-drop-verify`.
  - ☁ **In a cloud Cowork session there are no `mcp__mcp-unreal__*` tools at all, and the HTTP
    fallback is unavailable too** — the container cannot reach the user's `127.0.0.1:8090`.
    Adding `mcp-unreal` to `claude_desktop_config.json` did **not** make it proxy. **Do not burn
    turns on this.** Use the editor-Python fallback below; it works well.

### ☁ Editor-Python fallback (proven, 2026-08-01 — use this whenever the MCP is absent)
Write a script, commit it to `Content/Python/`, have the user run it from the editor's
**Output Log → command box (mode `Cmd`)**:
```
py "E:/UE5 Projects/Character_Creator/Content/Python/<script>.py"
```
`execute_script`-style calls return no stdout, so **the script must write a result file** to
`Saved/CC_Probe/` which is then staged back and read. Two working examples are checked in:
`cc_step2_dump.py` (read-only probe) and `cc_step2_author.py` (build → verify → delete → move).

The shape that made a destructive pass safe, and is worth copying:
1. a **separate read-only probe first** — dump the data AND the referencer graph of everything
   you intend to delete, then decide with real numbers rather than assumptions;
2. build new assets into a **throwaway staging folder**, never over the originals;
3. **read every field back off the saved asset** and compare;
4. delete only after 100% verification, **each delete gated on zero remaining referencers**;
5. move staging onto the real paths last.
Log per line with an open/close per write, so a crash names its last surviving step.
- Project: `BP_RPG_PlayerCharacter`, `ABP_RPG_Player`, assets under `/Game/RPG/`. CC assets (`BP_CC_Character`, etc.) are an untouched archive — don't edit unless explicitly asked.
- **This is no longer a Blueprint-only project.** A `Character_Creator` C++ game module was added 2026-08-01 (`Source/Character_Creator/`). Gameplay data + logic go in C++ now; Blueprint stays for actors, UI and glue.
- ☠ **Never call `BlueprintEditorLibrary.remove_function_graph`** — it hard-crashes the editor. See memory `mcp-blueprint-editing`.

## Where we are (2026-08-01, session 2 — STEPS 1 + 2 LIVE IN C++)

**The data layer is real in the editor.** Ran without any MCP connection, via two Unreal-Python
scripts (both checked in at `Content/Python/`, both re-runnable):

- `cc_step2_dump.py` — read-only. Confirmed all 12 C++ types registered and all 7 enums in the
  right entry order; dumped the 9 Blueprint DataAssets, the palette, the player CDO stance arrays,
  and the referencer graph of every delete candidate.
- `cc_step2_author.py` — built 10 assets into `/Game/RPG/_Staging`, verified every field off the
  saved asset (**10/10 clean**), deleted **19** Blueprint assets, moved the new ones onto the real
  paths, dropped staging. **0 errors.**

**Live now:** 9 `UItemData` assets in `/Game/RPG/Items/Assets/` and `URarityPalette` at
`/Game/RPG/Data/DA_RarityPalette`.

**The palette needed no migration.** `URarityPalette`'s constructor calls
`ResetToDesignDefaults()`, which authors both tables and does sRGB→linear in code. Verified its
output matches what the Blueprint asset held (`DBD5C9` → `(0.708, 0.665, 0.584)`).

**🐛 Data bug found and fixed — `DA_Item_Bow01`.** Yaw 170 was in `AttachRotationOffHand` with
`AttachRotation` at zero. The value is right (it comes from `StanceLeftRotations[7]`, hence the
off-hand filing), but the bow's slot is `Ranged`, so `GetAttachRotationForSlot(Ranged)` returns
`AttachRotation` — the 170 would never have been read. Step 4 would have mounted the bow 170° off
and it would have looked like a rig problem. `ItemData.h` says `AttachRotation` is the field used
"when mounted in the RIGHT hand (**or on the bow rig**)". Moved. Every other item cross-checked
against the CDO and matched.

**`BPC_RPG_Inventory` had zero referencers** — never attached to `BP_RPG_PlayerCharacter`. That
was the one genuinely risky deletion, and the probe settled it beforehand rather than after.
`E_RPG_Stance` was explicitly kept.

**`EInteractPriority` — resolved: keep it.** The only argument for dropping it was the cost of
hand-authoring user-defined enums in Blueprint, and the C++ pivot removed that cost.

**`UInventoryComponent` compiles but has never run.** ~~Nothing instantiates it yet.~~ It runs now —
step 3 put it on the player and pickups go into it.

---

## Step 4 — equip consumers — DONE, PIE-VERIFIED (2026-08-01, session 3)

**The plan's highest-risk step. It touched `ApplyStance` and `MeleeHit` and did NOT regress the
Q dev-cycle.** The user chose to close the BP/C++ seam properly rather than work around it.

### `BP_RPG_PlayerCharacter` now inherits from `ARPGPlayerCharacter` (C++)

New files: `Source/Character_Creator/Player/RPGPlayerCharacter.{h,cpp}`.

**Six stance properties moved down from Blueprint**, named IDENTICALLY to the old BP variables:
`CurrentStance`, `StanceRight/LeftMeshes`, `StanceRight/LeftRotations`, `StanceIsRanged`.
All `EditAnywhere`, so they stay tunable in the Details panel with no rebuild.
**Deliberately left in Blueprint:** the 2 combo tables, 4 dodge tables, `BowDrawAnims`,
`ComboIndex`, `ActorToTargetLock`, `IsDodging`, `bIsCharging`, `ReticleWidget` — C++ doesn't need
them, and every name left in BP is one less collision to handle. 6 collisions instead of 18.

**☠ THE REPARENT RECIPE — UE resolves name collisions ITSELF (proven, then done for real):**
`BlueprintEditorLibrary.reparent_blueprint(bp, unreal.Foo.static_class())` → `compile_blueprint`
→ `save_asset`. UE **removes** the colliding BP variable, **transfers its authored value onto the
inherited C++ property**, and **rebinds all 189 nodes by name**. Logs one
`LogK2Compiler: ValidateVariableNames … is already taken by …` **warning** per collision — warnings,
NOT errors. There is **no `remove_member_variable`** in `BlueprintEditorLibrary` and you don't need one.
**Proof the values transferred** (rather than reverting to the C++ constructor's `SetNum(8)` blanks):
bow yaw 170, spear roll 10, shield yaw −180, off-hand sword −90/−180, `StanceIsRanged[6,7]=true`
all read back correct. **24/24 verification checks passed before saving.**
Safety shape that made this cheap: dump the CDO first (`Saved/CC_Probe/player_cdo_dump.json`),
**reparent a DUPLICATE first** to answer "do the nodes rebind?", then do the real one.

### The API (all `BlueprintPure` on the character)

| Function | Behaviour |
|---|---|
| `ResolveRightHandMount(OutMesh, OutRotation)` | Melee first, then Ranged (a **wand is Ranged but right-handed**) |
| `ResolveLeftHandMount(OutMesh, OutRotation)` | OffHand slot |
| `ResolveBowRigMount(OutMesh, OutRotation)` | Ranged w/ `MountPoint==BowRig`; **null OutMesh = "leave the Bow component's mesh alone"** |
| `GetMeleeDamage()` / `GetRangedDamage()` | Equipped item's damage, else `UnarmedMeleeDamage`(20) / `DefaultRangedDamage`(30) |
| `DeriveStanceFromEquipment()` | The 8-row table in text, switching on `Category`. **Ranged beats melee.** Returns `CurrentStance` unchanged when nothing is equipped, so an empty bag never fights the Q-cycle |
| `OnEquipmentChanged(Slot)` | `BlueprintImplementableEvent` — the single seam; BP wires it to `ApplyStance` |

**Resolver contract: the OUT params are ALWAYS written with the value to use** — equipped item
wins, stance table is the fallback — so the graph needs no Select/Branch of its own.
**⚠ They return `void` on purpose.** An earlier revision returned `bool` ("came from an item?")
and that was a trap — see the Python out-param note in memory `mcp-blueprint-editing`.

### Graph changes (user placed nodes, I wired pins — the usual split)

- **`ApplyStance`**: the two `Get Stance*Meshes/Rotations → GetArrayItem[CurrentStance]` chains
  were replaced by **one resolver node per hand**, feeding `Set Static Mesh.NewMesh` and
  `Set Relative Rotation.NewRotation`. 4 pins rewired (disconnect first — `connect_pins` is additive).
  **Bow visibility (`CurrentStance == 7`) and the reticle (`StanceIsRanged[CurrentStance]`) were
  left untouched** — both still work, because the stance bridge writes `CurrentStance` and they now
  read the inherited C++ properties.
- **EventGraph** (189 → 194 nodes): `On Equipment Changed` → `Apply Stance`;
  `Get Melee Damage` → the `Apply Damage.BaseDamage` literal 20 in `MeleeHit`;
  `Get Ranged Damage` → the `Damage` pin on **both** `SpawnActor BP_Arrow` nodes.
  (I set **`Expose on Spawn` on `BP_Arrow.Damage`** so that pin exists. `Damage` is an **int**, default 30.)

### PIE verification — driven live from Python, no debug key needed

`Saved/CC_Probe/pie_equip_test.json`. Meshes read off the **real components on the running pawn**:

| Step | Stance | Melee dmg | Right hand | Left hand | Bow |
|---|---|---|---|---|---|
| empty bag | 1 | 20 | OHS03 *(table)* | — | hidden |
| THS01 → Melee | **2** | **25** | **THS01** | — | hidden |
| + Bow → Ranged | **7** | 25 | THS01 | rot 170 | **visible** |
| unequip bow | **2** | 25 | THS01 | — | hidden |
| OHS + Shield | **3** | **15** | OHS03 | **Shield04 @ −180** | hidden |
| all off | **0** | 20 | — | — | hidden |

Also confirmed: ranged damage 30→18 with a bow; **unequip returns items to the bag**
(gear count 0→1→2→4), which is the Unity bug the C++ API was reshaped to prevent.

⚠ **Item balance, not a bug:** `UnarmedMeleeDamage` is 20 but `DA_Item_OHS03_Sword` is 15, so
equipping the starter sword *lowers* damage. Spear=20, THS=25.

### Still unverified / loose ends

- **The two damage SINKS.** `GetMeleeDamage`/`GetRangedDamage` return correct values and the node
  count is consistent with the wiring, but nobody has confirmed a real swing consumes
  `GetMeleeDamage`, or that a spawned arrow carries 18. **Do a live combat check.**
- **Priority arbitration** (step 3's last untested path) — still not run. Test prop
  `ZZ_ArbTest_Priority50` was spawned into `Lvl_RPG_Test` **unsaved**; it is probably gone now.
  Note `AWorldItem::GetInteractPriority_Implementation()` returns a hard-coded `Pickup`
  (`WorldItem.cpp:176`), so **every interactable in the project is priority 10** and the priority
  term cannot be exercised without inventing a second interactable kind.
- **3 dead nodes remain in `ApplyStance`** (33 nodes; a full cleanup would be 30). One leftover
  `Get Stance… → GET → Get CurrentStance` chain. They *look* connected because they're wired to
  each other — what's dead is the GET's **Output** pin. Harmless (pruned at compile).
- **Pre-existing, unrelated:** `Content/CharacterCreator/CC_Attack/Assassination_01.uasset` is
  **corrupt** — "Invalid value for PACKAGE_FILE_TAG at start of file". Nothing depends on it.

### Build notes

`Build.bat Character_CreatorEditor Win64 Development` — ~8-14 s incremental, UHT clean under
`-WarningsAsErrors`. Adding the new `Player/` folder needed **no** `Build.cs` change (the
`PublicIncludePaths.Add(ModuleDirectory)` line carried it) and **no** project-file regeneration.
⚠ **Whether an incremental build tolerates an OPEN editor is still UNPROVEN** — one build
succeeded with the editor apparently up, but the user was closing it at the same moment, so the
test was inconclusive. Keep closing the editor for builds until someone actually establishes this.
A new `UCLASS` needs an editor restart to register regardless.

---

## Step 3 — interaction + world pickup — DONE, PIE-CONFIRMED (2026-08-01)

**8 new files in `Source/Character_Creator/Interaction/`:**

| File | What |
|---|---|
| `RPGInteractable.h/.cpp` | `IRPGInteractable` — `GetInteractPriority` / `GetPromptLabel` / `IsInteractEligible` / `Interact`. The **.cpp is deliberately empty** — see the C2084 trap below. |
| `InteractableComponent.h/.cpp` | `UInteractableComponent : USphereComponent`, 150 uu. IS the trigger. Registers itself with any `UInteractorComponent` that overlaps. Forwards every query to the owner's interface if implemented, else to its own `DefaultPriority`/`DefaultPromptLabel`. |
| `InteractorComponent.h/.cpp` | On the player. Arbitrates one winner, `TryInteract()` for the input binding, `OnActiveInteractableChanged` / `OnInteractAttempted` delegates. |
| `WorldItem.h/.cpp` | `AWorldItem` — the pickup. Static + skeletal display (bows are skeletal), placeholder cube for meshless items, optional spin, `SpawnWorldItem` deferred-spawn helper for step 5's loot drops. |

**Wired and live:** `Inventory` + `Interactor` components on `BP_RPG_PlayerCharacter`;
`IA_RPG_Interact` in `/Game/RPG/Input/`; `IMC_RPG_Default` maps it to **E**; 4 test pickups in
`Lvl_RPG_Test` (sword, shield, bow, Mat1 ×5) anchored on the PlayerStart.
Graph edit (manual, done): `EnhancedInputAction IA_RPG_Interact` → `Triggered` → `Interactor` →
`Try Interact`.

### Design decisions worth not re-litigating

- **Arbitration is fully ordered: priority desc → nearest → lowest object name.** Unity used a
  `HashSet` and its own source flags the iteration order as unreliable, so equal-priority ties
  resolved differently run to run. The name term only fires at identical priority *and* identical
  distance; it exists so there is no undefined outcome at all.
- **A full bag does NOT hide the prompt.** `IsInteractEligible` deliberately ignores capacity and
  the press fails loudly instead. Hiding it reads as "there is nothing here", which is a worse
  lie than "that didn't work".
- **`Interact` treats `AddItem`'s return as a count, not a bool.** Partial pickup decrements
  `Count` and leaves the remainder in the world. Treating it as a bool is exactly the Unity bug
  the C++ API was reshaped to prevent.
- **No UMG dependency.** The prompt is on-screen debug text (`UInteractorComponent::bShowDebugPrompt`,
  default on). `WBP_RPG_InteractPrompt` belongs to step 6; it subscribes to
  `OnActiveInteractableChanged` and this code does not change. This is a deliberate deviation from
  the plan's "the interactable owns a WidgetComponent" — building that plumbing now would have
  blocked step-3 testing behind hand-built UMG.
- **Names deviate from the plan's Blueprint-era table:** `AWorldItem` not `BP_RPG_WorldItem`,
  `UInteractableComponent`/`UInteractorComponent` not `BPC_*`, no `BPI_RPG_Interactable`.

### Build traps hit (all fixed — read before touching this module)

5. **UHT: a `UFUNCTION` parameter may not shadow a property in scope.** `SpawnWorldItem(… AActor* Owner)`
   → *"cannot be defined … as it is already defined in scope 'AActor'"*. Renamed to `SpawnOwner`.
6. ☠ **For a `BlueprintNativeEvent` on a `UINTERFACE`, UHT emits the `_Implementation` bodies
   INLINE into the generated header.** Defining them in the .cpp is a redefinition (**C2084**).
   This is the opposite of a `BlueprintNativeEvent` on a normal `UCLASS`, where you must supply
   the body. `RPGInteractable.cpp` is intentionally empty and says so.
   The generated defaults value-initialise: priority `None`, empty label, `IsInteractEligible`
   **false**, `Interact` false — a good default, since a half-implemented interactable stays
   silent instead of showing a dead prompt.
7. **C4263/C4264 — do not name a component method `SetActive`.** `UActorComponent::SetActive(bool, bool)`
   already exists; an overload hides the base virtual. Renamed to `SetActiveInteractable`.

### Engine-API corrections (found by READING the engine, not guessing)

- **`USkeletalMeshComponent::SetSkeletalMesh` is the pre-5.1 spelling.** Use
  **`SetSkeletalMeshAsset(USkeletalMesh*)`**. A deprecation is a hard failure under this
  project's warnings-as-errors.
- ⚠ **`UInputMappingContext::Mappings` is `UE_DEPRECATED(5.7)` — the live data is
  `DefaultKeyMappings.Mappings`** (an `FInputMappingContextMappingData` struct).
  **This retires the long-standing belief that "Python reports `IMC_RPG_Default` as 0 mappings
  because the API doesn't round-trip."** It was never a round-trip failure — every read was
  hitting the dead property. Reading `default_key_mappings.mappings` returns all 12 correctly.
- ⚠ **`UInputMappingContext::MapKey` / `UnmapKey` / `UnmapAllKeysFromAction` are
  `UFUNCTION(BlueprintCallable)` — key mapping IS scriptable.** The handoff's old "manual editor
  step" claim applies **only** to the `EnhancedInputAction` NODE in a graph (its InputAction pin
  still cannot be set from script), not to the mapping itself.
- `unreal.EnhancedInputLibrary.request_rebuild_control_mappings_for_context` is **not** exposed to
  Python. A PIE restart covers it.

### Enhanced Input behaviour, confirmed in PIE

**An Input Action with NO triggers fires `Triggered` on every frame the key is held.** One E press
logged ~50 pickups. Fix is `UInputTriggerPressed` **on the action** (not switching the graph to the
`Started` pin) so every future consumer inherits single-fire semantics. This is the same root cause
as the existing `IA_RPG_Attack` note — 0 triggers, opposite symptom (`Completed` on release).

### Python gotcha that cost a debugging round

**`unreal.InventoryComponent` is a Python TYPE, not a UClass.** `SomeClass.get_name()` on it raises
*"unbound method _ObjectBase.get_name() needs an argument"*. Use **`isinstance(obj, unreal.Foo)`**
for checks and **`unreal.Foo.static_class()`** wherever the C++ API wants a `UClass`. This silently
skipped adding both components to the player BP, which presented as "the prompt never appears".

### Scripts in `Content/Python/` (all re-runnable, all write to `Saved/CC_Probe/`)

| Script | Purpose |
|---|---|
| `cc_step2_dump.py` | Read-only probe of the old Blueprint data + CDO + referencer graph |
| `cc_step2_author.py` | Build → verify → delete → move, for the C++ asset regeneration |
| `cc_step3_wire.py` | Components + IA asset + test pickups. **Its component step is broken** (the `get_name()` bug) — superseded by `cc_step3_fix.py` |
| `cc_step3_fix.py` | Adds the two player components correctly; verifies pickups and IMC |
| `cc_step3_probe_input.py` | Compares `IA_RPG_Interact` against known-good actions; checks IMC references via the asset registry |
| `cc_step3_mapkey.py` | Reads `DefaultKeyMappings` correctly; maps E via `MapKey` |
| `cc_step3_trigger.py` | **May not have been run** — adds `UInputTriggerPressed`, unmaps the stray `IA_Interact` |

### Loose ends

- ~~`IA_Interact` (stray asset, `/Game/RPG/Input/`) — created by hand~~ **CORRECTED 2026-08-01
  session 3:** there is no `/Game/RPG/Input/IA_Interact`. The stray E binding pointed at
  **`/Game/Variant_SideScroller/Input/Actions/IA_Interact`** — an engine *template* asset, still
  used by `BP_SideScrollingCharacter` and `IMC_SideScroller`. It has been unmapped from
  `IMC_RPG_Default`; **leave the asset itself alone**, the SideScroller variant content references it.
  (This mattered: `cc_step3_trigger.py` guesses the stray's path and would have missed it — it
  only worked because its fallback resolves the instance the mapping points at.)
- `docs/ItemsLootUI_MechanicsSpec_ForUE5.md` is referenced by the port plan as its companion spec
  but **does not exist in `docs/`**. If it matters, it is probably still in the Unity repo at
  `E:\Unity\Unity_Procedural_Level_Creator\`.

## Where we are (2026-08-01, session 1 — ITEMS/LOOT TRACK STARTED, C++ MIGRATION PENDING FIRST BUILD)

**New track.** The player-character work is complete; this session opened the **Items / Loot / Inventory-UI** port to reach parity with the Unity project. Planning doc: **`docs/ItemsLoot_PortPlan.md`** (7 steps, sized, with per-step integration points and risks). Read it before touching anything.

**Decisions locked by the user (revisit during testing, not before):** instance model front-loaded to step 2 · hero preview = separate always-idle instance · palette = **Candy Warm** · ~9 items authored by hand now · take the arrow-damage-from-item win in step 4 · modular-character direction out of scope · **UE 5.8 upgrade deferred to after step 5, before step 6** (needs MCPUnreal to rebuild against 5.8 — test on a COPY first).

**⚠ THE BIG CHANGE — this project now has C++.** Mid-session we measured the real cost of Blueprint-only and the user chose to migrate the data + logic layer:
- Python **can** author DataAsset classes, typed/object/array/map variables, and DataAsset instances.
- Python **cannot** create or populate User-Defined **Enums** or **Structs** (no API exists — `FEnumEditorUtils` is C++-only), cannot make **enum-typed** or **soft-ref** variables, and cannot author **graph nodes**. That last one is fatal for steps 2/4/5/7, which are mostly logic.
- Toolchain verified present: **Visual Studio Community 2026** + full engine w/ `Build.bat` + UBT at `E:\UE4 Projects\_UE4\UE_5.7`.
- → `Source/Character_Creator/` written (14 files): `RPGItemTypes.h` (6 enums + `FRarityColors`), `ItemData`, `RarityPalette`, `ItemInstance`, `InventoryComponent`, module + target files. `.uproject` declares the module. **BUILDS GREEN as of 2026-08-01.**

**First-build traps (all fixed — read before touching the module or its Build.cs):**
1. **Stale UBT-generated target rules.** A BP-only project has UBT author temporary
   `Character_CreatorTarget` / `Character_Creator` rules classes into `Intermediate/Source/`.
   Once real ones appeared in `Source/`, both were in scope → `CS0101 already contains a
   definition`. UBT self-cleaned them on the next run ("no longer being treated as a
   code-based project"). If it ever recurs, delete `Intermediate/Source/`.
2. **`BuildSettingsVersion.V5` → `V6` in BOTH target files.** V5 leaves
   `UndefinedIdentifierWarningLevel = Off`, but the installed engine's `UnrealEditor`
   binaries were built with it at `Error`. A project target that shares build products with
   UnrealEditor may not change global compile settings → *"modifies the values of
   properties … This is not allowed."* V6 matches the engine, so nothing is modified.
   **Do NOT take the error message's `TargetBuildEnvironment.Unique` suggestion** — that
   forces a full from-source engine rebuild.
3. **UHT runs `-WarningsAsErrors` under `-installed`.** Every UHT warning is fatal.
   `EInteractPriority` started at `Pickup = 10` with no zero entry → fatal. **Every
   `UENUM` in this module must have an entry at 0.**
4. ☠ **`PublicIncludePaths.Add(ModuleDirectory)` in `Character_Creator.Build.cs` is
   load-bearing.** The module has no Public/Private split and its sources live in `Items/`
   and `Inventory/`. UBT does **not** put the module root on the include path by itself
   under V6 (which disables the legacy public/parent include-path behaviour), so every
   `#include "Items/…"` fails with C1083 without that line. Never delete it.
5. **Toolchain warning, benign so far:** VS 2026 ships MSVC 14.51.36248; UE 5.7 prefers
   14.44.35207. If errors ever appear *inside engine headers* rather than our files, that
   is the cause — install the 14.44 toolset or pin `WindowsPlatform.CompilerVersion`.
6. **`Plugin 'MCPUnreal' does not list plugin 'Fab' as a dependency`** — pre-existing gap in
   the plugin's `.uplugin`, unrelated to this module, non-fatal. Ignore.

**Design fix made during the build pass — `Equip` could not reach the off-hand.**
`UInventoryComponent::Equip()` derived its target slot from `Instance->GetSlot()`
(= `Template->Slot`), which is a single value — so a one-hand sword authored as `Melee`
could **never** be equipped into `OffHand`. That makes step 4's stance rows 3 (sword+shield)
and 4 (double sword) unreachable and `AttachRotationOffHand` dead weight. Added
`EquipToSlot(Instance, Slot)` + `CanEquipToSlot()`; `Equip()` is now a wrapper passing the
template default. `CanEquipToSlot` is permissive in exactly one place: OffHand also accepts
`Shield` or `OHS`. `EquipToSlot` also clears the instance out of any *other* slot it held.
Also: `Unequip()` now **refuses** (returns false) when the bag is full instead of destroying
the item, and `UItemData` lost its `Const` UCLASS specifier with all properties moved
`EditDefaultsOnly` → `EditAnywhere` so the DataAssets are actually editable.

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