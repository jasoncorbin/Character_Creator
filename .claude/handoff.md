# Handoff — RPG Character project

When the user says **"let's go" / "time to get started" / "continue"** (or anything like it), read these first, in order, before doing anything:

1. **`docs/RPG_BuildProgress.md`** — the running build log. Start at the **▶ RESUME HERE** block at the top (it states current state + next steps), then skim the latest session entries at the bottom.
2. **`docs/RPG_CharacterArchitecture.md`** — the approved architecture (the plan we build against).
3. **Auto-memory** `MEMORY.md` (loaded automatically) — especially:
   - `rpg-stance-switch.md` — the stance system we just finished
   - `rpg-combat-combo.md`, `target-lock-system.md`, `rpg-dodge-system.md` — SingleSword systems
   - `mcp-blueprint-editing.md` — **MCP tooling limits/traps; read before any Blueprint edits**
   - `inspecting-blueprints.md` — how to inspect live (subagent for big graphs)
4. **`.claude/bugs_to_fix.md`** — deferred issues (currently all CC-track, set to IGNORE until CC work resumes).

## Prereqs to check at start
- **UE editor must be running with the MCPUnreal plugin (port 8090).** Call `mcp__mcp-unreal__status` first to confirm `editor_online` + `plugin_online`. If offline, ask the user to open the editor.
- Project: `BP_RPG_PlayerCharacter`, `ABP_RPG_Player`, assets under `/Game/RPG/`. CC assets (`BP_CC_Character`, etc.) are an untouched archive — don't edit unless explicitly asked.

## Where we are (2026-06-14)
**SingleSword stance complete end-to-end (A–H)** + **Stance-switch foundation (Phase A) complete**, all PIE-confirmed:
- Q cycles all 8 stances `(CurrentStance+1)%8`; weapon mesh swaps live via `ApplyStance` (7/8 stances).
- `CurrentStance` is an **int, not the enum** — by design (UE has no int→UserDefinedEnum convert). Don't "fix" this.
- Weapon data = `StanceRightMeshes` / `StanceLeftMeshes` StaticMesh arrays on the char (populated via Python). Components `Sword` (Weapon_R socket) + `Weapon_L` (Weapon_L socket).

## Next up (pick with user)
- **Phase B — per-stance locomotion:** build `BS_RPG_Loco_<Stance>` blendspaces (7 more) and route through a **`Blend Poses by Int`** node (selector = `CurrentStance`) in the AnimGraph (the old `Blend Poses (E_RPG_Stance)` node was deleted). Suggested start: **NoWeapon** (scout its locomotion clips first). Order: NoWeapon → TwoHandsSword → SwordAndShield → DoubleSword → Spear → MagicWand → BowAndArrow (Bow last).
- **Phase C — per-stance combat/dodge montages**, replicating the SingleSword pattern.
- **Deferred:** BowAndArrow visual (bow is a *skeletal* mesh `/Weapon/Bows/Bow0x_SK` → needs its own SkeletalMeshComponent, visibility-toggled by stance) — do with the bow combat pass.
- **Not committed to git yet** — Phase A work is saved to disk but uncommitted on `main`. Offer to branch + commit when resuming.

## Working pattern that works here
Build automatable parts via MCP/Python (variables, array data, node spawning, pin connects); **guide the user through manual graph wiring** for things MCP can't configure (object-ref pins, EnhancedInputAction nodes, function-call node targets, custom-function calls, AnimGraph node internals); then **verify via `blueprint_query` (use a subagent for big graphs)** before the user PIE-tests. Confirm odd-looking BP choices with the user before assuming they're bugs.
