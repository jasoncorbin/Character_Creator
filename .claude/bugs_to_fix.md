# Bugs / Issues To Fix

Tracked issues for future resolution. Status as of 2026-08-02.

> **⏸️ DEFERRED — CC version only (2026-06-09).** The items in the *CC track* section below pertain to
> the original **`BP_CC_Character`** track (`BPC_AttackSystem`, `ABP_NoWeapon`), **not** the active RPG
> character (`BP_RPG_PlayerCharacter`). All are **set to IGNORE until we return to CC-version work.**
>
> ⚠ This no longer covers the whole file — see the RPG-track section immediately below.

---

# RPG track

## Bug 4 — Enemy death animation doesn't hold; the corpse stands back up

**Status:** 🕗 **DEFERRED BY USER (2026-08-02) — schedule AFTER the UE 5.8 conversion.**
Not blocking; do not action during steps 5–6.

**Reported:** user, 2026-08-02, during PIE verification of the `BPC_PlayerStats` fix.
**Regression window:** *"this was working before we started the conversion and the Unity port."*
So it broke somewhere between the pre-port state and now — git history across the port
(`b772959 Port from Unity` → `a7f4b05` → `db565d7` → `aaf4f38` → `d5d4323 C++ rewiring`)
is the place to bisect.

**Symptom:** on death the enemy plays its death montage, then **returns to standing/idle** instead of
staying down. It still destroys correctly ~2 s later, so the exec chain itself is intact.

**Not investigated yet.** Recorded leads only — verify before acting on any of them:

1. **The montage blends out and the AnimBP falls back to locomotion.** The usual cause. Fixes are
   either a Death state in the AnimBP with no exit transition, holding the final pose
   (blend-out time 0 / `Montage_Pause` on completion), or enabling ragdoll.
2. **There is an unreferenced `Ragdoll` custom event in `BP_RPG_Enemy`** —
   node `EDE9FBE54101529C1BCF928DD01712C7`, which runs `Delay 1.2 → Play Montage → Play Sound at
   Location → Delay 2.0 → Destroy Actor`. **Nothing in `BP_RPG_Enemy`'s own EventGraph calls it**, and
   its `OutputDelegate` pin is unlinked. If a ragdoll path was the original death behaviour and its
   caller (an anim notify, or a call from the CC-era graph) was lost in the port, that would produce
   exactly this symptom. **Check this first — it's the cheapest lead and it fits the regression window.**
3. Note the death chain that *does* run is the separate one off `Event AnyDamage`:
   `Branch (IsPlayerDead?) → Play Montage (9EEA6D2D…) → Play Sound → Delay 2.0 → Destroy Actor`.
   Two parallel death paths existing at once is itself suspicious.

**Do not confuse with:** the `BPC_PlayerStats` "Accessed None / As Dummy" spam — that was fixed and
PIE-verified 2026-08-02 and is unrelated (it was dead widget code, no gameplay effect).

---

# CC track

---

## Bug 3 — Two parallel hit-detection systems (no per-actor dedupe)

**Status:** ⏸️ IGNORE until CC-version work resumes (CC track). Latent / low priority. **Not currently double-hitting in playtest**, but the structure allows it. Revisit before shipping or before adding new attack types.

**Location:** `BPC_AttackSystem` EventGraph (`/Game/CharacterCreator/BluePrints/BPC_AttackSystem`).

`BPC_AttackSystem` has **two independent, live damage paths**, each with its own trace + `ApplyDamage`, sharing no guard and with **no "already-hit actors" tracking** anywhere in the graph:

| | Stick Trace (blade sweep) | Sphere Trace |
|---|---|---|
| Entry event | `Start Stick Trace` → Set Timer (0.05s) → `Stick Trace Loop` | `Sphere Trace` event |
| Trace node | `SphereTraceByChannel` R=**18**, swept Bottom→Top of Stick (real blade segment) | `SphereTraceByChannel` R=**25**, **Start == End (zero-length, single point)** |
| Tag gate | `ActorHasTag("CanDamage")` | `ActorHasTag("CanDamage")` |
| Guard | `DoOnce` (node `7058F2F4`, resettable) — global once-gate, NOT per-actor | **none** |
| Damage | `ApplyDamage` 10 (node `4CA78AE3`) | `ApplyDamage` 10 (node `F7E966E0`) |

Triggered by anim notifies on the combo montages: `BP_Notify_StickTrace` (notify-state, drives stick) and `BP_Notify_SphereTrace` (drives sphere). Stick Trace is the "real"/better system (sweeps along the Stick Top/Bottom Point arrows). Sphere Trace looks like the older/legacy version (zero-length, no DoOnce).

**Verbatim node refs:**
- ApplyDamage (stick): `4CA78AE345430D9D506CF690C9B88F12`, BaseDamage `10.000000`
- ApplyDamage (sphere): `F7E966E0459129A5F736BDAF2A125EB3`, BaseDamage `10.000000`
- SphereTraceByChannel R18: `6FF442FC488DE276AE5CEC8059156572` (Start≠End)
- SphereTraceByChannel R25: `0475395D41EC9C9F10722E901FEA324E` (**Start==End, zero-length**)
- Stick DoOnce: `7058F2F4…` (resettable); Sphere path DoOnce: none
- Timer: `D82F6530…`, Time `0.050000`, bound to `Stick Trace Loop` (`5CBC5B9E`), handle var `Stick Trae Loop ` (note trailing-space/typo), cleared by `30DA2BB9` on Stop Stick Trace
- Tag guard (both): `ActorHasTag`, Tag `"CanDamage"` — stick `4067B709`, sphere `A850FF7F`, target = `BreakHitResult.HitActor`
- Sphere Trace custom event entry: `B29D7F7B`

**Risk scenario:** if a swing fires both notifies, an enemy tagged `CanDamage` overlapping both volumes takes 10 (stick) + 10 (sphere) = 20. Also the sphere path has no DoOnce, so repeated `Sphere Trace` calls in one swing each apply damage. Guards are call-gated / tag-gated, never per-actor.

### Resolution options (pick one when revisiting)

**A. Remove the Sphere system (recommended — keep the blade sweep).**
- Cleanest at the montage level: open each attack montage (`CC_Combo01–05`), Notifies track, delete the `BP_Notify_SphereTrace` notifies (leave `BP_Notify_StickTrace`). No graph edits; Sphere nodes go dormant (can delete later).
- Or in BP: on the `Sphere Trace` event (`B29D7F7B`), break the exec wire into the trace node to orphan the sphere damage path.

**B. Keep both, add per-actor dedupe.**
1. Add variable `Hit Actors` = Array of Actor.
2. Before each `ApplyDamage`: `Hit Actors → Contains(HitActor)` → Branch. False → `Add Unique(HitActor)` then ApplyDamage; True → skip.
3. Clear `Hit Actors` at swing start (on `Start Stick Trace`, and wherever the sphere swing begins).

---

## Minor — Divide by zero warning in ABP_NoWeapon

**Status:** ⏸️ IGNORE until CC-version work resumes (CC track). Cosmetic. Low priority.

Runtime warning during PIE: `Script Msg: Divide by zero: Divide_DoubleDouble` from `ABP_NoWeapon_C` (the no-weapon AnimBP). Likely a speed/normalize/direction calc dividing by a zero magnitude when stationary. Add a guard (check denominator != 0, or use SafeDivide / a small epsilon) in the AnimBP's AnimGraph or EventGraph math.
