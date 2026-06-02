# Animation Asset Inventory — Character_Creator

**Generated:** 2026-06-01
**Source:** UE 5.7 editor via MCPUnreal `search_assets` (read-only; no assets modified).
**Scope:** Focuses on the project's own content under `/Game/CharacterCreator`. Third-party/template content (RPGTinyHeroWavePBR pack, UE template Variants, Mannequins, Engine tutorials) is summarized at the bottom rather than enumerated in full.

---

## ⭐ Key assets for the dodge system

The project ships **directional roll animations** — ideal for a dodge/roll. **Important:** they exist only as **AnimSequences, not Montages.** `Play Montage` needs a montage asset, so you'll need to **create a montage from the chosen roll sequence(s)** (right-click the sequence → Create → Create AnimMontage), or build one montage with directional sections.

| Likely-needed | Path | Note |
|---|---|---|
| `CC_RollFWD_Battle_InPlace_SingleSword_Anim` | `/Game/CharacterCreator/CC_Animations/OneHandWeapon/OneHandMovement/CC_RollFWD_Battle_InPlace_SingleSword_Anim` | Forward roll. **AnimSequence — needs montage created.** "InPlace" = likely no root motion baked; check before relying on root motion. |
| `CC_RollBWD_Battle_InPlace_SingleSword_Anim` | …/OneHandMovement/CC_RollBWD_Battle_InPlace_SingleSword_Anim | Back roll. AnimSequence. |
| `CC_RollLFT_Battle_InPlace_SingleSword_Anim` | …/OneHandMovement/CC_RollLFT_Battle_InPlace_SingleSword_Anim | Left roll. AnimSequence. |
| `CC_RollRGT_Battle_InPlace_SingleSword_Anim` | …/OneHandMovement/CC_RollRGT_Battle_InPlace_SingleSword_Anim | Right roll. AnimSequence. |
| `CC_Dash{FWD,BWD,LFT,RGT}_Battle_InPlace_SingleSword_Anim` | …/OneHandMovement/ | Directional **dashes** (alt dodge style). AnimSequences. |
| `AM_Dash` (template) | `/Game/Variant_Platforming/Anims/AM_Dash` | A ready-made dash **montage** from the UE Platforming template — reference for montage setup, but it's Manny/template content, not CC. |

> ⚠️ **"InPlace" naming:** every CC locomotion/roll/dash clip is tagged `InPlace`, suggesting root motion is *not* baked in (movement is driven by code, not the animation). The only **RM (root-motion)** clips are the vaulting spins (`*_RM_*`). If your dodge needs the character to physically travel via root motion, confirm/enable Root Motion on the roll, or drive displacement from the dodge logic. Flagged because you mentioned wanting root-motion + force-root-lock for the dodge.

---

## AnimMontages

### Project (`/Game/CharacterCreator`)

| Asset Name | Full Path | Notes |
|---|---|---|
| CC_Combo01_InPlace_SingleSword_Anim_Montage | /Game/CharacterCreator/CC_Animations/OneHandWeapon/OneHandAttack/CC_OneHandAttackMontoge/CC_Combo01_InPlace_SingleSword_Anim_Montage | **Combat** — combo 1 (sword). Used by BPC_AttackSystem. |
| CC_Combo02_InPlace_SingleSword_Anim_Montage | …/CC_OneHandAttackMontoge/CC_Combo02_InPlace_SingleSword_Anim_Montage | **Combat** — combo 2. |
| CC_Combo03_InPlace_SingleSword_Anim_Montage | …/CC_OneHandAttackMontoge/CC_Combo03_InPlace_SingleSword_Anim_Montage | **Combat** — combo 3. |
| CC_Combo04_InPlace_SingleSword_Anim_Montage | …/CC_OneHandAttackMontoge/CC_Combo04_InPlace_SingleSword_Anim_Montage | **Combat** — combo 4 (reserved/level-up). |
| CC_Combo05_InPlace_SingleSword_Anim_Montage | …/CC_OneHandAttackMontoge/CC_Combo05_InPlace_SingleSword_Anim_Montage | **Combat** — combo 5 (reserved/level-up). |
| CC_Combo01_InPlace_NoWeapon_Anim_Montage | /Game/CharacterCreator/CC_Attack/CC_Combo01_InPlace_NoWeapon_Anim_Montage | **Combat** — unarmed combo 1. |
| Assassination_01_Anim_Sequence_Montage | /Game/CharacterCreator/CC_Attack/Assassination_01_Anim_Sequence_Montage | **Combat** — assassination (IA_Assassinate). |
| CC_GetHit01_SingleSword_Anim_Montage | /Game/CharacterCreator/CC_Animations/OneHandWeapon/OneHanded_GetHit/CC_GetHitMontoge/CC_GetHit01_SingleSword_Anim_Montage | Hit reaction. |
| CC_GetHit02_SingleSword_Anim_Montage | …/CC_GetHitMontoge/CC_GetHit02_SingleSword_Anim_Montage | Hit reaction. |
| CC_Die01Stay_NoWeapon_Anim_Montage | /Game/CharacterCreator/CC_GetHit/CC_Die01Stay_NoWeapon_Anim_Montage | Death. |
| CC_Die02_NoWeapon_Anim_Montage | /Game/CharacterCreator/CC_GetHit/CC_Die02_NoWeapon_Anim_Montage | Death. |
| CC_Takedown_NoWeapon_Anim_Montage | /Game/CharacterCreator/CC_GetHit/CC_Takedown_NoWeapon_Anim_Montage | Takedown/finisher. |
| CC_JumpFullSpin_RM_NoWeapon_Anim_Montage | /Game/CharacterCreator/CC_Vaulting/CC_JumpFullSpin_RM_NoWeapon_Anim_Montage | **Locomotion** — vault spin. **Root motion (RM).** Used by vault system. |
| CC_JumpFullSpinFWD_RM_NoWeapon_Anim_Montage | /Game/CharacterCreator/CC_Vaulting/CC_JumpFullSpinFWD_RM_NoWeapon_Anim_Montage | **Locomotion** — vault spin fwd. **Root motion (RM).** |

### Template / non-project

| Asset Name | Full Path | Notes |
|---|---|---|
| AM_ChargedAttack | /Game/Variant_Combat/Anims/AM_ChargedAttack | UE Combat template. |
| AM_ComboAttack | /Game/Variant_Combat/Anims/AM_ComboAttack | UE Combat template. |
| AM_Dash | /Game/Variant_Platforming/Anims/AM_Dash | UE Platforming template — **dash montage reference for dodge**. |
| MM_Pistol_Fire_Montage | /Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Fire_Montage | Mannequin template. |

*Montage total: 18 (14 project + 4 template).*

---

## BlendSpaces

| Asset Name | Full Path | Notes |
|---|---|---|
| BS_Locomotion | /Game/CharacterCreator/CC_Animations/Locomotion/BS_Locomotion | **Locomotion** — 1D. Main movement blendspace (used by ABP_NoWeapon). |
| BS_Lean | /Game/CharacterCreator/CC_Animations/Locomotion/BS_Lean | **Locomotion** — 1D lean (turn lean). |
| BS_Idle_Walk_Run | /Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run | Template (Manny). 2D. |
| AO_Pistol | /Game/Characters/Mannequins/Anims/Pistol/Aim/AO_Pistol | Template aim offset. |
| AO_Rifle | /Game/Characters/Mannequins/Anims/Rifle/AIM/AO_Rifle | Template aim offset. |
| NewBlendSpace1D | /Engine/Tutorial/SubEditors/TutorialAssets/Character/NewBlendSpace1D | Engine tutorial asset (ignore). |

*BlendSpace total: 6 (2 project + 4 template/engine).*

---

## AnimBlueprints

| Asset Name | Full Path | Notes |
|---|---|---|
| ABP_NoWeapon | /Game/CharacterCreator/CC_Animations/Locomotion/ABP_NoWeapon | **The project's character AnimBP.** Drives locomotion via BS_Locomotion. (Known minor: divide-by-zero PIE warning — see `.claude/bugs_to_fix.md`.) Dodge state/slot will likely live here. |
| ABP_Manny_Combat | /Game/Variant_Combat/Anims/ABP_Manny_Combat | UE Combat template. |
| ABP_Manny_Platforming | /Game/Variant_Platforming/Anims/ABP_Manny_Platforming | UE Platforming template. |
| ABP_Manny_SideScroller | /Game/Variant_SideScroller/Anims/ABP_Manny_SideScroller | UE SideScroller template. |
| ABP_Unarmed | /Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed | Mannequin template. |
| TutorialAnimationBlueprint | /Engine/Tutorial/SubEditors/TutorialAssets/TutorialAnimationBlueprint | Engine tutorial (ignore). |
| TutorialTPP_AnimBlueprint | /Engine/Tutorial/SubEditors/TutorialAssets/Character/TutorialTPP_AnimBlueprint | Engine tutorial (ignore). |

*AnimBlueprint total: 7 (1 project + 6 template/engine).*

---

## AnimSequences — Project (`/Game/CharacterCreator`), 75 total

### Locomotion — NoWeapon (`CC_Animations/Anim_Sequences/`)
| Asset Name | Full Path | Notes |
|---|---|---|
| CC_Idle_Normal_NoWeapon_Anim | …/Anim_Sequences/CC_Idle_Normal_NoWeapon_Anim | **Locomotion** idle. |
| CC_Idle_Battle_NoWeapon_Anim | …/Anim_Sequences/CC_Idle_Battle_NoWeapon_Anim | **Locomotion** battle idle. |
| CC_Wakl_Idle_Normal_NoWeapon_Anim | …/Anim_Sequences/CC_Wakl_Idle_Normal_NoWeapon_Anim | **Locomotion** (note: "Wakl" typo in asset name). |
| CC_WalkFWD_InPlace_NoWeapon_Anim | …/Anim_Sequences/CC_WalkFWD_InPlace_NoWeapon_Anim | **Locomotion** walk fwd. |
| CC_MoveFWD_Normal_InPlace_NoWeapon_Anim | …/Anim_Sequences/CC_MoveFWD_Normal_InPlace_NoWeapon_Anim | **Locomotion** move fwd. |
| CC_RunFWD_Battle_InPlace_NoWeapon_Anim | …/Anim_Sequences/CC_RunFWD_Battle_InPlace_NoWeapon_Anim | **Locomotion** run fwd. |
| CC_SprintFWD_Battle_InPlace_NoWeapon_Anim1 | …/Anim_Sequences/CC_SprintFWD_Battle_InPlace_NoWeapon_Anim1 | **Locomotion** sprint fwd. |
| Left_Lean_Normal_InPlace_NoWeapon_Anim1 | …/Anim_Sequences/Left_Lean_Normal_InPlace_NoWeapon_Anim1 | **Locomotion** lean (BS_Lean). |
| Right_Lean_Normal_InPlace_NoWeapon_Anim2 | …/Anim_Sequences/Right_Lean_Normal_InPlace_NoWeapon_Anim2 | **Locomotion** lean (BS_Lean). |
| CC_JumpAir_InPlace_NoWeapon_Anim | …/Anim_Sequences/CC_JumpAir_InPlace_NoWeapon_Anim | **Locomotion** jump (air). |
| CC_JumpEnd_InPlace_NoWeapon_Anim | …/Anim_Sequences/CC_JumpEnd_InPlace_NoWeapon_Anim | **Locomotion** jump (land). |
| CC_JumpFull_InPlace_NoWeapon_Anim | …/Anim_Sequences/CC_JumpFull_InPlace_NoWeapon_Anim | **Locomotion** jump (full). |

### Movement — SingleSword (`CC_Animations/OneHandWeapon/OneHandMovement/`)
| Asset Name | Full Path | Notes |
|---|---|---|
| CC_RollFWD_Battle_InPlace_SingleSword_Anim | …/OneHandMovement/CC_RollFWD_Battle_InPlace_SingleSword_Anim | **⭐ DODGE/ROLL** fwd. |
| CC_RollBWD_Battle_InPlace_SingleSword_Anim | …/OneHandMovement/CC_RollBWD_Battle_InPlace_SingleSword_Anim | **⭐ DODGE/ROLL** back. |
| CC_RollLFT_Battle_InPlace_SingleSword_Anim | …/OneHandMovement/CC_RollLFT_Battle_InPlace_SingleSword_Anim | **⭐ DODGE/ROLL** left. |
| CC_RollRGT_Battle_InPlace_SingleSword_Anim | …/OneHandMovement/CC_RollRGT_Battle_InPlace_SingleSword_Anim | **⭐ DODGE/ROLL** right. |
| CC_DashFWD_Battle_InPlace_SingleSword_Anim | …/OneHandMovement/CC_DashFWD_Battle_InPlace_SingleSword_Anim | **Dodge alt** — dash fwd. |
| CC_DashBWD_Battle_InPlace_SingleSword_Anim | …/OneHandMovement/CC_DashBWD_Battle_InPlace_SingleSword_Anim | **Dodge alt** — dash back. |
| CC_DashLFT_Battle_InPlace_SingleSword_Anim | …/OneHandMovement/CC_DashLFT_Battle_InPlace_SingleSword_Anim | **Dodge alt** — dash left. |
| CC_DashRGT_Battle_InPlace_SingleSword_Anim | …/OneHandMovement/CC_DashRGT_Battle_InPlace_SingleSword_Anim | **Dodge alt** — dash right. |
| CC_MoveFWD_Battle_InPlace_SingleSword_Anim | …/OneHandMovement/CC_MoveFWD_Battle_InPlace_SingleSword_Anim | **Locomotion** (armed) fwd. |
| CC_MoveBWD_Battle_InPlace_SingleSword_Anim | …/OneHandMovement/CC_MoveBWD_Battle_InPlace_SingleSword_Anim | **Locomotion** (armed) back. |
| CC_MoveLFT_Battle_InPlace_SingleSword_Anim | …/OneHandMovement/CC_MoveLFT_Battle_InPlace_SingleSword_Anim | **Locomotion** (armed) left. |
| CC_MoveRGT_Battle_InPlace_SingleSword_Anim | …/OneHandMovement/CC_MoveRGT_Battle_InPlace_SingleSword_Anim | **Locomotion** (armed) right. |
| CC_MoveFWD_Normal_InPlace_SingleSword_Anim | …/OneHandMovement/CC_MoveFWD_Normal_InPlace_SingleSword_Anim | **Locomotion** (armed, normal) fwd. |
| CC_SprintFWD_Battle_InPlace_SingleSword_Anim | …/OneHandMovement/CC_SprintFWD_Battle_InPlace_SingleSword_Anim | **Locomotion** sprint (armed). |
| CC_JumpStart_InPlace_SingleSword_Anim | …/OneHandMovement/CC_JumpStart_InPlace_SingleSword_Anim | **Locomotion** jump start. |
| CC_JumpAir_InPlace_SingleSword_Anim | …/OneHandMovement/CC_JumpAir_InPlace_SingleSword_Anim | **Locomotion** jump air. |
| CC_JumpEnd_InPlace_SingleSword_Anim | …/OneHandMovement/CC_JumpEnd_InPlace_SingleSword_Anim | **Locomotion** jump end. |
| CC_JumpFull_InPlace_SingleSword_Anim | …/OneHandMovement/CC_JumpFull_InPlace_SingleSword_Anim | **Locomotion** jump full. |
| CC_JumpFullSpin_InPlace_SingleSword_Anim | …/OneHandMovement/CC_JumpFullSpin_InPlace_SingleSword_Anim | **Locomotion** jump spin. |
| CC_JumpAirSpin_InPlace_SingleSword_Anim | …/OneHandMovement/CC_JumpAirSpin_InPlace_SingleSword_Anim | **Locomotion** jump air spin. |
| CC_JumpAirDoubleJump_InPlace_SingleSword_Anim | …/OneHandMovement/CC_JumpAirDoubleJump_InPlace_SingleSword_Anim | **Locomotion** double jump. |

### Attacks — SingleSword (`CC_Animations/OneHandWeapon/OneHandAttack/`)
| Asset Name | Full Path | Notes |
|---|---|---|
| CC_Attack01_SingleSword_Anim | …/OneHandAttack/Attack/CC_Attack01_SingleSword_Anim | **Combat** attack 1. |
| CC_Attack02_SingleSword_Anim | …/OneHandAttack/Attack/CC_Attack02_SingleSword_Anim | **Combat** attack 2. |
| CC_Attack03_SingleSword_Anim | …/OneHandAttack/Attack/CC_Attack03_SingleSword_Anim | **Combat** attack 3. |
| CC_Attack04_SingleSword_Anim | …/OneHandAttack/Attack/CC_Attack04_SingleSword_Anim | **Combat** attack 4. |
| CC_Attack04_Start_SingleSword_Anim | …/OneHandAttack/Attack/CC_Attack04_Start_SingleSword_Anim | **Combat** attack 4 start. |
| CC_Attack04_Spinning_SingleSword_Anim | …/OneHandAttack/Attack/CC_Attack04_Spinning_SingleSword_Anim | **Combat** attack 4 spin. |
| CC_Combo01_InPlace_SingleSword_Anim | …/OneHandAttack/CC_Combo01_InPlace_SingleSword_Anim | **Combat** combo 1 (source for montage). |
| CC_Combo02_InPlace_SingleSword_Anim | …/OneHandAttack/CC_Combo02_InPlace_SingleSword_Anim | **Combat** combo 2. |
| CC_Combo03_InPlace_SingleSword_Anim | …/OneHandAttack/CC_Combo03_InPlace_SingleSword_Anim | **Combat** combo 3. |
| CC_Combo04_InPlace_SingleSword_Anim | …/OneHandAttack/CC_Combo04_InPlace_SingleSword_Anim | **Combat** combo 4. |
| CC_Combo05_InPlace_SingleSword_Anim | …/OneHandAttack/CC_Combo05_InPlace_SingleSword_Anim | **Combat** combo 5. |

### Unarmed attacks (`CC_Attack/`)
| Asset Name | Full Path | Notes |
|---|---|---|
| CC_Combo01_InPlace_NoWeapon_Anim | /Game/CharacterCreator/CC_Attack/CC_Combo01_InPlace_NoWeapon_Anim | **Combat** unarmed combo 1. |
| CC_Combo02_InPlace_NoWeapon_Anim | /Game/CharacterCreator/CC_Attack/CC_Combo02_InPlace_NoWeapon_Anim | **Combat** unarmed combo 2. |
| CC_Combo03_InPlace_NoWeapon_Anim | /Game/CharacterCreator/CC_Attack/CC_Combo03_InPlace_NoWeapon_Anim | **Combat** unarmed combo 3. |
| CC_Combo04_InPlace_NoWeapon_Anim | /Game/CharacterCreator/CC_Attack/CC_Combo04_InPlace_NoWeapon_Anim | **Combat** unarmed combo 4. |
| CC_Combo05_InPlace_NoWeapon_Anim | /Game/CharacterCreator/CC_Attack/CC_Combo05_InPlace_NoWeapon_Anim | **Combat** unarmed combo 5. |
| Assassination_01_Anim_Sequence | /Game/CharacterCreator/CC_Attack/Assassination_01_Anim_Sequence | **Combat** assassination source. |

### Hit reactions / death — SingleSword (`CC_Animations/OneHandWeapon/OneHanded_GetHit/`)
| Asset Name | Full Path | Notes |
|---|---|---|
| CC_GetHit01_SingleSword_Anim | …/OneHanded_GetHit/CC_GetHit01_SingleSword_Anim | Hit reaction. |
| CC_GetHit02_SingleSword_Anim | …/OneHanded_GetHit/CC_GetHit02_SingleSword_Anim | Hit reaction. |
| CC_Defend_SingleSword_Anim | …/OneHanded_GetHit/CC_Defend_SingleSword_Anim | **Combat** block/defend. |
| CC_Defendhit_SingleSword_Anim | …/OneHanded_GetHit/CC_Defendhit_SingleSword_Anim | **Combat** block-hit. |
| CC_Dizzy_SingleSword_Anim | …/OneHanded_GetHit/CC_Dizzy_SingleSword_Anim | Stun. |
| CC_GetUp_SingleSword_Anim | …/OneHanded_GetHit/CC_GetUp_SingleSword_Anim | Recovery. |
| CC_Die01_SingleSword_Anim | …/OneHanded_GetHit/CC_Die01_SingleSword_Anim | Death. |
| CC_Die01Stay_SingleSword_Anim | …/OneHanded_GetHit/CC_Die01Stay_SingleSword_Anim | Death (stay). |
| CC_Die02_SingleSword_Anim | …/OneHanded_GetHit/CC_Die02_SingleSword_Anim | Death. |

### Hit reactions / death — NoWeapon (`CC_GetHit/`)
| Asset Name | Full Path | Notes |
|---|---|---|
| CC_GetHit01_NoWeapon_Anim | /Game/CharacterCreator/CC_GetHit/CC_GetHit01_NoWeapon_Anim | Hit reaction. |
| CC_GetHit02_NoWeapon_Anim | /Game/CharacterCreator/CC_GetHit/CC_GetHit02_NoWeapon_Anim | Hit reaction. |
| CC_Takedown_NoWeapon_Anim | /Game/CharacterCreator/CC_GetHit/CC_Takedown_NoWeapon_Anim | Takedown. |
| CC_Die01_NoWeapon_Anim | /Game/CharacterCreator/CC_GetHit/CC_Die01_NoWeapon_Anim | Death. |
| CC_Die01Stay_NoWeapon_Anim | /Game/CharacterCreator/CC_GetHit/CC_Die01Stay_NoWeapon_Anim | Death (stay). |
| CC_Die02_NoWeapon_Anim | /Game/CharacterCreator/CC_GetHit/CC_Die02_NoWeapon_Anim | Death. |

### Vaulting — root motion (`CC_Vaulting/`)
| Asset Name | Full Path | Notes |
|---|---|---|
| CC_JumpFullSpin_RM_NoWeapon_Anim | /Game/CharacterCreator/CC_Vaulting/CC_JumpFullSpin_RM_NoWeapon_Anim | **Locomotion** vault spin. **Root motion.** |
| CC_JumpFullSpinFWD_RM_NoWeapon_Anim | /Game/CharacterCreator/CC_Vaulting/CC_JumpFullSpinFWD_RM_NoWeapon_Anim | **Locomotion** vault spin fwd. **Root motion.** |

### Random / flavor — SingleSword (`CC_Animations/OneHandWeapon/OneHand_RandomMovements/`)
| Asset Name | Full Path | Notes |
|---|---|---|
| CC_Challenging_SingleSword_Anim | …/OneHand_RandomMovements/CC_Challenging_SingleSword_Anim | Taunt/flavor. |
| CC_Dance_SingleSword_Anim | …/OneHand_RandomMovements/CC_Dance_SingleSword_Anim | Flavor. |
| CC_LevelUp_SingleSword_Anim | …/OneHand_RandomMovements/CC_LevelUp_SingleSword_Anim | Level-up (ties to Attack4/5 unlock plan). |
| CC_Victory_SingleSword_Anim | …/OneHand_RandomMovements/CC_Victory_SingleSword_Anim | Flavor. |
| CC_SenseSomethingStart_SingleSword_Anim | …/OneHand_RandomMovements/CC_SenseSomethingStart_SingleSword_Anim | Flavor. |
| CC_SenseSomethingSearching_SingleSword_Anim | …/OneHand_RandomMovements/CC_SenseSomethingSearching_SingleSword_Anim | Flavor. |

---

## Template / third-party AnimSequences (not enumerated)

These are **not** part of the project's own content; left out of the tables above to keep the inventory focused.

| Source pack / folder | Count | Notes |
|---|---|---|
| `/Game/RPGTinyHeroWavePBR/…` | 609 | Third-party "RPG Tiny Hero Wave PBR" animation pack (large library of generic RPG anims). Mine here if you want extra dodge/roll variety, but none are wired into CC. |
| `/Game/Characters/Mannequins/…` | 97 | UE Mannequin (Manny/Quinn) template anims — Unarmed, Pistol, Rifle sets. |
| `/Engine/…` | 2 | Engine tutorial assets. Ignore. |

**Grand total AnimSequences in project registry: 783** (75 in `/Game/CharacterCreator`, 708 template/third-party/engine).

---

## Notes for wiring the dodge system

1. **Rolls are AnimSequences, not Montages** — create a montage from `CC_RollFWD_Battle_InPlace_SingleSword_Anim` (and/or the other 3 directions) before using `Play Montage`. A single montage with FWD/BWD/LFT/RGT **sections** is a clean approach for directional dodge.
2. **Root motion**: CC rolls are `InPlace` (likely no baked root motion). For the dodge to move the character you'll either enable Root Motion on the roll asset (per your plan: Root Motion + Force Root Lock) or drive displacement from the dodge logic. Confirm in the sequence's asset details before wiring.
3. **AnimBP**: `ABP_NoWeapon` is the live character AnimBP — a dodge **slot**/state likely belongs there (and the dodge montage must play on a slot that AnimBP exposes).
4. **Weapon variants**: rolls/dashes exist only in the **SingleSword** set, not NoWeapon. If the character can be unarmed, there's no unarmed roll — note for design.
