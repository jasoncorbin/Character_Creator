# Handoff — RPG Character project

When the user says **"let's go" / "time to get started" / "continue"** (or anything like it), read these first, in order, before doing anything:

1. **`docs/RPG_BuildProgress.md`** — the running build log. Start at the **▶ RESUME HERE** block at the top (it states current state + next steps), then skim the latest session entries at the bottom.
2. **`docs/RPG_CharacterArchitecture.md`** — the approved architecture (the plan we build against).
3. **Auto-memory** `MEMORY.md` (loaded automatically) — especially:
   - `rpg-bow-setup.md` — **the ranged/bow track — most relevant right now** (charge/release, shoulder cam, what's next)
   - `rpg-stance-switch.md` — the 8-stance cycle + ApplyStance (`CurrentStance` is an int, by design)
   - `rpg-combat-combo.md`, `target-lock-system.md`, `rpg-dodge-system.md` — SingleSword systems
   - `mcp-blueprint-editing.md` — **MCP tooling limits/traps; read before any Blueprint edits**
   - `inspecting-blueprints.md` — how to inspect live (subagent for big graphs)
4. **`.claude/bugs_to_fix.md`** — deferred issues (currently all CC-track, set to IGNORE until CC work resumes).

## Prereqs to check at start
- **UE editor must be running with the MCPUnreal plugin (port 8090).** Call `mcp__mcp-unreal__status` first to confirm `editor_online` + `plugin_online`. If offline, ask the user to open the editor.
- Project: `BP_RPG_PlayerCharacter`, `ABP_RPG_Player`, assets under `/Game/RPG/`. CC assets (`BP_CC_Character`, etc.) are an untouched archive — don't edit unless explicitly asked.

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

## Next up — CROSSHAIR / RETICLE for ranged stances (user-requested 2026-07-16, NOT started)
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
