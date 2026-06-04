# RPG Player Character — Build Progress

Running log of build sessions for the new `/Game/RPG/` strafe-movement, 8-stance player character.
Reference architecture: `docs/RPG_CharacterArchitecture.md` (approved).

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
