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

## Where we are (2026-07-16)
**Bow flying-arrow polish COMPLETE, PIE-confirmed working.** Ranged (bow) now fires a real `BP_Arrow` projectile that damages enemies and credits the player.
- `BP_Arrow` (`/Game/RPG/Blueprints/BP_Arrow`): CollisionSphere root + ArrowMesh + ProjectileMovement (speed 6000, gravity 0.4). Hit graph: `OnComponentHit → ApplyDamage(OtherActor) → DestroyActor`. On **BeginPlay** it now calls `CollisionSphere.IgnoreActorWhenMoving(GetInstigator, true)` → **no self-hit**.
- Player BP fires arrows from `MeleeHit` → `Branch(CurrentStance==7)` → `IsValid(ActorToTargetLock)`: valid → SpawnActor aimed at locked target (`296912CA…`), else → SpawnActor down camera-forward (`FD0B40CD…`). Both spawn with **Instigator = Self** (`A863D9B5…`) so kills are credited and the arrow knows who to ignore.
- **Target-lock was accidentally broken and re-fixed this session:** `AC98CB28` (Tick FindLookAt → SetControlRotation) is the LOCK-ON look-at, NOT a bow node. Its `Start` must be the **camera** world location (`8119523F`), `Target` = enemy(−100Z). Do not wire bow muzzle/end into it.
- **Pre-existing bug (flagged, NOT fixed — needs user OK):** `BPC_PlayerStats.Decrease Health` throws `Accessed None … As Dummy` whenever the player takes damage from a non-"Dummy" causer (was spamming because of the old self-hit). Revisit if the user wants.

## Next up — REMOVE COMBOS FROM RANGED WEAPONS (charge/release). Plan APPROVED by user 2026-07-16.
**Goal:** ranged stances fire single, discrete **hold-to-charge → release-to-fire** shots instead of running the melee combo chain.

**Design decisions (user-chosen):**
- **Ranged stances = 6 (Magic Wand, `Wand01_SM`) + 7 (Bow).** (Full stance map: 0 NoWeapon,1 SingleSword,2 TwoHandSword,3 Sword+Shield,4 DoubleSword,5 Spear,6 MagicWand,7 Bow.)
- **Gate via a new `StanceIsRanged` bool[8] array** ([6]=[7]=true; retickable). Scales to future ranged weapons.
- **Fire behavior = hold to charge, release to fire.**
- **Wand reuses `BP_Arrow`** as a placeholder (its muzzle = the bow's `NockArrow` at the left hand for now — refine later).

**Build steps (logic all in `BP_RPG_PlayerCharacter` EventGraph):**
1. Create `StanceIsRanged` bool[8] via Python typed-array method (see [[mcp-blueprint-editing]]); set [6],[7]=true, instance-editable, save.
2. Add `bIsCharging` bool.
3. Rewire `IA_RPG_Attack` (`F6312A9641926A64D95583B102DCDF36`):
   - **Started** → `Branch(StanceIsRanged[CurrentStance])`: ranged → play draw montage + `bIsCharging=true`, **skip the ComboIndex/light-heavy machinery**; melee → existing combo path (`Branch 0C4E4079…`) UNTOUCHED.
   - **Completed** (release) → `Branch(StanceIsRanged AND bIsCharging)` → fire via `BowFire` (`AF52F2724BB2F09B91E7B6AF33D1A612`) → `bIsCharging=false`.
4. Generalize the fire gate: change `MeleeHit`'s `CurrentStance==7` (Equal `711C1B9F…` feeding Branch `767BFFD5…`) to `StanceIsRanged[CurrentStance]` so the wand fires too; route ranged fire through **release**, not the montage notify.

**v1 simplifications (agreed):**
- **Animation:** first pass fires on release WITHOUT a held-draw pose (existing combo anim plays). The pack has **no draw/hold/release clips** — only full shot-cycle combos. Real charge feel needs the user to author each ranged weapon's montage into **Draw(holds, no next-link) + Release(has fire notify)** sections, then wire `Montage_JumpToSection("Release")` on release (montage section editing is manual/user-side).
- **Wand muzzle** = placeholder (NockArrow/left hand); refine later.

**Verify during test:** that `IA_RPG_Attack` actually fires its **Completed** pin on release (depends on the IA trigger config) — if not, add the right input trigger in-editor.

**Key attack-area GUIDs (player EventGraph), for fast pickup:**
- Input `IA_RPG_Attack` = `F6312A9641926A64D95583B102DCDF36` (Started wired; Completed FREE — this is the release hook)
- Bow-draw Play Animation = `2CBA2077411669089513359B7FB05B0F`; combo split Branch(ComboIndex<3) = `0C4E407944CA7027E5F1BB97AD88B54F`; Set ComboIndex(+1%5) = `D4158D41435DB88FF09188A553056271`
- Light PlayMontage = `FC66BA61…`; Heavy = `718691F0…` (montages = `StanceComboLight/Heavy[CurrentStance]`)
- `MeleeHit` event = `A045B4434142618DCA0833A9BC990EB7`; its stance Branch = `767BFFD54B1BE7F590EC02A92745A493`; Equal(==7) = `711C1B9F4EFA7D13E395E7B9C9F5510A`
- `BowFire` event = `AF52F2724BB2F09B91E7B6AF33D1A612` → `IsValid` `5F083F664519D19135419CBD710F6E81` → arrow SpawnActors lock=`296912CA4C545D5A0671E39CA02811D9` / free=`FD0B40CD423334D26AB531892CF2EC16`; Self(instigator)=`A863D9B546055D97865B62BF10CBB00E`

**Not committed to git** — user commits manually (never offer to). BP_Arrow + player BP changes are saved to disk, uncommitted on `main`.

## Working pattern that works here
Build automatable parts via MCP/Python (variables, array data, node spawning, pin connects); **guide the user through manual graph wiring** for things MCP can't configure (object-ref pins, EnhancedInputAction nodes, function-call node targets, custom-function calls, AnimGraph node internals); then **verify via `blueprint_query` (use a subagent for big graphs)** before the user PIE-tests. Confirm odd-looking BP choices with the user before assuming they're bugs.
