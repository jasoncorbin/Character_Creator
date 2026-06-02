# RPG Player Character — Architecture Plan (for review)

**Generated:** 2026-06-02 · READ-ONLY investigation via MCPUnreal. **Nothing was created, modified, or deleted.** Build begins only after approval.

**Goal:** A new strafe-movement, 8-weapon-stance, target-lock-ready player character in a fresh `RPG_` folder, built on `OneMeshCharacter01_Skeleton`, leaving all `CharacterCreator`/`CC_` assets untouched as an archive.

---

## 1. Connectivity
- `status`: editor **online**, MCPUnreal plugin **online** (port 8090). PIE not active. ✅

---

## 2. Inventory — what already exists (adopt/extend/build driver)

### 2a. The pack's "character" BPs are NOT playable characters
| BP | Parent class | Components | Variables | AnimBP dep | Verdict |
|---|---|---|---|---|---|
| `OneMeshCharacter_BP` (`/Game/RPGTinyHeroWavePBR/CharacterBP/`) | **SkeletalMeshActor** | 5 | 0 | none | **Display prop** for showcase maps. Not a Pawn, no movement, no input. |
| `ModularCharacter_BP` (same folder) | **SkeletalMeshActor** | 14 | 0 | none | **Display prop** — modular body/head/hair/weapon parts assembled for the modular demo. Not a Pawn. |

> ⚠️ **Critical correction to the brief's assumption.** `OneMeshCharacter_BP` is a `SkeletalMeshActor` (a posed mannequin for the demo maps), **not** an `ACharacter`. We **cannot "base the player on it"** in the inheritance sense — there is no movement, input, or controller logic to inherit. What we adopt from it is its **mesh + material setup recipe** (which `OneMeshCharacter01_SK` + materials + weapon meshes to use), not its class. The real player must be a **new `ACharacter`**.
>
> `ModularCharacter_BP` is the reference for **how to assemble a modular character** (it has 14 components: body, cloak, eye, hair, head, mouth, weapons as separate meshes on one skeleton) — exactly the recipe for later character-customization work. Keep it as the **pattern reference for the modular phase**, not a base class either.

### 2b. There IS a fully working playable combat character — but on the wrong skeleton
`BP_CombatCharacter` (`/Game/Variant_Combat/Blueprints/`) — UE's Third-Person **Combat template** Pawn:
- Parent **`Character`**, 329-node EventGraph, has `Move`/`Aim` functions, **28 gameplay variables** (Max/Current HP, full **combo system** [Combo Attack Montage, Combo Section Names, Combo Count, input-cache tolerances], **charged attack**, melee trace distance/radius/damage/knockback, camera shoulder offset/height, **danger trace** for lock-on-style targeting, respawn).
- Implements interfaces `BPI_Attacker`, `BPI_Damageable`, touch interface.
- **Movement model: `bOrientRotationToMovement = FALSE`** → this is **already strafe**, not turn-to-move. (Pawn faces via controller, not movement direction.)
- **BUT** its mesh is `SKM_Quinn_Simple` and its AnimBP is `ABP_Manny_Combat` whose **TargetSkeleton = `SK_Mannequin`** — the **Manny/Quinn skeleton, NOT `OneMeshCharacter01_Skeleton`.**

> So `BP_CombatCharacter` is a **gold-standard logic reference** (strafe + combo + charged + targeting already solved), but it's skeleton-incompatible with our RPG anims. We **port its patterns**, we don't reuse its AnimBP.

### 2c. Existing project player (the CC archive) is turn-to-move
`BP_CC_Character` (`/Game/CharacterCreator/BluePrints/`): **`bOrientRotationToMovement = TRUE`** → confirmed **turn-to-move** (the model the brief wants to replace). Its AnimBP `ABP_NoWeapon` is **single-stance** (NoWeapon only): direct deps are NoWeapon idle/jump anims + `BS_Locomotion` + `BS_Lean`. No stance enum, no multi-state-machine. **Not stance-aware.**

### 2d. Variant folders summary
| Folder | Player BP | AnimBP | Skeleton | Useful to us? |
|---|---|---|---|---|
| `Variant_Combat` | `BP_CombatCharacter` (Character) | `ABP_Manny_Combat` | SK_Mannequin | **Logic patterns** (strafe, combo, charged, lock-on targeting, hit react). Skeleton-incompatible. |
| `Variant_Platforming` | `BP_PlatformingCharacter` | `ABP_Manny_Platforming` | SK_Mannequin | Double-jump/dash patterns if wanted. Skeleton-incompatible. |
| `Variant_SideScroller` | `BP_SideScrollingCharacter` | `ABP_Manny_SideScroller` | SK_Mannequin | Not relevant (2.5D). |

All three Variants are Manny-skeleton template content — **reference only**, nothing adoptable as-is for our skeleton.

### 2e. There is NO stance-aware AnimBP anywhere in the project
AnimBP search (whole project): `ABP_NoWeapon` (CC, single-stance), 3× `ABP_Manny_*` (Manny skeleton), 2 engine tutorials, `ABP_Unarmed` (Manny). **None are on `OneMeshCharacter01_Skeleton` except `ABP_NoWeapon`, and that one is single-stance.** → The 8-stance AnimBP must be **built**.

### 2f. Showcase / demo maps (11, all under `/Game/RPGTinyHeroWavePBR/Map/`)
| Map | Demonstrates |
|---|---|
| `PBR_Default_OneMesh_MP` | The one-mesh character look (default materials). |
| `PBR_MaskTint_OneMesh_MP` | One-mesh with mask-tint material recoloring. |
| `PBR_Default_Modular_MP` | The modular character assembled from body parts. |
| `NoWeaponNormal_Anim_Showcase_MP` | NoWeapon "normal" (relaxed) locomotion set. |
| `NoWeaponBattle_Anim_Showcase_MP` | NoWeapon "battle" (combat-ready) set. |
| `SingleSword_Anim_Showcase_MP` | SingleSword full anim set. |
| `TwoHandsSword (THS)_Anim_Showcase_MP` | Two-hand sword set. |
| `SwordAndShield_Anim_Showcase_MP` | Sword + shield set. |
| `DoubleSword_Anim_Showcase_MP` | Dual-wield set. |
| `Spear_Anim_Showcase_MP` | Spear set. |
| `MagicWand_Anim_Showcase_MP` | Magic wand set. |
| `BowAndArrow_Anim_Showcase_MP` | Bow set (separate bow + arrow meshes). |

These are the **per-stance reference libraries** — open one to preview every clip a stance offers before wiring it.

---

## 3. Per-stance clip coverage (all 8 stances)

Source: `/Game/RPGTinyHeroWavePBR/Animation/<Stance>/`. Each stance ships ~72 clips (Bow 82, MagicWand 73, NoWeapon 94). **Every gameplay-critical category is present in every stance.** Clips come in **both `_InPlace_` (no root motion) and `_RM_` (root motion)** variants — important: we pick per-use (locomotion = InPlace driven by CMC; dodge/attacks = RM).

| Stance | Idle | Dir Move (FWD/BWD/LFT/RGT) | Sprint | Roll ×4 dir | Dash ×4 dir | Jump set | Attacks/Combos | Hit/Die |
|---|---|---|---|---|---|---|---|---|
| **NoWeapon** | ✅ (+Walk) | ✅ ×4 (InPlace+RM) | ✅ | ✅ | ✅ | ✅ (11) | ✅ combos | ✅ |
| **SingleSword** | ✅ | ✅ ×4 | ✅ | ✅ | ✅ | ✅ (11) | ✅ 16 | ✅ |
| **TwoHandsSword** | ✅ | ✅ ×4 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **SwordAndShield** | ✅ | ✅ ×4 | ✅ | ✅ | ✅ | ✅ | ✅ 16 | ✅ |
| **DoubleSword** | ✅ | ✅ ×4 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Spear** | ✅ | ✅ ×4 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **MagicWand** | ✅ | ✅ ×4 | ✅ | ✅ | ✅ | ✅ | ✅ 17 (cast) | ✅ |
| **BowAndArrow** | ✅ ×4 | ✅ ×4 | ✅ | ✅ | ✅ | ✅ | ✅ 24 (bow+arrow split) | ✅ |

### ⚠️ Flagged gaps / quirks (not blockers, but plan around them)
1. **No dedicated "Run" tier.** Speed clips are **Move (Battle = jog)** and **Sprint** only; there's no separate Walk→Run→Sprint triad. NoWeapon is the only stance with a **Walk** clip. → Blendspace speed axis will be **Idle → Move(jog) → Sprint** (3 points), with Walk available only for NoWeapon. Acceptable; just not a 4-tier speed ramp.
2. **Directional set is cardinal-only** (FWD/BWD/LFT/RGT) — **no baked diagonals.** A 2D blendspace **interpolates** the 4 cardinals into diagonals, so this is fine for strafe; quality of diagonal blend should be eyeballed per stance.
3. **BowAndArrow attacks are split into Bow + Arrow meshes** (e.g. `Attack01_Combo0102_Bow_Anim` + `..._Arrow_Anim`). The bow stance needs **two weapon meshes** (bow in hand, arrow spawned/attached) and the combat montages reference both. Most complex stance to wire — do it last.
4. **MagicWand combat is "cast" style** (17 clips) — may want projectile/VFX hooks later, not just melee traces. Flag for combat-system design.
5. **RM vs InPlace decision is per-clip and matters.** Locomotion should use **InPlace** (CMC moves the capsule); dodges/some attacks should use **RM** (animation moves the capsule, like the CC vault). Mixing wrong causes foot-slide or double-motion.

---

## 4. Proposed architecture (NOT yet built)

### 4a. ADOPT / EXTEND / BUILD per system
| System | Decision | Detail |
|---|---|---|
| **Player Character BP** | **BUILD new** (`BP_RPG_PlayerCharacter`, parent `ACharacter`) | Neither pack BP is a Pawn. Port proven patterns from `BP_CC_Character` (Enhanced Input wiring, target-lock we just built, vault/motion-warp) and `BP_CombatCharacter` (strafe CMC config, combo/charged structure). |
| **Movement** | **BUILD config** (trivial) on the new CMC | Set strafe flags (4d). Pattern proven by `BP_CombatCharacter`. |
| **AnimBP** | **BUILD new** (`ABP_RPG_Player` on `OneMeshCharacter01_Skeleton`) | No stance-aware AnimBP exists. Use `ABP_NoWeapon` only as a **structural reference** for one stance's state machine, then generalize. |
| **Stance system** | **BUILD new** (enum + driver) | Nothing like it exists. |
| **Combat** | **EXTEND pattern from `BP_CombatCharacter`** | Reuse its combo-cache/section-name approach and our existing `BPC_AttackSystem` ideas; re-author on the RPG montages. |
| **Target-lock** | **ADOPT our own** (just built in `BP_CC_Character`) | Port the IA_TargetLock + sphere-trace + FindLookAt-on-Tick logic; it's strafe-compatible by design. |
| **Skeleton / Mesh / Weapon meshes** | **ADOPT as-is** | `OneMeshCharacter01_Skeleton`, `OneMeshCharacter01_SK`, pack weapon SMs. Do not duplicate. |
| **Animations** | **ADOPT pack clips directly** (reference in place) | 600+ clips already on the right skeleton. Do **not** re-duplicate into RPG_ the way CC_ did — reference them. (Optional: duplicate only clips that need RM/notify edits, mirroring the CC_ working-copy pattern, into `RPG_/Anims/Edited/`.) |

### 4b. AnimBP strategy at 8 stances — **recommend: single AnimBP + Stance Enum driving a Layered Blend, with per-stance state machines via a stance sub-graph pattern**

Three options weighed **specifically at 8 stances**:

- **Per-stance AnimBPs (8 separate ABPs):** ❌ Rejected. 8× duplicated locomotion/jump/dodge graphs; any movement fix must be made 8 times. Swapping AnimBP at runtime (SetAnimInstanceClass) is heavier and loses cross-stance state. Maintenance nightmare at 8.
- **Single AnimBP, one giant state machine with stance branches inside every state:** ❌ Rejected. State count explodes (states × stances); unreadable at 8 stances.
- **✅ Single AnimBP with a Stance enum + "Stance Layers":** One AnimBP on the shared skeleton. A **`Stance` enum** (8 values) is the master driver. Locomotion **logic** (state machine: Idle/Move/Sprint/Jump/Fall/Dodge transitions) is authored **once**; the **clips** plugged into each state are chosen by stance. Two viable implementation mechanisms, recommend a hybrid:
  - **Blendspace-per-stance + select-by-enum:** each locomotion state evaluates a **stance-indexed array of 2D blendspaces** (one BS per stance) via a "Blend Poses by Enum" node. The state-machine graph is single-authored; only the leaf blendspace differs by stance. This is the cleanest for **locomotion** (8 stances × 1 blendspace each = 8 blendspaces, all driven by one graph).
  - **Layered Blend per Bone (upper/lower split)** for **combat while moving:** play attack/cast montages on an **upper-body slot** (`DefaultSlot`/custom `UpperBody` slot) layered over the locomotion lower body via `Layered blend per bone` at the spine. Lets the character strafe-walk while swinging — essential for action combat. This is **stance-agnostic** (montages are chosen by stance at the gameplay layer, the AnimBP just hosts the slot).

  **Why this wins at 8:** locomotion graph authored once (not 8×); adding/altering a movement behavior is one edit; stance differences are pure **data** (which blendspace / which montage), not duplicated **graph logic**. Scales identically whether 4 or 8 or 12 stances.

### 4c. Blendspace structure for camera-relative strafe
- **One 2D blendspace per stance** = **8 locomotion blendspaces** (`BS_RPG_Locomotion_<Stance>`).
- **Axes:**
  - **X = Direction** (−180..+180°), samples at the 4 cardinals: FWD(0), RGT(90), BWD(180/−180), LFT(−90). Blendspace interpolates diagonals.
  - **Y = Speed** (0..MaxSprint), samples: 0 = Idle, ~jog = Move(Battle), ~max = Sprint. (NoWeapon may add a Walk sample.)
- Direction is computed in the AnimBP from **velocity vs. actor forward** (`CalculateDirection`), the standard strafe input. Because the character faces camera/aim (not velocity), Direction varies full 360° → the 4-corner blendspace is exactly right.
- **Lean** (optional): the pack/CC has lean clips; a `BS_Lean`-style additive can layer on top later. Not required for v1.

### 4d. CharacterMovementComponent settings for camera-relative strafe (+ lock-on coexistence)
On the new `BP_RPG_PlayerCharacter`'s `CharacterMovementComponent` / Pawn:
| Setting | Value | Why |
|---|---|---|
| `bOrientRotationToMovement` | **FALSE** | Stop turn-to-move (this is the current CC behavior we're replacing). |
| `bUseControllerDesiredRotation` | **TRUE** | Character smoothly faces the **controller** yaw (mouse direction) → mouse controls facing. (Smoother than `bUseControllerRotationYaw` snapping.) |
| Pawn `bUseControllerRotationYaw` | **FALSE** | Let the CMC do the smoothing via DesiredRotation instead of hard-snapping the Pawn. |
| `RotationRate` (Yaw) | ~ 500–720°/s | Tune facing responsiveness. |
| WASD input | camera-relative `AddMovementInput` | `Move` adds input along **camera** forward/right (same math the pack's & CC's `Move` already use off control rotation) → WASD is camera-relative; combined with the facing rule above = **strafe**. |

**Lock-on coexistence:** our target-lock sets **Control Rotation** each tick via `FindLookAtRotation` toward the target. With `bUseControllerDesiredRotation = TRUE`, the character **automatically faces the locked target** (because it follows controller yaw) while WASD still strafes camera-relative around it. **This is exactly the lock-on strafe model** — the two systems compose with no conflict. (Contrast: `bOrientRotationToMovement=TRUE` would fight the lock by yanking facing toward WASD.) When unlocked, controller yaw follows the mouse → free strafe. **No mode-switch code needed**; lock-on just changes what drives control rotation.

### 4e. Stance-switching mechanism
- **`E_RPG_Stance`** enum: `NoWeapon, SingleSword, TwoHandsSword, SwordAndShield, DoubleSword, Spear, MagicWand, BowAndArrow` (8).
- A **`CurrentStance`** variable on the player (replicated-ready). Setter → updates AnimBP (via the AnimBP reading the Pawn each frame, or a BP interface call) and triggers **draw/sheath**.
- **Input:** an `IA_RPG_SwitchStance` (e.g. scroll/number keys/radial menu later). v1 can cycle or set directly for testing.
- **AnimBP consumption:** AnimBP `BlueprintUpdateAnimation` reads `CurrentStance` from the owning Pawn → drives the "Blend Poses by Enum" selecting the stance's blendspace + the correct idle/jump leaf clips. No AnimBP swap.
- **Weapon mesh attach:** each stance maps to one (or two, for Bow) **weapon static/skeletal meshes** attached to hand sockets on `OneMeshCharacter01_Skeleton`. A **DataTable / map `Stance → WeaponMesh(es) + Socket`** drives attach/detach on stance change. (The pack weapon meshes — `OHS03_Sword_SM`, `Shield08_SM`, bow/arrow, spear, wand, etc. — already exist and are sized to this skeleton; `OneMeshCharacter_BP`/`ModularCharacter_BP` show the socket recipe.) Draw/sheath uses the stance's equip montage on the upper-body slot.

### 4f. Skeleton-level guarantee (works for OneMesh now AND Modular later)
**Everything stance/movement-related is authored against `OneMeshCharacter01_Skeleton`, not a specific mesh:**
- The **AnimBP targets the skeleton** (`OneMeshCharacter01_Skeleton`), so any SkeletalMesh using that skeleton (the one-mesh `OneMeshCharacter01_SK` **or** the modular assembled body parts, which all share it) can run the same AnimBP unchanged.
- The **blendspaces/clips are skeleton-bound** (already are).
- The **stance enum, CMC config, input, target-lock, combat logic** live on the **Character BP** and are mesh-agnostic.
- **What would wrongly tie it to one mesh (avoid):** hard-coding a specific `SkeletalMesh` asset in logic, or putting weapon sockets/attach logic that assumes one body. **Mitigation:** drive mesh by variable/DataAsset; reference **sockets by name** (defined on the shared skeleton), not by mesh. Then the **Modular phase** = swap the mesh component(s) + reuse the same AnimBP/Character logic. Plan: make `BP_RPG_PlayerCharacter` accept its SkeletalMesh via a **Character DataAsset** so OneMesh = one asset, Modular = assembled parts, **same class**.

### 4g. Proposed `RPG_` folder structure (all new assets; CharacterCreator untouched)
```
/Game/RPG/
  Blueprints/
    BP_RPG_PlayerCharacter        (ACharacter — the player)
    BP_RPG_PlayerController
    BP_RPG_GameMode
    Components/
      BPC_RPG_Combat              (combo/charged logic, ported pattern)
      BPC_RPG_StanceManager       (stance enum state + weapon attach)
  Anim/
    ABP_RPG_Player               (single stance-aware AnimBP, on OneMeshCharacter01_Skeleton)
    Blendspaces/
      BS_RPG_Loco_NoWeapon ... BS_RPG_Loco_BowAndArrow   (8)
    Edited/                       (ONLY clips needing RM/notify edits — duplicated working copies)
  Input/
    IA_RPG_SwitchStance, IA_RPG_Dodge, IA_RPG_Attack, IA_RPG_TargetLock, ...
    IMC_RPG_Default
  Data/
    E_RPG_Stance                 (enum)
    DT_RPG_Stances               (Stance -> weapon mesh(es), socket, equip montage, blendspace, montage set)
    DA_RPG_Character_OneMesh     (mesh + materials recipe for the default character)
  Montages/
    <Stance>/ ...                (combat/dodge montages authored from pack clips)
  Maps/
    Lvl_RPG_Test                 (a clean test level)
```
- **Dependency note:** RPG_ will **reference** pack assets (skeleton, mesh, clips, weapon meshes) in place. That's fine and intended. (Long-term, optionally migrate skeleton+mesh+weapons into `/Game/RPG/Core/` to fully decouple from the pack folder — not required for v1.)

### 4h. Incremental build plan (validate one stance end-to-end first)
**Build order — prove the pattern on ONE stance, then replicate:**
1. **Foundations:** `E_RPG_Stance` enum, `BP_RPG_PlayerCharacter` (ACharacter) with strafe CMC config, camera boom + follow camera, Enhanced Input (Move/Look/Jump), `ABP_RPG_Player` shell on the skeleton, test level + GameMode/Controller. **Milestone: walk/strafe around with the mannequin mesh, no stance yet.**
2. **First stance, fully:** **SingleSword** (recommended first — richest moveset, clean single weapon mesh, has every category, and we already understand sword combat from `BPC_AttackSystem`). Build `BS_RPG_Loco_SingleSword`, wire it into the AnimBP locomotion state machine, jumps, **dodge (RM roll)**, weapon attach, **combat montages + upper-body slot**, target-lock. **Milestone: SingleSword is a complete, shippable stance with strafe + lock-on + combat + dodge.**
3. **Validate the pattern**, then **replicate** the data (blendspace + montage set + weapon mapping) for the other stances in this order: **NoWeapon → TwoHandsSword → SwordAndShield → DoubleSword → Spear → MagicWand → BowAndArrow** (Bow last: dual bow+arrow mesh + cast-style complexity; MagicWand second-last: cast/projectile design).
4. **Modular phase (later):** add `DA_RPG_Character_*` driving mesh selection; prove the same AnimBP/logic runs on the assembled modular body (pattern from `ModularCharacter_BP`).

> **Why SingleSword first, not NoWeapon:** the brief wants all 8 *weapon* stances; SingleSword exercises **weapon attach + armed combat + armed locomotion + dodge** (the full hard path) so the pattern we validate covers every later stance. NoWeapon is actually the *easy* case (no weapon mesh) and would leave weapon-attach unproven. Build the representative hard stance first.

---

## 5. UE API grounding (lookup_class)
- **`UCharacterMovementComponent`** (verified): exposes `bOrientRotationToMovement`, `MaxWalkSpeed`, `JumpZVelocity`, `RotationRate` (via `ACharacter`), movement modes — confirms the strafe-config approach in 4d is real API. (Note: `bUseControllerDesiredRotation` is a CMC UPROPERTY set in the editor Details panel / via set_property at build time.)
- **`ACharacter`** is the correct parent for the player (capsule + CMC + mesh + movement replication), confirmed as the base used by both `BP_CC_Character` and `BP_CombatCharacter`.
- `SkeletalMeshActor` (the pack BPs' parent) is a **non-Pawn display actor** — confirms it cannot serve as the player base.
- Strafe direction math uses **`UKismetAnimationLibrary::CalculateDirection`** (standard for strafe blendspaces) in the AnimBP; lock-on facing uses **`FindLookAtRotation`** + Control Rotation (already proven in our target-lock build).

---

## 6. Key recommendations (TL;DR)
1. **BUILD a new `ACharacter`** player — the pack's `OneMeshCharacter_BP`/`ModularCharacter_BP` are **SkeletalMeshActor display props**, not playable bases. Adopt their **mesh/weapon recipe**, not their class.
2. **Port logic patterns** from `BP_CombatCharacter` (already strafe + combo + targeting, but **Manny-skeleton** so not directly reusable) and from our own just-built **target-lock**.
3. **One stance-aware AnimBP** on `OneMeshCharacter01_Skeleton` using **Stance enum → Blend Poses by Enum → per-stance blendspace**, plus **Layered Blend per Bone** for attack-while-moving. Justified specifically because at **8 stances** per-stance AnimBPs or mega-state-machines don't scale.
4. **8 locomotion blendspaces** (one per stance), 2D = **Direction × Speed**, cardinal samples interpolated.
5. **Strafe CMC:** `bOrientRotationToMovement=FALSE`, `bUseControllerDesiredRotation=TRUE`, Pawn yaw off → **composes with target-lock for free** (lock just drives control rotation).
6. **Skeleton-level design** guarantees OneMesh-now / Modular-later reuse; drive mesh via DataAsset, reference sockets by name.
7. **Build SingleSword end-to-end first** (the representative hard path), then replicate data to the other 7; **Bow last**.

### Flagged gaps to acknowledge before building
- **No "Run" tier / Walk only in NoWeapon** → speed axis is Idle→Jog→Sprint (3 points).
- **Cardinal-only directional clips** (diagonals interpolated) — verify blend quality per stance.
- **BowAndArrow = dual bow+arrow meshes + 24 split clips** → most complex, last.
- **MagicWand = cast-style** → needs projectile/VFX combat design, not just melee traces.
- **RM vs InPlace** must be chosen per use (loco=InPlace, dodge/attacks=RM) to avoid foot-slide/double-motion.
- **Dependency on the pack folder** is intentional (skeleton/mesh/clips live there); optional later migration to fully decouple.

---

*Awaiting approval. No assets will be created or modified until the plan is approved.*
