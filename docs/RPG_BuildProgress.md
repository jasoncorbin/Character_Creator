# RPG Player Character — Build Progress

Running log of build sessions for the new `/Game/RPG/` strafe-movement, 8-stance player character.
Reference architecture: `docs/RPG_CharacterArchitecture.md` (approved).

> ## ▶ RESUME HERE (next session) — last updated 2026-06-23 (Phase C core combat DATA-COMPLETE for all 8 stances; awaiting full PIE sweep)
> **PHASE C (per-stance combat) — CORE COMBAT DATA-COMPLETE FOR ALL 8 STANCES.** Stance-aware montage selection (BUILT + PIE-CONFIRMED mechanism) reads per-stance `AnimMontage` arrays by `CurrentStance` (`StanceComboLight`, `StanceComboHeavy`, `StanceDodgeFWD/BWD/LFT/RGT`, size 8). **ALL 8 indices (0-7) now filled** for both combo arrays AND all 4 dodge arrays — verified clips/sections, BP saved. PIE-confirmed so far: NoWeapon(0), SingleSword(1), TwoHandsSword(2). **(3)-(7) populated but NOT yet PIE-swept.**
> **▶▶ DO FIRST NEXT SESSION — full PIE sweep of stances 3-7:** Q-cycle to each of SwordAndShield(3), DoubleSword(4), Spear(5), MagicWand(6), BowAndArrow(7) and verify: correct weapon mesh + 4-dir dodge + standing backstep + LMB light/heavy combo, no displacement/seep. Watch for: (a) any stance whose light combo looks stiff on UpperBody (spine_01 filter clips wide swings) → move that stance's light combo to DefaultSlot like the heavy; (b) weapon drift → tune `StanceRightRotations[N]`/`StanceLeftRotations[N]`. MagicWand(6)+Bow(7) combos currently play as melee swings (cast/projectile + bow skeletal visual are SEPARATE deferred passes — montages exist so the slots aren't empty).
> Still open: per-stance get-hit/death montages (deferred, batchable via Python — currently all stances share the SingleSword get-hit/death on the enemy); Bow(7) skeletal-mesh visual; MagicWand(6) cast/projectile design; commit Phase B+C work (UNCOMMITTED on `main` — offer branch+commit after the PIE sweep passes). Full detail in the 2026-06-23 session entry at bottom.
> **(Phase B) PER-STANCE LOCOMOTION COMPLETE + PIE-CONFIRMED (2026-06-17):** All **8 `BS_RPG_Loco_<Stance>` blendspaces** built and wired through a **`Blend Poses by Int`** (Active Child Index ← `CurrentStance`) in the AnimGraph Grounded state — Q cycles all 8 stances with correct per-stance locomotion. **Weapon orientation done & data-driven:** components renamed `Weapon_R`/`Weapon_L`; `ApplyStance` sets per-stance mesh AND rotation for both hands from `StanceRight/LeftMeshes` + `StanceRight/LeftRotations` (tuned: Spear[5] right=R10; S&S[3] left=0,0,−180; DualSword[4] left=−90,0,−180; shield=Shield04). **All weapons confirmed correct in every stance.** Bow(7) body-loco works; bow **skeletal mesh visual still deferred** to the bow combat pass. **NEXT: Phase C — per-stance combat/dodge montages.** **Phase B saved but UNCOMMITTED on `main` — offer branch + commit.**
> **(Phase B build reference, 2026-06-15):** the 8 blendspaces and the manual AnimGraph wiring. **How blendspaces were built:** duplicate `BS_RPG_Loco_SingleSword` → remap the 7 samples (by position) to the stance's clips via Python — `sample_data`/`FBlendSample`/`blend_parameters` ARE settable in Python (axis = Direction −180..180 grid4 × Speed 0..600 grid2; samples idle@0,0 / MoveFWD@0,300 / MoveBWD@±180,300 / MoveRGT@90,300 / MoveLFT@−90,300 / SprintFWD@0,600). **Pack clip-naming quirks:** TwoHandsSword suffix=`THS` & its Move clips drop "Battle" (`MoveFWD_InPlace_THS`); Spear Move clips also drop "Battle"; SwordAndShield idle is a pack TYPO `Idle_Battle_SwordAndShiled_Anim`. **AnimGraph wiring is MANUAL** — the blendspace player nodes live inside the Grounded *state sub-graph* which MCP get_graph/add_node CANNOT reach (only top-level FunctionGraphs/UbergraphPages), and the BlendSpace asset ref + AnimGraph pins aren't Python-settable. Inspect inner state graph read-only via `BlueprintEditorLibrary.find_graph(bp,"Grounded").get_graph_nodes_of_class(unreal.AnimGraphNode_Base)`.
> **SingleSword stance COMPLETE end-to-end (A–H) + STANCE-SWITCH FOUNDATION (Phase A) COMPLETE, all PIE-confirmed. Player can now cycle all 8 stances on `Q` and the weapon mesh swaps live (7/8 stances; bow deferred).**
> **Stance-switch (Phase A, 2026-06-14):** `IA_RPG_SwitchStance`/**Q** → `(CurrentStance+1)%8` cycle. **`CurrentStance` is an `int`, NOT the `E_RPG_Stance` enum** — chosen deliberately because UE has no int→UserDefinedEnum conversion (blocks cycling/AnimGraph). The enum asset is kept only as the index legend (0=NoWeapon…7=BowAndArrow). ABP reads it as int (Update Animation) for a future **`Blend Poses by Int`** (NOT by Enum). Weapon swap = `ApplyStance` fn called on BeginPlay + after each switch: `Sword`(Weapon_R socket).SetStaticMesh(`StanceRightMeshes[idx]`) + `Weapon_L`(Weapon_L socket).SetStaticMesh(`StanceLeftMeshes[idx]`). Both arrays are `StaticMesh` arrays on the char, populated via Python. Right: [_,OHS03,THS01,OHS03,OHS03,Spear01,Wand01,_]; Left: [_,_,_,Shield01,OHS03,_,_,_]. Bow (idx7) = skeletal mesh → needs separate SkeletalMeshComponent, deferred to bow combat pass.
> Done: locomotion blendspace (A), AnimBP loco (B), jump state machine (C), directional dodge + standing backstep (D, **LeftControl**), weapon attach (E — `Sword`=`OHS03_Sword_SM` on **Mesh → Weapon_R**), **combat combo (F — hybrid: light Combo01–03 on UpperBody slot strafe-swing, heavy Combo04–05 full-body on DefaultSlot, ComboIndex on LMB/`IA_RPG_Attack`)**, **target-lock (G — `IA_RPG_TargetLock`/Tab; sphere-trace from `FollowCamera` fwd×3000 r125, CanDamage gate, FindLookAt-on-Tick; composes w/ strafe CMC)**. All saved to disk.
> **Combat hit-detection (H) DONE 2026-06-14:** `Hit` Montage Notifies on combo sections → PlayMontage `OnNotifyBegin` → `MeleeHit` custom event (SphereOverlapActors in front, r150, [Pawn,PhysicsBody], IgnoreSelf → ForEach → ActorHasTag CanDamage → ApplyDamage 20). Unique-actor overlap = built-in dedupe. **PIE-confirmed: damage applies, Dummy reacts.** Enemy hit-react fixed (see below).
> **NEXT — two tracks (pick with user):**
> 1. **Phase B — per-stance locomotion:** stance-switch + weapon swap is DONE. Next is making each stance *animate* differently. Build `BS_RPG_Loco_<Stance>` blendspaces (7 more) and feed them through a **`Blend Poses by Int`** node (selector = `CurrentStance` int) in the AnimGraph — replacing the old single `Blend Poses (E_RPG_Stance)` (already deleted). Recommended order: NoWeapon → TwoHandsSword → SwordAndShield → DoubleSword → Spear → MagicWand → BowAndArrow (Bow last). Then **Phase C** per-stance combat/dodge montages replicating SingleSword. Bow stance also still needs its skeletal-mesh visual.
> 2. **RPG enemy — DONE (`/Game/RPG/Enemies/BP_RPG_Enemy`, duplicated from CC `Dummy`):** in-place hit-react + health bar + in-place death, tagged CanDamage. Remaining enemy work: sever CC deps if desired (AnimClass `ABP_NoWeapon`, `BPC_PlayerStats`, CC widgets), fix assassinate-overlap cast (CC→RPG char), add AI. Plus combat polish: tune combo window (~1.0s), damage values, JumpStart/JumpEnd, foot-slide calibration.
> Editor must be running with the MCPUnreal plugin (port 8090) for live inspection. Full detail in the session entries at the bottom of this file.

---

## Session 2026-06-02 — Foundations milestone
STATUS: Foundations built + WASD modifier bug RESOLVED + **PIE-CONFIRMED by user 2026-06-03** (strafe feel, jump, mesh facing, pitch all good). Foundation milestone COMPLETE. Proceeding to SingleSword stance.

DONE:
- /Game/RPG/ folder structure
- E_RPG_Stance enum (8 stances)
- BP_RPG_PlayerCharacter (ACharacter) with camera-relative strafe CMC (bOrientRotationToMovement=FALSE, bUseControllerDesiredRotation=TRUE, bUseControllerRotationYaw=FALSE, RotationRate yaw=500), spring arm + follow camera, mesh = OneMeshCharacter01_SK
- Enhanced Input: IA_Move (camera-relative Move function), IA_Look, IA_Jump, IMC_RPG_Default
- ABP_RPG_Player shell (TargetSkeleton OneMeshCharacter01_Skeleton, idle only — Idle_Normal_NoWeapon_Anim → Output Pose, no stance logic); assigned as mesh Anim Class
- BP_RPG_GameMode + BP_RPG_PlayerController, Default Pawn set (GameMode also set as Lvl_RPG_Test World Settings override)
- Lvl_RPG_Test (floor, directional light, sky atmosphere + skylight, player start)

NEXT SESSION: Build SingleSword stance end-to-end — BS_RPG_Loco_SingleSword (2D dir×speed), wire into ABP_RPG_Player locomotion state, jumps, dodge (RM roll), weapon attach (sword mesh to hand socket), combat montages on upper-body slot, target-lock port. Reference: docs/RPG_CharacterArchitecture.md sections 4b–4h.

---

## Session 2026-06-03 — SingleSword stance (in progress)
STATUS: Locomotion working in PIE. Building remaining SingleSword sub-steps.
DONE:
- Foundation PIE-confirmed by user (strafe/jump/mesh/pitch all good).
- Sub-step A: BS_RPG_Loco_SingleSword 2D blendspace (Direction -180..180 grid4, Speed 0..600 grid2), 7 samples (Idle@0,0; MoveFWD@0,300; MoveBWD@±180,300; MoveRGT@90,300; MoveLFT@-90,300; SprintFWD@0,600). All InPlace clips.
- Sub-step B1: ABP_RPG_Player EventGraph — vars Speed/Direction/CurrentStance(E_RPG_Stance, default SingleSword)/IsFalling?/OwnerChar; Update Animation casts pawn→OwnerChar, sets Speed (velocity length), Direction (CalculateDirection), IsFalling? (CharacterMovement). All targets point at OwnerChar.
- Sub-step B2: AnimGraph — Direction/Speed → BS_RPG_Loco_SingleSword → Default Pose of Blend Poses(E_RPG_Stance) [ActiveEnumValue=CurrentStance] → Output Pose. Blendspace on Default pin = plays for all stances until explicit per-stance pins added (scalable). **PIE-confirmed: SingleSword strafe locomotion plays.**
- Sub-step C: AnimGraph state machine "Locomotion" — Grounded state (the blendspace+enum) + Jump state (JumpAir_InPlace_SingleSword_Anim), transitions Grounded→Jump (IsFalling?) and Jump→Grounded (NOT IsFalling?, blend ~0.2). **PIE-confirmed: jump shows air pose + soft landing.** (2-state version; JumpStart/JumpEnd polish deferred.)
- Sub-step D: Dodge (RM roll). IA_RPG_Dodge (bool) → LeftControl in IMC_RPG_Default. RM roll clips: pack's …/SingleSword/RootMotion/Roll*_RM had enable_root_motion=FALSE (RM baked but flag off). Duplicated the 4 into /Game/RPG/Anim/Edited/ with RM flag ON (pack left pristine — reverted). AM_RPG_Dodge_SingleSword montage created from Edited/RollFWD_Battle_RM (forward-only v1). BP_RPG_PlayerCharacter: IsDodging guard bool; IA_RPG_Dodge(Started)→Branch(NOT IsDodging)→Set IsDodging=true→Play Montage [AM_RPG_Dodge, Mesh]→On Completed/On Blend Out→Set IsDodging=false.
  - **KEY FIX: AnimGraph had NO Slot node** → montages couldn't surface (symptom: split-second pause, no anim). Inserted Slot 'DefaultSlot' between Locomotion state machine and Output Pose. **PIE-confirmed: dodge plays full roll with root-motion movement.** (DefaultSlot now also hosts attack montages for Sub-step F; later add UpperBody slot + Layered Blend per Bone for attack-while-moving per arch §4b.)
- Sub-step D2: Directional dodge (relative-to-facing). Created 3 more montages from Edited RM rolls (renamed FWD for consistency): AM_RPG_Dodge_FWD/BWD/LFT/RGT_SingleSword in /Game/RPG/Montages/SingleSword. BP dodge logic now picks montage by input direction: GetLastMovementInputVector → length>0.1? (standing=BWD backstep) : CalculateDirection(input, actorRot)→/90→Round→AND 3→Select{0FWD,1RGT,2BWD,3LFT}→Play Montage. **PIE-confirmed: directional dodge + standing backstep all working.**
COMPLETED THIS SESSION (2026-06-03→04): Foundation PIE-confirmed; Sub-steps A (blendspace), B1/B2 (AnimBP loco), C (jump state machine), D+D2 (directional dodge). SingleSword locomotion + jump + dodge all working in PIE.

NEXT SESSION: Continue SingleSword stance —
- Sub-step E: Weapon attach (sword mesh OHS03_Sword_SM → right-hand socket on OneMeshCharacter01_Skeleton). Was mid-investigation: need to find the actual hand socket name on the skeleton (or create one). Add sword as a child mesh component on BP_RPG_PlayerCharacter attached to that socket.
- Sub-step F: Combat (attack montages on a slot; consider UpperBody slot + Layered Blend per Bone for attack-while-moving per arch §4b — DefaultSlot already exists from dodge fix).
- Sub-step G: Target-lock port from BP_CC_Character (IA_TargetLock + sphere-trace + FindLookAt-on-Tick).

OPEN / TO VERIFY:
- Confirm all 4 dodge directions play the CORRECT-facing anim (esp. LFT/RGT not swapped) — reorder Select options if any look wrong.
- Root-motion roll distance/feel — tune if rolls travel too far/short.
- Foot-sliding on locomotion — expected until Speed axis / MaxWalkSpeed calibrated (deferred).
- Jump is 2-state only (air loop); JumpStart/JumpEnd polish deferred.
- ABP_RPG_Player AnimGraph now: Locomotion SM → Slot 'DefaultSlot' → Output Pose (slot required for montages).

OPEN / TO VERIFY FIRST:
- **RESOLVED (2026-06-02): WASD all moved right.** Root cause: IA_Move key modifiers (Negate / Swizzle) did not persist when set via Python — UE 5.7 does not round-trip InputMappingContext mappings/modifiers through the Python API. Fixed manually in the IMC editor (W=Swizzle YXZ, S=Swizzle YXZ+Negate, D=none, A=Negate). User confirmed resolved. (Lesson: set IMC modifiers in-editor, never via Python.)
- **Strafe feel: confirm in a quick PIE pass next session before building SingleSword.** Verify: W/S = camera-relative forward/back, A/D = strafe left/right, mouse controls facing, the "D-key test" (side-step right while still facing the mouse, NOT turn-to-move) passes, and Space jumps. (WASD directions now correct per the modifier fix; this is a final feel check, no longer a known bug.)
- **Mesh facing UNVERIFIED.** Mesh component set to standard −90° yaw / Z−88 offset. If the body faces sideways/backward in PIE, adjust the mesh component yaw on BP_RPG_PlayerCharacter.
- **Look pitch invert UNVERIFIED.** If mouse-up looks down, negate IA_Look Y (or add Negate modifier on IA_Look).
- **Empty folder /Game/RPG/Anim/Blendspaces** has no asset yet — UE may not persist a truly-empty folder across restarts; it will be populated next session (BS_RPG_Loco_SingleSword). Not a problem.

TOOLING NOTES (for next session efficiency):
- MCP/Python CANNOT: set EnhancedInputAction node's InputAction ref, set object-ref pins (e.g. Mapping Context), spawn K2/AnimGraph nodes, set IMC modifiers, position graph nodes, or edit UserDefinedEnum values. All of these are editor-dropdown/drag actions → use guided manual steps. MCP/Python CAN: create assets, set CDO/component properties (CMC flags, mesh, camera), assign Anim Class, build folder structure, create levels + actors, and verify graphs via blueprint_query.
- Editor connection dropped once during new_level but recovered; re-saved and verified all assets on disk.

---

## Session 2026-06-09→10 — Dodge regression fix + Sub-step E (weapon attach)
STATUS: Dodge rolling RESTORED + **PIE-CONFIRMED by user**. Sub-step E (SingleSword weapon attach) DONE + **in-hand confirmed by user**. Both persisted to disk.

DODGE REGRESSION (rolling stopped working on reload):
- **Root cause:** the `LeftControl → IA_RPG_Dodge` binding in `IMC_RPG_Default` was live in-editor during the 6-04 PIE session but **never saved to the IMC asset** before the commit. On reload the binding was gone → no key fired the dodge. (Classic "worked in PIE, lost on reload" = unsaved IMC binding. The graph was fine.)
- **Fix:** re-bound `IA_RPG_Dodge → LeftControl` and **SAVED the IMC to disk** this time (`IMC_RPG_Default.uasset` now tracked modified). Verified via input_ops get_bindings (7 bindings).
- **Lesson reinforced:** MCP `input_ops bind_action` only sets the binding in editor memory — `git status` stays clean until you Save in-editor. Always Save the IMC after binding.
- **Self-inflicted detour (reverted):** earlier this session the dodge graph was wrongly edited to "move-only" (deleted the standing BWD-backstep node) based on a serialization artifact — `blueprint_query get_graph` does NOT show object-ref pin defaults, so the assigned BWD montage read as "empty." Reverted `BP_RPG_PlayerCharacter.uasset` to commit 8fd4a95 (known-good). **Standing-still dodge = BWD backstep is BY DESIGN, not a bug.**

SUB-STEP E — WEAPON ATTACH (DONE):
- Attach point: skeleton ships a dedicated **`Weapon_R` socket** (parent bone `weapon_r`, child of `hand_r`; rel loc 0,0,0; rel rot roll **−103.49°** = pack's baked grip alignment). Other sockets: `Weapon_L`, `BackPack`, `Head`.
- Added `Sword` StaticMeshComponent to `BP_RPG_PlayerCharacter`, parented to **Mesh → Weapon_R** socket, Static Mesh = `OHS03_Sword_SM`, zero local offset (socket roll handles grip). Compiled + saved (`BP_RPG_PlayerCharacter.uasset` modified on disk). Verified component + mesh via SubobjectDataSubsystem; user confirmed sword sits correctly in the right hand.
- Manual add was needed: a Static Mesh Component must be **dragged onto Mesh** in the Components tree to become its child before the Parent Socket dropdown lists skeleton sockets.

HOUSEKEEPING:
- `.claude/bugs_to_fix.md`: both tracked bugs (`BPC_AttackSystem` dual-trace, `ABP_NoWeapon` divide-by-zero) tagged **⏸️ IGNORE until CC-version work resumes** — they're CC-track, not RPG.

NEXT SESSION: Continue SingleSword stance —
- **Sub-step F: Combat.** Attack/combo montages on a slot; plan UpperBody slot + Layered Blend per Bone (spine) for attack-while-strafing (arch §4b). `DefaultSlot` already exists from dodge.
- **Sub-step G: Target-lock port** from BP_CC_Character (IA_TargetLock + sphere-trace + FindLookAt-on-Tick).
- Polish backlog (deferred): verify 4 dodge directions face-correct; tune roll distance; foot-sliding (calibrate Speed axis / MaxWalkSpeed); JumpStart/JumpEnd; confirm sword follows hand during RM dodge in PIE.

TOOLING NOTES (this session):
- `blueprint_query get_graph` hides object-ref pin defaults (montage/mesh/asset refs read as empty) — never infer "unset" from it.
- MCP `blueprint_modify` edits DO write to the `.uasset` on disk (not just editor memory) — treat as committed changes, not scratch.
- `execute_script` stdout isn't returned by the tool — read prints via `get_output_log` (category LogPython). Skeleton `sockets` property is protected; enumerate bones/sockets via a transient `SkeletalMeshComponent` (`get_num_bones`/`get_bone_name`/`get_all_socket_names`).

---

## Session 2026-06-14 — Sub-step F (Combat / combo) COMPLETE + PIE-confirmed
STATUS: SingleSword combat combo built end-to-end and **PIE-confirmed by user**. Hybrid layered design working. All assets saved.

DESIGN (decided with user this session):
- **Attack-while-strafing via upper-body layering** for light hits, **full-body for heavy finishers** (hybrid). Combo clips are the pack's `InPlace/Combo01–05_InPlace_SingleSword_Anim` (NOT the top-level `Attack01–04`, which are reserved/special; `Attack04_Start`+`Attack04_Spinning` are a future charged-attack candidate).

ANIMGRAPH (`ABP_RPG_Player`):
- Inserted **`Slot 'UpperBody'`** + **`Layered blend per bone`** (branch filter **`spine_01`**, blend depth 1). New topology: `Locomotion SM → Slot 'DefaultSlot' ─┬→ LayeredBlend.BasePose → Output` and `└→ Slot 'UpperBody'.Source → LayeredBlend.BlendPoses_0`. DefaultSlot = full-body (dodge + heavy combos); UpperBody = layered (light combos). **PIE-confirmed: light swing plays on upper body while legs keep strafing.**

MONTAGES (`/Game/RPG/Montages/SingleSword/`):
- **`AM_RPG_Combo_SingleSword`** (UpperBody slot) — sections `Combo01`–`Combo05` from the InPlace clips, **section links Cleared** (each section plays once & stops; BP drives advance). Used for light combos 1–3.
- **`AM_RPG_ComboHeavy_SingleSword`** (DefaultSlot, full body) — sections `Combo04`,`Combo05`. Used for heavy finishers 4–5 (their footwork/spin needs the lower body, which the spine_01 filter discards → they were "limited" on UpperBody; full-body fixes them).

INPUT: **`IA_RPG_Attack`** (bool) → **LeftMouseButton** in `IMC_RPG_Default` (saved). Wired on the event node's **Started** exec (fires once per click).

BP LOGIC (`BP_RPG_PlayerCharacter`, restart-at-section combo):
- Vars: `ComboIndex` (int). On `IA_RPG_Attack` Started → **Branch (`ComboIndex < 3`)**: true→light Play Montage (UpperBody), false→heavy Play Montage (DefaultSlot). Both share one **Select<Name>** (Index=ComboIndex; options Combo01–05) feeding `StartingSection`, and one `Get Mesh`. `bShouldStopAllMontages=true` (each press hard-cuts to the next swing). Both `Then` → **Set ComboIndex = `(ComboIndex+1) % 5`** → **Retriggerable Delay (~1.0s)** → **Set ComboIndex = 0** (combo window; tune 0.8–1.2). **PIE-confirmed: 1–3 strafe-swings, 4–5 full-body finishers, chains then resets.**

GOTCHAS HIT THIS SESSION (for next time):
- Montage `Default` section (factory artifact) + auto-section-loop caused single swing to loop → fix in montage Sections panel (**Clear** links + delete `Default`).
- Combo capped at 3: combo window (RetriggerableDelay) too short for press cadence → lengthen; AND a `÷` node used where `%` (modulo) belongs → index never advanced past the divide-rounding.
- Heavy combos (4/5) looked "limited" on UpperBody → they're full-body; route to DefaultSlot.

TOOLING NOTES (capabilities CHANGED vs. prior sessions — verified live):
- ✅ **MCP `blueprint_modify` CAN now add AnimGraph nodes** (`AnimGraphNode_Slot`, `AnimGraphNode_LayeredBoneBlend`) and **K2 nodes that are their own class** (`K2Node_PlayMontage`, `K2Node_IfThenElse`, `K2Node_Select`, VariableGet/Set, `K2Node_EnhancedInputAction`), plus `connect_pins`/`disconnect_pins`. The old "can't spawn nodes" limit is GONE.
- ❌ STILL can't via MCP: set **object-ref pins** (`MontageToPlay` — `set_pin_value` stores an invalid string → compile fatal; clear to `None` and set via the node dropdown); set the **`InputAction`** on an EnhancedInputAction node (stays "None"); add **function-call nodes with a target** (`K2Node_CallFunction` adds an *unconfigured* node — can't pick the function, so math/`<`/`%`/array/montage-function/RetriggerableDelay nodes are user-built); set **AnimGraph node internal props** (slot name, branch-filter bone) — manual in Details.
- Montage section next-links and slot name are NOT script-readable/writable (only `get_num_sections`/`get_section_name`/`get_montage_slot_names` exposed) — montage timeline/section/slot editing is manual.
- `AnimMontageFactory` (Python) creates single-clip montages on **DefaultSlot** only.

### Sub-step G — Target-lock port COMPLETE + PIE-confirmed (same day 2026-06-14)
Ported the `BP_CC_Character` lock-on to `BP_RPG_PlayerCharacter` 1:1 (RPG char uses the **same** `FollowCamera` component name, so it dropped in cleanly). Guided manual build (entire graph is function/getter nodes MCP can't configure — see tooling notes above).
- **Input:** `IA_RPG_TargetLock` (bool, `/Game/RPG/Input/`) → **Tab** (Pressed) in `IMC_RPG_Default` (saved).
- **Var:** `ActorToTargetLock` (Actor object ref) — added in-editor (MCP add_variable mistypes object refs to int).
- **Acquire (on Triggered):** `IsValid(ActorToTargetLock)` → Valid: SET empty (toggle off); NotValid: `SphereTraceForObjects` Start=`FollowCamera` GetWorldLocation, End=Start+(GetForwardVector×3000), Radius 125, ObjectTypes=[Pawn,PhysicsBody], **IgnoreSelf=true** → **Branch(ReturnValue) [mandatory hit-gate]** → Break Hit Result → Hit Actor → Branch(`ActorHasTag "CanDamage"`) → SET ActorToTargetLock=Hit Actor.
- **Maintain (Event Tick, wired directly — RPG char had no prior Tick logic, no Sequence splice needed):** `IsValid(ActorToTargetLock)` → Valid → `Set Control Rotation`(target=Get Controller) = `FindLookAtRotation`(self GetActorLocation, target GetActorLocation − (0,0,100)).
- **Tuning:** distance=fwd×3000; aim forgiveness=radius 125; camera-higher=−100 Z.
- **GOTCHA fixed during build:** acquire-path `IsValid` had its `InputObject` pin unconnected (Tick-path one was fine) → toggle-OFF didn't work (always re-acquired); connected it to `Get ActorToTargetLock`. (Reminder: object-typed *links* DO show in `get_graph`; only object *default values* are hidden — so a missing object link there is real.)
- Needs a `CanDamage`-tagged Pawn/PhysicsBody target in the level (reused/placed an enemy). **PIE-confirmed: Tab locks, character faces & strafe-orbits target, Tab again releases.**
- **SingleSword stance is now feature-complete (A–G).** Validated pattern ready to replicate to the other 7 stances.

### Sub-step H — Combat hit-detection + enemy hit-react COMPLETE + PIE-confirmed (same day 2026-06-14)
Made the combo actually deal damage.
- **Notifies:** a **Montage Notify** named `Hit` placed at the impact frame of each combo section — `Combo01–03` on `AM_RPG_Combo_SingleSword`, `Combo04–05` on `AM_RPG_ComboHeavy_SingleSword`. (Must be a **Montage Notify**, not a plain skeleton notify — only Montage Notifies route to the Play Montage node's `OnNotifyBegin`.)
- **`MeleeHit` custom event** (`BP_RPG_PlayerCharacter`): `SphereOverlapActors`(pos = GetActorLocation + GetActorForwardVector×150, radius 150, ObjectTypes [Pawn,PhysicsBody], IgnoreSelf) → ForEach OutActor → Branch `ActorHasTag "CanDamage"` → `ApplyDamage` 20 (instigator = Get Controller, causer = self). `SphereOverlapActors` returns **unique** actors → built-in per-swing dedupe (no Hit-Actors array needed; avoids the CC `BPC_AttackSystem` double-hit weakness).
- **Wiring:** both light & heavy Play Montage `OnNotifyBegin` → `MeleeHit`.
- **PIE-confirmed: damage applies to the CanDamage target.**

ENEMY HIT-REACT — "Dummy flies into the distance" FIXED:
- Root cause: test target is the CC archive `Dummy` (`/Game/CharacterCreator/enemies/Dummy`, Character, mesh on `OneMeshCharacter01_SK`, AnimClass `ABP_NoWeapon`). Its `Event AnyDamage` plays a random `hitAnim` montage (`CC_GetHit01/02_SingleSword_Anim_Montage`) + decrements `BPC_PlayerStats` health + death/destroy. `ABP_NoWeapon` is **`Root Motion From Montages Only`** and those CC hit montages carry root motion → each hit slides the capsule; a 5-hit combo launches it.
- Fix WITHOUT touching CC archive: created in-place hit-react montages **`AM_RPG_GetHit01/02_SingleSword`** (from the pack's top-level `GetHit0x_SingleSword_Anim`, `enable_root_motion=False`) and **overrode the Dummy *instance's* `hitAnim` array** in `Lvl_RPG_Test` to use them (instance-editable, so CC BP + shared `ABP_NoWeapon` stay untouched). **PIE-confirmed: flinches in place, no displacement.**
- **TODO (proper):** replace the CC Dummy with a dedicated `BP_RPG_Enemy` (RPG-track, on OneMeshCharacter01, in-place hit-react + own health). The CC Dummy is a placeholder. → **DONE (see below).**

### BP_RPG_Enemy — dedicated RPG enemy COMPLETE + PIE-confirmed (2026-06-14)
`/Game/RPG/Enemies/BP_RPG_Enemy` — **duplicated from the CC `Dummy`** (user's call: reuse its working health/death/hit systems rather than rebuild). Parent Character, mesh on OneMeshCharacter01, tagged **CanDamage** (so lock-on + MeleeHit detect it). Updates made:
- **hitAnim** default → in-place `AM_RPG_GetHit01/02_SingleSword` (baked on the BP, no per-instance override) → flinch in place, no flying.
- **Death montage** → new in-place **`AM_RPG_Die01Stay_SingleSword`** (created from pack `Die01Stay_SingleSword_Anim`, enable_root_motion=False; also made `AM_RPG_Die01/Die02`). Death flow = montage → Delay 2s → Destroy.
- **Health-bar fix:** the shared CC `BPC_PlayerStats.Decrease Health` updates the bar via its `As Dummy` ref, which is **null** on BP_RPG_Enemy (its BeginPlay casts owner→`Dummy` class, which BP_RPG_Enemy isn't) → bar didn't move (CurrentHealth/death still worked). Fixed by **pushing the bar % from BP_RPG_Enemy's own AnyDamage**: after `Decrease Health`, `Set Percent` on `Get Health Bar UI → HealthBar` (= CurrentHealth/MaxHealth from its `BPC_PlayerStats`). Keeps the CC component untouched.
- **Still CC-dependent (acceptable, future cleanup):** AnimClass `ABP_NoWeapon`, `BPC_PlayerStats` component, CC widgets (`WB_Dummy_Health`, `WB_Assassinate_Prompt`); assassinate-radius overlap still casts to `BP_CC_Character` (won't fire for the RPG player — cosmetic).
- New RPG montages this round: `AM_RPG_GetHit01/02_SingleSword`, `AM_RPG_Die01Stay/Die01/Die02_SingleSword` (all in-place, DefaultSlot, in `/Game/RPG/Montages/SingleSword/`).

---

## Session 2026-06-14 (cont.) — Stance-switch system (Phase A) COMPLETE + PIE-confirmed

Built the foundation that lets the player cycle all 8 weapon stances and swaps the held weapon mesh live. **All PIE-confirmed by user.** This is the spine the per-stance locomotion (Phase B) and combat (Phase C) will hang off.

### The int-vs-enum decision (important)
`CurrentStance` is an **`int`**, not the `E_RPG_Stance` enum, on both `BP_RPG_PlayerCharacter` and `ABP_RPG_Player`. Reason: UE has **no int→UserDefinedEnum conversion node** ("No entries" when trying to connect), which blocks both the `(x+1)%8` cycle write-back and any int-driven AnimGraph selector. So we standardized on int everywhere; the `E_RPG_Stance` asset is kept **only as the index legend** (0=NoWeapon, 1=SingleSword, 2=TwoHandsSword, 3=SwordAndShield, 4=DoubleSword, 5=Spear, 6=MagicWand, 7=BowAndArrow).
- GOTCHA hit & fixed: MCP `add_variable` (and `blueprint_query inspect`) **lie about UserDefinedEnum types** — `add_variable("CurrentStance","E_RPG_Stance")` silently created an **int**, and `inspect` reported it as `E_RPG_Stance` (reads the stored descriptor sub-type) while the compiled property was `int`. Detect via `execute_script` get_editor_property pytype. This is why the char var ended up int (kept it, by the decision above).

### A1–A4: switch logic
- Input `IA_RPG_SwitchStance` (bool) bound to **`Q`** in `IMC_RPG_Default` (via `input_ops bind_action`).
- `IA_RPG_SwitchStance (Started)` → `Set CurrentStance = (Get CurrentStance + 1) % 8` → `ApplyStance` → `Print String` (validation). True modulo, wraps 7→0. **PIE-confirmed cycling + wrap.**
- `ABP_RPG_Player` Update Animation reads it: off the existing `Cast To BP_RPG_PlayerCharacter`, `Get CurrentStance (int)` → `Set CurrentStance (ABP int)`, spliced before Set OwnerChar. (Earlier the old `Blend Poses (E_RPG_Stance)` node + a stale enum getter caused int↔enum compile errors → resolved by going int and **deleting that Blend-by-Enum node**; Phase B will use `Blend Poses by Int`.)

### A5: weapon-mesh swap (data = populated arrays, not a DataTable)
Chose populated `StaticMesh` arrays over a `DT_RPG_Stances` struct/table — same data-driven result, fully fillable via Python (object refs can't be set on BP pins, but CAN be set on CDO array props). A real DataTable can come later if Phase B/C want per-stance montages/blendspaces in one home.
- Vars on char: **`StanceRightMeshes`** and **`StanceLeftMeshes`** (Array of StaticMesh, created in-editor, populated via Python on the CDO + saved).
  - Right: `[None, OHS03_Sword_SM, THS01_Sword_SM, OHS03_Sword_SM, OHS03_Sword_SM, Spear01_SM, Wand01_SM, None]`
  - Left:  `[None, None, None, Shield01_SM, OHS03_Sword_SM, None, None, None]`
- Components: existing **`Sword`** on socket **`Weapon_R`** (right hand) + new **`Weapon_L`** (cloned from Sword, socket **`Weapon_L`**, default mesh cleared) for shield/off-hand. Sockets `Weapon_R`/`Weapon_L` already exist on `OneMeshCharacter01_Skeleton`, so swapping = just `Set Static Mesh` on already-attached components (no re-attach, no socket data needed).
- **`ApplyStance`** function (custom fn, called on **BeginPlay** + after each switch): `Sword.SetStaticMesh(StanceRightMeshes[CurrentStance])` then `Weapon_L.SetStaticMesh(StanceLeftMeshes[CurrentStance])`. Index 0/7 = None → component hides. **PIE-confirmed: sword/2H-sword/spear/wand swap on right; shield (idx3) + dual-sword (idx4) on left; NoWeapon empty.**
  - GOTCHA: MCP `add_function` creates a function **without the BlueprintCallable flag** (graph compiles but the palette won't offer a call node — "not BP callable"). Fix = recreate the function in-editor (the **+ Add Function** button). MCP `connect_pins`/`add_node` are fine inside it afterward.

### Deferred
- **BowAndArrow visual (idx 7):** bows are **skeletal** meshes (`/Weapon/Bows/Bow0x_SK`), so a static `Weapon_L` can't show them. Needs a dedicated `SkeletalMeshComponent` (visibility-toggled by stance) — folded into the bow combat pass ("bow last").
- Cosmetic: swapped meshes inherit the component transform tuned for OHS03; spear/2H may want per-stance offsets later.

### Next
**Phase B — per-stance locomotion:** build `BS_RPG_Loco_<Stance>` blendspaces (7 more) and route them through a **`Blend Poses by Int`** (selector = `CurrentStance`) in the AnimGraph. Then **Phase C** per-stance combat/dodge montages replicating SingleSword.

---

## Session 2026-06-15 — Phase B per-stance locomotion (in progress)
STATUS: All 8 locomotion blendspaces built. NoWeapon wired into AnimGraph + PIE-confirmed. Other 6 built, awaiting manual wiring into Blend Poses by Int.

DONE:
- **All 8 `BS_RPG_Loco_<Stance>` blendspaces exist** in `/Game/RPG/Anim/Blendspaces/`. Indices: 0 NoWeapon, 1 SingleSword (pre-existing), 2 TwoHandsSword, 3 SwordAndShield, 4 DoubleSword, 5 Spear, 6 MagicWand, 7 BowAndArrow.
- **Build method (Python, repeatable):** duplicate `BS_RPG_Loco_SingleSword` → for each of the 7 samples, remap `animation` by sample POSITION to the stance's clip, then `save_asset`. Confirmed Python CAN read/write `sample_data` (array of `unreal.BlendSample`: `animation`, `sample_value` Vector, `rate_scale`) and `blend_parameters` (array of `BlendParameter`: `display_name`,`min`,`max`,`grid_num`). Axis config copied from SingleSword: Direction (−180..180, grid 4) × Speed (0..600, grid 2). Sample layout (7): Idle@(0,0), MoveFWD@(0,300), MoveBWD@(−180,300) AND @(+180,300) [wrap], MoveRGT@(90,300), MoveLFT@(−90,300), SprintFWD@(0,600). All clips are `_InPlace_` (CMC drives capsule).
- **Pack clip-naming quirks discovered** (per stance, all suffix = folder name except THS):
  - TwoHandsSword: suffix `THS`; Move clips are `MoveDIR_InPlace_THS_Anim` (NO "Battle"); Sprint keeps "Battle" (`SprintFWD_Battle_InPlace_THS_Anim`); idle `Idle_Battle_THS_Anim`.
  - Spear: Move clips `MoveDIR_InPlace_Spear_Anim` (NO "Battle"); idle/sprint normal.
  - SwordAndShield: idle is a PACK TYPO → `Idle_Battle_SwordAndShiled_Anim` (moves/sprint spell "SwordAndShield" correctly).
  - DoubleSword / MagicWand / BowAndArrow: fully standard `..._Battle_InPlace_<Stance>_Anim`.
- **NoWeapon (idx 0) wired + PIE-confirmed.** AnimGraph Grounded state now: BlendSpacePlayer(NoWeapon)→BlendPose0, BlendSpacePlayer(SingleSword)→BlendPose1 of a `Blend Poses by Int` (Active Child Index ← `CurrentStance`) → StateResult. User verified Q-cycle NoWeapon↔SingleSword changes locomotion.

KEY TOOLING NOTES (important for replication):
- AnimGraph blendspace-player nodes live inside the **Grounded state sub-graph** (`AnimationStateGraph`). MCP `blueprint_query get_graph` / `add_node` / `connect_pins` only search top-level `FunctionGraphs`+`UbergraphPages` → CANNOT reach state sub-graphs. `anim_blueprint_modify add_anim_node` is a STUB (no-op) in this plugin build.
- A node's BlendSpace asset ref (FAnimNode_BlendSpacePlayer.BlendSpace) is NOT Python-exposed (only `blend_weight`/`internal_time_accumulator`); AnimGraph pin link APIs are NOT Python-exposed. → **AnimGraph wiring inside states is MANUAL in-editor.**
- READ-ONLY inspection of a state's inner graph: `g = unreal.BlueprintEditorLibrary.find_graph(bp, "Grounded"); g.get_graph_nodes_of_class(unreal.AnimGraphNode_Base)` (the `Nodes` property is protected; `export_text` unavailable). Use this to verify node count/type after manual wiring.
- `search_assets` with short `class_filter` ("AnimSequence") triggers a non-fatal editor ensure; use fully-qualified `/Script/Engine.AnimSequence`, or prefer Python `EditorAssetLibrary.list_assets` / `does_asset_exist`.

PENDING (next):
- User to add Blend Pose pins 2–7 to the `Blend Poses by Int` node and connect each `BS_RPG_Loco_<Stance>` (+ Direction/Speed Get nodes) per the index table; compile.
- Verify Grounded graph = 8 BlendSpacePlayer + 1 BlendListByInt; PIE-test all 8 stances cycle correctly.
- BowAndArrow (idx 7): body locomotion blendspace built, but the bow **skeletal-mesh visual** is still deferred to the bow combat pass.
- Then Phase C — per-stance combat/dodge montages.
- Phase B work saved to disk, still UNCOMMITTED on `main` (offer branch+commit once all stances PIE-confirmed).

### 2026-06-17 — Weapon orientation pass (per-stance rotation, data-driven both hands)
During Phase B PIE testing, two-handed weapons (spear, THS) appeared misaligned ("seep off"). Root cause: weapon meshes attach to ONE shared component per hand (mesh-swapped per stance), but different weapon types need different rotations — a single component default can't satisfy all. (Pack reference: `ModularCharacter_BP`/`Body05_SK` uses dedicated pre-rotated components per type — Weapon_R 0,0,−180; Weapon_L −90,0,−180; Shield 0,0,−90; Bow 90,90,0; see memory `weapon-socket-convention`.)

**Solution (user-built, by-eye tuned):** `ApplyStance` now sets per-stance rotation on BOTH hands from new Rotator arrays, fully data-driven:
- `ApplyStance` flow: `Weapon_R.SetStaticMesh(StanceRightMeshes[idx])` → `Weapon_R.SetRelativeRotation(StanceRightRotations[idx])` → `Weapon_L.SetStaticMesh(StanceLeftMeshes[idx])` → `Weapon_L.SetRelativeRotation(StanceLeftRotations[idx])`.
- New vars: `StanceRightRotations`, `StanceLeftRotations` (Rotator arrays, 8). Populated via Python on the CDO (object-ref/struct arrays settable on CDO; same path as the mesh arrays).
- `StanceRightRotations` = all identity **except [5] Spear = (R10,P0,Y0)** (user-tuned).
- `StanceLeftRotations` = **[3] SwordAndShield = (0,0,−180)**, **[4] DoubleSword = (−90,0,−180)**, rest identity.
- `StanceLeftMeshes[3]` shield changed `Shield01_SM` → `Shield04_SM` (Shield01 looked like a log; Shield04 is the pack's demo shield).
- **Components renamed** `Sword`→**`Weapon_R`**, `Sword1`→**`Weapon_L`** (match socket names + pack convention; renamed in-editor so all Get-node refs auto-updated).

**Behavior note:** `SetRelativeRotation` is absolute, so it OVERRIDES the components' editor default rotation (Weapon_R/Weapon_L defaults are 0,0,−180 in the SCS but moot at runtime — the arrays govern from BeginPlay onward). So tune rotations in the arrays, not the component defaults. Minor editor-vs-PIE discrepancy (viewport shows the default until BeginPlay) is harmless.

**Confirmed by user:** off-hand sword (DoubleSword) correct once rotation moved to Weapon_L; shield (Shield04) correct; spear improved with R10. TO RE-VERIFY: TwoHandsSword (idx2) at identity (was an original "seep off" case) — set `StanceRightRotations[2]` if it drifts.

**Future:** when moving to modular characters + armor, adopt the `ModularCharacter_BP`/`Body05_SK` dedicated-pre-rotated-component model (also obviates per-stance rotation arrays). User flagged this as the chosen direction.

---

## Session 2026-06-18 — Phase C combat: stance-aware mechanism + NoWeapon (combo + dodge)
STATUS: Stance-aware combat montage selection BUILT + PIE-CONFIRMED. NoWeapon combat core (combo + dodge) done for both NoWeapon(0) and SingleSword(1). Mechanism proven; remaining stances are now data-only (no graph rewiring).

DONE:
- **Stance-aware montage selection (data-driven, mirrors the mesh/rotation arrays):** added 6 `AnimMontage` ARRAY vars (size 8, user-created): `StanceComboLight`, `StanceComboHeavy`, `StanceDodgeFWD/BWD/LFT/RGT`. Populated idx0=NoWeapon, idx1=SingleSword via Python on the CDO (object-ref arrays settable on CDO).
- **EventGraph rewiring (one-time, done):** the attack + dodge `Play Montage` `MontageToPlay` pins now read `Array[CurrentStance]` instead of hardcoded SingleSword literals. Specific nodes: dodge `Select`(4-opt, id 5E1B9971) Option0=FWD/1=RGT/2=BWD/3=LFT ← `StanceDodge{DIR}[CurrentStance]` → dodge PlayMontage 7911AD22. Attack light PlayMontage FC66BA61 (IfThenElse 0C4E40 *then*, ComboIndex<3) ← `StanceComboLight[CurrentStance]`; attack heavy PlayMontage 718691F0 (*else*) ← `StanceComboHeavy[CurrentStance]`. StartingSection still from the 5-opt combo Select (89BB3C, unchanged). **→ ADDING A STANCE = fill array index N; NO graph edits.**
- **NoWeapon dodge (Python-automated):** pack RM rolls `/NoWeapon/RootMotion/Roll{DIR}_Battle_RM_NoWeapon` had `enable_root_motion=False` → duplicated to `/Game/RPG/Anim/Edited/Roll{DIR}_Battle_RM_NoWeapon_Anim` with RM flag ON; created 4 single-clip DefaultSlot montages `AM_RPG_Dodge_{DIR}_NoWeapon` via `unreal.AnimMontageFactory` (set `source_animation` + `target_skeleton` → `AssetTools.create_asset`).
- **NoWeapon combos (manual, duplicate-and-swap):** `AM_RPG_Combo_NoWeapon` (UpperBody, sections Combo01/02/03 ← Combo01/02/03_InPlace_NoWeapon) + `AM_RPG_ComboHeavy_NoWeapon` (DefaultSlot, Combo04/05 ← Combo04/05_InPlace_NoWeapon). Duplicated the SingleSword combo montages and swapped the segment clips (kept slot + sections + Hit notifies).
- **PIE-confirmed:** dodge (4 dir + backstep) and light/heavy combo work in BOTH NoWeapon and SingleSword.

KEY TOOLING NOTE (Phase C):
- Montage **single-clip DefaultSlot** montages ARE Python-creatable (`AnimMontageFactory.source_animation`+`target_skeleton`). Multi-section/slot/notify montages are NOT script-authorable — montage slot/segment tracks are NOT Python-exposed (no `slot_anim_tracks`, no `SlotAnimationTrack` struct, can't remap clips like blendspaces). So combos are manual (duplicate-and-swap from SingleSword preserves sections/slot/notifies).
- Montage's referenced clip is readable via AssetRegistry `get_dependencies` (montage object-ref pins are hidden in get_graph, but deps reveal the anim). `get_num_sections`/`get_section_name` readable.

REMAINING (replicate per stance — data-only now):
- Per stance idx N: (me) dup RM rolls + 4 dodge montages + populate StanceDodge arrays[N]; (user) author 2 combo montages + (me) populate StanceCombo arrays[N]. NO graph edits.
- Order: TwoHandsSword → SwordAndShield → DoubleSword → Spear → MagicWand → BowAndArrow.
- Bow(7): also needs skeletal-mesh visual + cast/projectile design for MagicWand still open.
- Get-hit + death montages per stance (deferred — "core first"); can batch-create via Python (single-clip).
- Phase C combat work UNCOMMITTED on `main`.

---

## Session 2026-06-23 — Phase C combat: TwoHandsSword(2) DONE; SwordAndShield(3) dodge done
STATUS: TwoHandsSword combat COMPLETE + **PIE-CONFIRMED by user** (Q→THS: 4-dir dodge + standing backstep + light/heavy combo all working). SwordAndShield dodge automated + arrays populated; S&S combos pending user authoring.

DONE — TwoHandsSword(2):
- USER authored the 2 combo montages (duplicate-and-swap from SingleSword): `AM_RPG_Combo_TwoHandsSword` (UpperBody, sections Combo01/02/03 ← `Combo01/02/03_InPlace_THS_Anim`) + `AM_RPG_ComboHeavy_TwoHandsSword` (DefaultSlot, Combo04/05 ← `Combo04/05_InPlace_THS_Anim`), in `/Game/RPG/Montages/TwoHandsSword/`. Verified clip refs via AssetRegistry `get_dependencies` + sections via `get_num_sections`.
- (me) populated `StanceComboLight[2]`/`StanceComboHeavy[2]` on the char CDO; saved BP. THS dodge `StanceDodge*[2]` + mesh (`THS01_Sword_SM`) confirmed already in place. **PIE-confirmed by user — no graph edits needed (data-driven mechanism held).**

DONE — SwordAndShield(3) dodge (Python-automated, same pattern as NoWeapon/THS):
- Source RM rolls `/SwordAndShield/RootMotion/Roll{DIR}_Battle_RM_SwordAndShield_Anim` → duplicated to `/Game/RPG/Anim/Edited/` with `enable_root_motion=True` → 4 single-clip DefaultSlot montages `AM_RPG_Dodge_{DIR}_SwordAndShield` via `AnimMontageFactory`. Populated `StanceDodgeFWD/BWD/LFT/RGT[3]`, saved BP.
- S&S clip names are STANDARD (combo InPlace = `Combo0x_InPlace_SwordAndShield_Anim`, rolls `Roll{DIR}_Battle_RM_SwordAndShield_Anim`); the `SwordAndShiled` pack typo is ONLY on the idle loco clip, not relevant to combat.

DONE — SwordAndShield(3) combos: USER authored `AM_RPG_Combo_SwordAndShield` (UpperBody, Combo01/02/03_InPlace_SwordAndShield) + `AM_RPG_ComboHeavy_SwordAndShield` (DefaultSlot, Combo04/05). (me) populated `StanceComboLight[3]`/`StanceComboHeavy[3]`.

### 2026-06-23 (cont.) — Dodge automation 4-7 + all remaining combos populated → core combat data-complete
- **Dodge montages for DoubleSword(4), Spear(5), MagicWand(6), BowAndArrow(7)** batch-created via the documented Python pattern (dup RM rolls → `enable_root_motion=True` → single-clip DefaultSlot `AnimMontageFactory` montages → populate `StanceDodge*[N]`). **Pack quirk:** Spear RM rolls drop "Battle" (`Roll{DIR}_RM_Spear_Anim`); the other three use `Roll{DIR}_Battle_RM_<Stance>_Anim`. Edited clips named `Roll{DIR}_RM_<Stance>_Edited_Anim` in `/Game/RPG/Anim/Edited/`.
  - **TOOLING NOTE:** the all-stances-in-one-call batch script (16 dups + 16 montages + saves) **timed out the HTTP response AND dropped the editor**, but the work had already completed on the editor side — all 16 montages on disk + all 4 `StanceDodge` arrays fully populated (0-7) + BP saved, confirmed after restart. LESSON: heavy asset-creation batches should be split per-stance (≤4 montages/call) to stay under the request timeout and avoid crashing the editor.
- **USER authored all remaining combo montages** (3-7) by duplicate-and-swap from SingleSword; (me) verified each (correct InPlace clips + sections Combo01/02/03 light, Combo04/05 heavy via AssetRegistry deps) and populated `StanceComboLight[3..7]`/`StanceComboHeavy[3..7]`, BP saved.
- **RESULT: all 8 stances have complete combo + dodge data.** PIE-confirmed: 0,1,2. PENDING: full PIE sweep of 3-7.

REMAINING: PIE-sweep stances 3-7; per-stance get-hit/death montages (deferred); Bow(7) skeletal visual; MagicWand(6) cast/projectile design. Phase B+C work UNCOMMITTED on `main` — offer branch+commit after sweep passes.
