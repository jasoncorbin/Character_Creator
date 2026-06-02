# RPG Player Character — Build Progress

Running log of build sessions for the new `/Game/RPG/` strafe-movement, 8-stance player character.
Reference architecture: `docs/RPG_CharacterArchitecture.md` (approved).

---

## Session 2026-06-02 — Foundations milestone
STATUS: Foundations built. WASD modifier bug RESOLVED (2026-06-02, in-editor). Full strafe feel pending final PIE confirmation — see OPEN.

DONE:
- /Game/RPG/ folder structure
- E_RPG_Stance enum (8 stances)
- BP_RPG_PlayerCharacter (ACharacter) with camera-relative strafe CMC (bOrientRotationToMovement=FALSE, bUseControllerDesiredRotation=TRUE, bUseControllerRotationYaw=FALSE, RotationRate yaw=500), spring arm + follow camera, mesh = OneMeshCharacter01_SK
- Enhanced Input: IA_Move (camera-relative Move function), IA_Look, IA_Jump, IMC_RPG_Default
- ABP_RPG_Player shell (TargetSkeleton OneMeshCharacter01_Skeleton, idle only — Idle_Normal_NoWeapon_Anim → Output Pose, no stance logic); assigned as mesh Anim Class
- BP_RPG_GameMode + BP_RPG_PlayerController, Default Pawn set (GameMode also set as Lvl_RPG_Test World Settings override)
- Lvl_RPG_Test (floor, directional light, sky atmosphere + skylight, player start)

NEXT SESSION: Build SingleSword stance end-to-end — BS_RPG_Loco_SingleSword (2D dir×speed), wire into ABP_RPG_Player locomotion state, jumps, dodge (RM roll), weapon attach (sword mesh to hand socket), combat montages on upper-body slot, target-lock port. Reference: docs/RPG_CharacterArchitecture.md sections 4b–4h.

OPEN / TO VERIFY FIRST:
- **RESOLVED (2026-06-02): WASD all moved right.** Root cause: IA_Move key modifiers (Negate / Swizzle) did not persist when set via Python — UE 5.7 does not round-trip InputMappingContext mappings/modifiers through the Python API. Fixed manually in the IMC editor (W=Swizzle YXZ, S=Swizzle YXZ+Negate, D=none, A=Negate). User confirmed resolved. (Lesson: set IMC modifiers in-editor, never via Python.)
- **Strafe feel: confirm in a quick PIE pass next session before building SingleSword.** Verify: W/S = camera-relative forward/back, A/D = strafe left/right, mouse controls facing, the "D-key test" (side-step right while still facing the mouse, NOT turn-to-move) passes, and Space jumps. (WASD directions now correct per the modifier fix; this is a final feel check, no longer a known bug.)
- **Mesh facing UNVERIFIED.** Mesh component set to standard −90° yaw / Z−88 offset. If the body faces sideways/backward in PIE, adjust the mesh component yaw on BP_RPG_PlayerCharacter.
- **Look pitch invert UNVERIFIED.** If mouse-up looks down, negate IA_Look Y (or add Negate modifier on IA_Look).
- **Empty folder /Game/RPG/Anim/Blendspaces** has no asset yet — UE may not persist a truly-empty folder across restarts; it will be populated next session (BS_RPG_Loco_SingleSword). Not a problem.

TOOLING NOTES (for next session efficiency):
- MCP/Python CANNOT: set EnhancedInputAction node's InputAction ref, set object-ref pins (e.g. Mapping Context), spawn K2/AnimGraph nodes, set IMC modifiers, position graph nodes, or edit UserDefinedEnum values. All of these are editor-dropdown/drag actions → use guided manual steps. MCP/Python CAN: create assets, set CDO/component properties (CMC flags, mesh, camera), assign Anim Class, build folder structure, create levels + actors, and verify graphs via blueprint_query.
- Editor connection dropped once during new_level but recovered; re-saved and verified all assets on disk.
