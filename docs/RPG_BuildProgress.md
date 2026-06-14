# RPG Player Character — Build Progress

Running log of build sessions for the new `/Game/RPG/` strafe-movement, 8-stance player character.
Reference architecture: `docs/RPG_CharacterArchitecture.md` (approved).

> ## ▶ RESUME HERE (next session) — last updated 2026-06-14
> **SingleSword stance COMPLETE end-to-end (A–G), all PIE-confirmed. The validated pattern is ready to replicate to the other 7 stances.**
> Done: locomotion blendspace (A), AnimBP loco (B), jump state machine (C), directional dodge + standing backstep (D, **LeftControl**), weapon attach (E — `Sword`=`OHS03_Sword_SM` on **Mesh → Weapon_R**), **combat combo (F — hybrid: light Combo01–03 on UpperBody slot strafe-swing, heavy Combo04–05 full-body on DefaultSlot, ComboIndex on LMB/`IA_RPG_Attack`)**, **target-lock (G — `IA_RPG_TargetLock`/Tab; sphere-trace from `FollowCamera` fwd×3000 r125, CanDamage gate, FindLookAt-on-Tick; composes w/ strafe CMC)**. All saved to disk.
> **Combat hit-detection (H) DONE 2026-06-14:** `Hit` Montage Notifies on combo sections → PlayMontage `OnNotifyBegin` → `MeleeHit` custom event (SphereOverlapActors in front, r150, [Pawn,PhysicsBody], IgnoreSelf → ForEach → ActorHasTag CanDamage → ApplyDamage 20). Unique-actor overlap = built-in dedupe. **PIE-confirmed: damage applies, Dummy reacts.** Enemy hit-react fixed (see below).
> **NEXT — two tracks (pick with user):**
> 1. **Replicate the stance pattern** to the other 7 stances, recommended order: NoWeapon → TwoHandsSword → SwordAndShield → DoubleSword → Spear → MagicWand → BowAndArrow (Bow last). Per stance = data, not new graph logic: build `BS_RPG_Loco_<Stance>`, add it to the Blend-Poses-by-Enum default/per-stance pin, per-stance jump/dodge/combo montages + weapon-mesh attach + stance-switch wiring. See arch §4b/§4h. **Also need: stance-switch system** (`IA_RPG_SwitchStance` + `DT_RPG_Stances` weapon-attach table, arch §4e).
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
