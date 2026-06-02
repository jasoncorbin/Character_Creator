# CC_ Asset Cleanup Map — Character_Creator

**Generated:** 2026-06-01 · via MCPUnreal `search_assets` + `get_asset_info` (READ-ONLY; nothing modified or deleted).
**Purpose:** Map referencers/dependencies of every `CC_` animation & montage in `/Game/CharacterCreator` to scope a safe cleanup.

---

## ⚠️ Read this first — two key findings

### 1. These CC_ assets are NOT duplicates of a base asset
The task asked to match each `CC_` asset to "what base asset it was duplicated from (name minus CC_ prefix)." **No such base assets exist.** Every `CC_` anim is an **original FBX import** built on the project's own skeleton **`OneMeshCharacter01_Skeleton`** (a few also touch `OneMeshCharacter01_SK`, `CC_MS_Footsteps`). There is no non-CC twin to collapse them into — so "duplicated from" is **N/A** across the board. If the cleanup premise was "CC_ are dupes of stock assets," that premise doesn't hold here; they're the actual content.

### 2. The real duplication is at the BLUEPRINT level (this is likely the cleanup target)
- **Stale ObjectRedirectors** sit at the CharacterCreator root for `BP_CC_Character`, `BP_CC_Character1`, `BP_CC_Main` (leftovers from moving those BPs into `/BluePrints/` etc.). Redirectors are safe to fix via "Fix Up Redirectors in Folder."
- **`BP_CC_Character1`** references the **same montages** as `BP_CC_Character` (`CC_Die02_NoWeapon_Anim_Montage`, `CC_JumpFullSpinFWD_RM_NoWeapon_Anim_Montage`) — strongly suggesting **`BP_CC_Character1` is a stale duplicate** of the real player character. Confirm it's unused (not set as anyone's Default Pawn / not in any level) before considering removal — that's the high-value cleanup, not the anims.

---

## High-care assets (referenced by a Blueprint / AnimBP — do NOT delete without care)

| CC_ Asset | Type | Referenced by | Why it matters |
|---|---|---|---|
| CC_Idle_Battle_NoWeapon_Anim | AnimSequence | **ABP_NoWeapon** | Live AnimBP idle state |
| CC_Idle_Normal_NoWeapon_Anim | AnimSequence | **ABP_NoWeapon** | Live AnimBP idle state |
| CC_JumpAir_InPlace_NoWeapon_Anim | AnimSequence | **ABP_NoWeapon** | Live AnimBP jump state |
| CC_JumpEnd_InPlace_NoWeapon_Anim | AnimSequence | **ABP_NoWeapon** | Live AnimBP jump state |
| CC_JumpFull_InPlace_NoWeapon_Anim | AnimSequence | **ABP_NoWeapon** | Live AnimBP jump state |
| CC_MoveFWD_Normal_InPlace_NoWeapon_Anim | AnimSequence | **BS_Locomotion** → ABP_NoWeapon | In the locomotion blendspace |
| CC_WalkFWD_InPlace_NoWeapon_Anim | AnimSequence | **BS_Locomotion** → ABP_NoWeapon | In the locomotion blendspace |
| CC_RunFWD_Battle_InPlace_NoWeapon_Anim | AnimSequence | **BS_Locomotion** → ABP_NoWeapon | In the locomotion blendspace |
| CC_SprintFWD_Battle_InPlace_NoWeapon_Anim1 | AnimSequence | **BS_Locomotion** → ABP_NoWeapon | In the locomotion blendspace |
| CC_Combo01_InPlace_SingleSword_Anim_Montage | AnimMontage | **BPC_AttackSystem** | Combo 1 (also → BP_Notify_StickTrace) |
| CC_Combo02_InPlace_SingleSword_Anim_Montage | AnimMontage | **BPC_AttackSystem** | Combo 2 (also → BP_Notify_StickTrace) |
| CC_Combo03_InPlace_SingleSword_Anim_Montage | AnimMontage | **BPC_AttackSystem** | Combo 3 (also → BP_Notify_StickTrace) |
| CC_Combo04_InPlace_SingleSword_Anim_Montage | AnimMontage | **BPC_AttackSystem** | Combo 4 (also → BP_Notify_StickTrace) |
| CC_Combo05_InPlace_SingleSword_Anim_Montage | AnimMontage | **BPC_AttackSystem** | Combo 5 (no StickTrace notify) |
| CC_Die02_NoWeapon_Anim_Montage | AnimMontage | **BP_CC_Character**, **BP_CC_Character1** | Player death |
| CC_JumpFullSpinFWD_RM_NoWeapon_Anim_Montage | AnimMontage | **BP_CC_Character**, **BP_CC_Character1** | Vault (root motion) |
| CC_GetHit01_SingleSword_Anim_Montage | AnimMontage | **Dummy** | Enemy hit react |
| CC_GetHit02_SingleSword_Anim_Montage | AnimMontage | **Dummy** | Enemy hit react |
| CC_Die01Stay_NoWeapon_Anim_Montage | AnimMontage | **Dummy, Dummy1, Dummy2** | Enemy death |
| CC_Takedown_NoWeapon_Anim_Montage | AnimMontage | **Dummy, Dummy1, Dummy2** | Enemy takedown |

> Note: anims consumed only **inside** a montage (e.g. `CC_Die02_NoWeapon_Anim` ← its montage) are indirectly live too — deleting the anim breaks the montage that a BP uses. The `Takedown_NoWeapon_Anim` montage also internally references `CC_Die01Stay/Die01/GetHit01/GetHit02_NoWeapon_Anim` as composite segments.

---

## Apparently UNREFERENCED CC_ assets (candidate cleanup — verify before removing)

"Referenced By: none" (self-references from the registry are ignored). **Caveat:** the asset registry doesn't capture *soft* references or montages built at runtime, and several of these are clearly intended for **planned features** (level-up combos, dashes, rolls for the upcoming dodge). Flagged, not condemned.

### Likely intentional / reserved for planned features — KEEP
| CC_ Asset | Note |
|---|---|
| CC_RollFWD/BWD/LFT/RGT_Battle_InPlace_SingleSword_Anim | **Reserved for the dodge system** you're about to build. Keep. |
| CC_DashFWD/BWD/LFT/RGT_Battle_InPlace_SingleSword_Anim | Dodge/dash alternates. Keep. |
| CC_Combo02/03/04/05_InPlace_NoWeapon_Anim | Unarmed combo set (only NoWeapon combo **01** has a montage so far). Reserved for unarmed combat. |
| CC_Attack04_Start / CC_Attack04_Spinning / CC_Attack04 / CC_Attack01-03_SingleSword_Anim | Source attacks (combos are the wrapped/montaged versions); Attack4/5 tie to the level-up unlock plan. |
| CC_LevelUp_SingleSword_Anim | Level-up flavor (ties to Attack4/5 unlock). |
| CC_Combo01_InPlace_NoWeapon_Anim_Montage | Unarmed combo 1 montage — built but not yet wired to a BP. |
| CC_JumpFullSpin_RM_NoWeapon_Anim_Montage | Vault spin variant montage — not currently wired (its FWD sibling IS used by the character). |

### Flavor / one-off (lower stakes, still verify)
| CC_ Asset | Note |
|---|---|
| CC_Idle_Battle_SingleSword_Anim, CC_Idle_Normal_SingleSword_Anim | Armed idles — not in the (NoWeapon) AnimBP/blendspace currently. |
| CC_Move/Sprint{BWD,FWD,LFT,RGT}_Battle_InPlace_SingleSword_Anim | Armed locomotion set — not wired into the NoWeapon AnimBP. Would be used by an armed AnimBP if/when added. |
| CC_Jump* _InPlace_SingleSword_Anim (Air/AirSpin/DoubleJump/End/Full/FullSpin/Start) | Armed jump set — not currently wired. |
| CC_Wakl_Idle_Normal_NoWeapon_Anim | NoWeapon idle variant not in blendspace (note "Wakl" typo in name). |
| CC_Challenging / CC_Dance / CC_Victory / CC_SenseSomethingStart / CC_SenseSomethingSearching_SingleSword_Anim | Flavor/taunt anims, unreferenced. |
| CC_Defend / CC_Defendhit / CC_Dizzy / CC_GetUp / CC_Die01 / CC_Die01Stay / CC_Die02_SingleSword_Anim | Armed defend/hit/death set — unreferenced (the **SingleSword** GetHit *montages* are used by Dummy, but these specific **SingleSword anims** aren't, except via their own montages). |

---

## Full reference table (all 85 CC_ assets)

Legend: **Ref By** = external referencers (self/registry artifacts removed). **In Montage/BP?** = live via a Blueprint or AnimBP (directly or through a montage/blendspace).

| CC_ Asset | Type | Ref By | Live? |
|---|---|---|---|
| CC_Idle_Battle_NoWeapon_Anim | AnimSeq | ABP_NoWeapon | ✅ AnimBP |
| CC_Idle_Normal_NoWeapon_Anim | AnimSeq | ABP_NoWeapon | ✅ AnimBP |
| CC_JumpAir_InPlace_NoWeapon_Anim | AnimSeq | ABP_NoWeapon | ✅ AnimBP |
| CC_JumpEnd_InPlace_NoWeapon_Anim | AnimSeq | ABP_NoWeapon | ✅ AnimBP |
| CC_JumpFull_InPlace_NoWeapon_Anim | AnimSeq | ABP_NoWeapon | ✅ AnimBP |
| CC_MoveFWD_Normal_InPlace_NoWeapon_Anim | AnimSeq | BS_Locomotion | ✅ blendspace |
| CC_RunFWD_Battle_InPlace_NoWeapon_Anim | AnimSeq | BS_Locomotion | ✅ blendspace |
| CC_SprintFWD_Battle_InPlace_NoWeapon_Anim1 | AnimSeq | BS_Locomotion | ✅ blendspace |
| CC_WalkFWD_InPlace_NoWeapon_Anim | AnimSeq | BS_Locomotion | ✅ blendspace |
| CC_Wakl_Idle_Normal_NoWeapon_Anim | AnimSeq | none | ⬜ |
| CC_Idle_Battle_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_Idle_Normal_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_Attack01_SingleSword_Anim | AnimSeq | none | ⬜ source |
| CC_Attack02_SingleSword_Anim | AnimSeq | none | ⬜ source |
| CC_Attack03_SingleSword_Anim | AnimSeq | none | ⬜ source |
| CC_Attack04_SingleSword_Anim | AnimSeq | none | ⬜ source |
| CC_Attack04_Spinning_SingleSword_Anim | AnimSeq | none | ⬜ source |
| CC_Attack04_Start_SingleSword_Anim | AnimSeq | none | ⬜ source |
| CC_Combo01_InPlace_SingleSword_Anim | AnimSeq | CC_Combo01…_Montage | ✅ via montage→BPC_AttackSystem |
| CC_Combo02_InPlace_SingleSword_Anim | AnimSeq | CC_Combo02…_Montage | ✅ via montage→BPC_AttackSystem |
| CC_Combo03_InPlace_SingleSword_Anim | AnimSeq | CC_Combo03…_Montage | ✅ via montage→BPC_AttackSystem |
| CC_Combo04_InPlace_SingleSword_Anim | AnimSeq | CC_Combo04…_Montage | ✅ via montage→BPC_AttackSystem |
| CC_Combo05_InPlace_SingleSword_Anim | AnimSeq | CC_Combo05…_Montage | ✅ via montage→BPC_AttackSystem |
| CC_DashBWD_Battle_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ reserved (dodge) |
| CC_DashFWD_Battle_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ reserved (dodge) |
| CC_DashLFT_Battle_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ reserved (dodge) |
| CC_DashRGT_Battle_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ reserved (dodge) |
| CC_RollBWD_Battle_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ reserved (dodge) |
| CC_RollFWD_Battle_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ reserved (dodge) |
| CC_RollLFT_Battle_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ reserved (dodge) |
| CC_RollRGT_Battle_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ reserved (dodge) |
| CC_MoveBWD_Battle_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_MoveFWD_Battle_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_MoveFWD_Normal_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_MoveLFT_Battle_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_MoveRGT_Battle_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_SprintFWD_Battle_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_JumpAir_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_JumpAirSpin_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_JumpAirDoubleJump_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_JumpEnd_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_JumpFull_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_JumpFullSpin_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_JumpStart_InPlace_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_Challenging_SingleSword_Anim | AnimSeq | none | ⬜ flavor |
| CC_Dance_SingleSword_Anim | AnimSeq | none | ⬜ flavor |
| CC_LevelUp_SingleSword_Anim | AnimSeq | none | ⬜ reserved (level-up) |
| CC_SenseSomethingSearching_SingleSword_Anim | AnimSeq | none | ⬜ flavor |
| CC_SenseSomethingStart_SingleSword_Anim | AnimSeq | none | ⬜ flavor |
| CC_Victory_SingleSword_Anim | AnimSeq | none | ⬜ flavor |
| CC_Defend_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_Defendhit_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_Dizzy_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_GetUp_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_Die01_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_Die01Stay_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_Die02_SingleSword_Anim | AnimSeq | none | ⬜ |
| CC_GetHit01_SingleSword_Anim | AnimSeq | CC_GetHit01…_Montage | ✅ via montage→Dummy |
| CC_GetHit02_SingleSword_Anim | AnimSeq | CC_GetHit02…_Montage | ✅ via montage→Dummy |
| CC_Combo01_InPlace_NoWeapon_Anim | AnimSeq | CC_Combo01…NoWeapon_Montage | ⬜ montage exists, not BP-wired |
| CC_Combo02_InPlace_NoWeapon_Anim | AnimSeq | none | ⬜ |
| CC_Combo03_InPlace_NoWeapon_Anim | AnimSeq | none | ⬜ |
| CC_Combo04_InPlace_NoWeapon_Anim | AnimSeq | none | ⬜ |
| CC_Combo05_InPlace_NoWeapon_Anim | AnimSeq | none | ⬜ |
| CC_Die01_NoWeapon_Anim | AnimSeq | Takedown…_Anim (composite) | ✅ in Takedown montage→Dummy |
| CC_Die01Stay_NoWeapon_Anim | AnimSeq | CC_Die01Stay…_Montage, Takedown…_Anim | ✅ →Dummy(1/2) |
| CC_Die02_NoWeapon_Anim | AnimSeq | CC_Die02…_Montage | ✅ via montage→BP_CC_Character(1) |
| CC_GetHit01_NoWeapon_Anim | AnimSeq | Takedown…_Anim (composite) | ✅ in Takedown montage→Dummy |
| CC_GetHit02_NoWeapon_Anim | AnimSeq | Takedown…_Anim (composite) | ✅ in Takedown montage→Dummy |
| CC_Takedown_NoWeapon_Anim | AnimSeq | CC_Takedown…_Montage | ✅ via montage→Dummy(1/2) |
| CC_JumpFullSpinFWD_RM_NoWeapon_Anim | AnimSeq | CC_JumpFullSpinFWD…_Montage | ✅ via montage→BP_CC_Character(1) |
| CC_JumpFullSpin_RM_NoWeapon_Anim | AnimSeq | CC_JumpFullSpin…_Montage | ⬜ montage exists, not BP-wired |
| CC_Combo01_InPlace_SingleSword_Anim_Montage | Montage | BPC_AttackSystem | ✅ BP |
| CC_Combo02_InPlace_SingleSword_Anim_Montage | Montage | BPC_AttackSystem | ✅ BP |
| CC_Combo03_InPlace_SingleSword_Anim_Montage | Montage | BPC_AttackSystem | ✅ BP |
| CC_Combo04_InPlace_SingleSword_Anim_Montage | Montage | BPC_AttackSystem | ✅ BP |
| CC_Combo05_InPlace_SingleSword_Anim_Montage | Montage | BPC_AttackSystem | ✅ BP |
| CC_Combo01_InPlace_NoWeapon_Anim_Montage | Montage | none | ⬜ |
| CC_GetHit01_SingleSword_Anim_Montage | Montage | Dummy | ✅ BP |
| CC_GetHit02_SingleSword_Anim_Montage | Montage | Dummy | ✅ BP |
| CC_Die01Stay_NoWeapon_Anim_Montage | Montage | Dummy, Dummy1, Dummy2 | ✅ BP |
| CC_Die02_NoWeapon_Anim_Montage | Montage | BP_CC_Character, BP_CC_Character1 | ✅ BP |
| CC_Takedown_NoWeapon_Anim_Montage | Montage | Dummy, Dummy1, Dummy2 | ✅ BP |
| CC_JumpFullSpinFWD_RM_NoWeapon_Anim_Montage | Montage | BP_CC_Character, BP_CC_Character1 | ✅ BP |
| CC_JumpFullSpin_RM_NoWeapon_Anim_Montage | Montage | none | ⬜ |

---

## Recommended cleanup scope (in priority order)

1. **Fix Up Redirectors** in `/Game/CharacterCreator` (the 3 stale BP redirectors: BP_CC_Character, BP_CC_Character1, BP_CC_Main). Zero-risk housekeeping.
2. **Investigate `BP_CC_Character1`** — looks like a stale duplicate of the player character (shares the same montage refs). If it's not anyone's Default Pawn and not placed in a level, it (and anything only it references) is the biggest real cleanup win. **Confirm before touching.**
3. **Leave all dodge/dash/roll/level-up/unarmed-combo anims** — reserved for features in progress (dodge system next).
4. The remaining "none"/flavor anims (taunts, armed locomotion/jump sets not in the NoWeapon AnimBP) are *candidates* only — but asset-registry "unreferenced" ≠ safe, since an armed AnimBP or future wiring could need them. Recommend keeping until the armed AnimBP question is settled.

**Bottom line:** there are no CC_ "duplicates of base assets" to clean up — the CC_ anims are the project's real content. The actionable cleanup is the **redirectors** and the **possible duplicate `BP_CC_Character1`**, plus a decision on whether the armed (SingleSword) locomotion set will ever be wired.

---

## CC_ vs Base — skeleton & twin check (added 2026-06-01)

**Correction to the "no duplicates" finding above.** A whole-project search (not just `/Game/CharacterCreator`) shows the CC_ anims **DO have base twins** — they live in the **`RPGTinyHeroWavePBR`** asset pack. The earlier pass only searched the CharacterCreator folder, so it missed them. The CC_ assets are **project-local duplicates of the RPG Tiny Hero Wave pack animations** (same name minus the `CC_` prefix), re-saved into the project's folders.

### Twin coverage
- **68 of 72** CC_ AnimSequences have a base twin in `/Game/RPGTinyHeroWavePBR/Animation/...` (matched by name minus `CC_`).
- **4 CC_ anims have NO base twin** (project-original, no pack source):
  - `CC_RunFWD_Battle_InPlace_NoWeapon_Anim`
  - `CC_SprintFWD_Battle_InPlace_NoWeapon_Anim1`
  - `CC_Takedown_NoWeapon_Anim`
  - `CC_Wakl_Idle_Normal_NoWeapon_Anim` (note the "Wakl" typo — likely a hand-made/renamed clip)

### The one skeleton (key fact)
**Everything is bound to the SAME skeleton:** `OneMeshCharacter01_Skeleton`, which physically lives **inside the pack** at `/Game/RPGTinyHeroWavePBR/Mesh/OneMeshCharacter/OneMeshCharacter01_Skeleton`.
- CC_ copies → `OneMeshCharacter01_Skeleton`
- Base pack twins → `OneMeshCharacter01_Skeleton` (same)
- **`ABP_NoWeapon`** → TargetSkeleton = `OneMeshCharacter01_Skeleton` (same)
- **`BP_CC_Character`** → skeletal mesh `OneMeshCharacter01_SK` (the pack's mesh, which uses that skeleton); also depends on weapon mesh `OHS01_Stick_SM` from the pack.

➡️ **No retargeting is involved** — CC_ and base are same-skeleton copies. This also means **the project depends on the RPGTinyHeroWavePBR pack** for the skeleton + mesh; the pack is NOT pure throwaway content even though most of its 600+ loose anims are unused.

### CC_ vs base — identical or different?
Same source FBX (identical `FileMD5` import hashes), but **the CC_ copies were edited after duplication** — most importantly **Root Motion was toggled ON** for the movement set:

| Sample | CC_ path | Base twin path | Skeleton (both) | RootMotion CC_ | RootMotion Base | Frames | Verdict |
|---|---|---|---|---|---|---|---|
| RollFWD…SingleSword | /Game/CharacterCreator/CC_Animations/OneHandWeapon/OneHandMovement/CC_RollFWD_Battle_InPlace_SingleSword_Anim | /Game/RPGTinyHeroWavePBR/Animation/SingleSword/InPlace/RollFWD_Battle_InPlace_SingleSword_Anim | OneMeshCharacter01_Skeleton | **True** | False | 26 | Same anim, **RM enabled on CC_** |
| DashFWD…SingleSword | …/OneHandMovement/CC_DashFWD_Battle_InPlace_SingleSword_Anim | /Game/RPGTinyHeroWavePBR/Animation/SingleSword/InPlace/DashFWD_Battle_InPlace_SingleSword_Anim | OneMeshCharacter01_Skeleton | **True** | False | 19 | Same anim, **RM enabled on CC_** |
| Combo01…SingleSword | …/OneHandAttack/CC_Combo01_InPlace_SingleSword_Anim | /Game/RPGTinyHeroWavePBR/Animation/SingleSword/InPlace/Combo01_InPlace_SingleSword_Anim | OneMeshCharacter01_Skeleton | **True** | False | 15 | Same anim, **RM enabled on CC_** |
| Idle_Normal…NoWeapon | /Game/CharacterCreator/CC_Animations/Anim_Sequences/CC_Idle_Normal_NoWeapon_Anim | /Game/RPGTinyHeroWavePBR/Animation/NoWeapon/Idle_Normal_NoWeapon_Anim | OneMeshCharacter01_Skeleton | False | False | 141 | Same anim, identical settings |
| WalkFWD…NoWeapon | /Game/CharacterCreator/CC_Animations/Anim_Sequences/CC_WalkFWD_InPlace_NoWeapon_Anim | /Game/RPGTinyHeroWavePBR/Animation/NoWeapon/InPlace/WalkFWD_InPlace_NoWeapon_Anim | OneMeshCharacter01_Skeleton | False | False | 29 | Same anim, identical settings |

**Pattern:** the CC_ copies of the **movement/attack** clips (Roll, Dash, Combo) have **`bEnableRootMotion = True`** while their pack twins are `False`. The locomotion-blendspace clips (Idle, Walk, etc.) are unchanged (`False` both sides). So the CC_ duplicates exist specifically so the team could **enable root motion and add notifies** without touching the read-only pack — they are intentional working copies, not accidental dupes.

> 🔧 **Corrects the AnimationInventory note** that said the rolls are "InPlace → likely no root motion." The **CC_ roll/dash already have Root Motion ENABLED** (the `InPlace` is just the inherited pack name). For the dodge system, the CC_ roll is closer to ready than first thought — but confirm whether the root-motion translation is actually baked into the curve (name says InPlace) vs. just the flag being on.

### Base pack twins are referenced only by showcase maps
Each base twin's only referencer is a pack demo map (e.g. `/Game/RPGTinyHeroWavePBR/Map/SingleSword_Anim_Showcase_MP`), never by the game. So the **base pack anims are safe-to-ignore**, but the **pack's skeleton + mesh are load-bearing** (the character and AnimBP use them). Don't delete the pack wholesale.

### Implications for cleanup
1. The CC_ anims are **NOT redundant with the pack** for gameplay — they carry edited settings (root motion, and likely notifies on the combos). Keep them.
2. The **base pack anims** (the ~600 loose ones) are only used by pack showcase maps → candidate bulk cleanup IF you also remove the showcase maps AND keep the skeleton/mesh/needed twins. High effort, low urgency; defer.
3. There are also **stale move-redirectors inside the CC set** (e.g. an old `…/OneHandWeapon/CC_RollFWD…` path redirecting to the current `…/OneHandMovement/CC_RollFWD…`) — visible as self-referencers in the per-asset data. "Fix Up Redirectors" clears these.
4. **Dependency note:** because the skeleton/mesh live *in* `RPGTinyHeroWavePBR`, that pack folder cannot be fully deleted without first migrating `OneMeshCharacter01_Skeleton`, `OneMeshCharacter01_SK`, and `OHS01_Stick_SM` into the project. Worth doing eventually for a clean dependency graph; not required now.
