# Task — Step 5: loot tables + dropper

**Status:** C++ built, data authored, component attached. **One node left for you**, then PIE.

---

## 1. What's already done

| Piece | State |
|---|---|
| `FLootEntry` / `FLootDrop` (`Loot/LootTypes.h`) | ✅ built |
| `ULootTable` + `RollLoot()` (`Loot/LootTable.*`) | ✅ built |
| `ULootDropperComponent` (`Loot/LootDropperComponent.*`) | ✅ built |
| Module rebuild | ✅ succeeded first try, editor target |
| `DA_Loot_Grunt` (`/Game/RPG/Data/`) | ✅ authored + saved |
| `LootDropper` component on `BP_RPG_Enemy` | ✅ added + configured + saved |

### Roll rates, verified empirically (2000 rolls, not just "it compiled")

| Entry | Spec | Observed | Counts seen |
|---|---|---|---|
| `DA_Item_Mat1` | 80% ×1–3 | 0.785 | 1, 2, 3 |
| `DA_Item_Mat2` | 30% ×1–2 | 0.290 | 1, 2 |
| `DA_Item_Mat3` | 5% ×1 | 0.050 | 1 |
| `DA_Item_THS01_Sword` | 25% ×1 | 0.257 | 1 |

### Component config as saved

```
loot_table                = DA_Loot_Grunt
world_item_class          = WorldItem            (C++ class; no BP child exists)
palette                   = DA_RarityPalette
gear_placeholder_mesh     = /Engine/BasicShapes/Cube
material_placeholder_mesh = /Engine/BasicShapes/Sphere
scatter_radius            = 60.0    (0.6 m, per spec)
spawn_height              = 60.0    (0.6 m, per spec)
placeholder_scale         = 0.3     (spec; the 4 hand-placed pickups use 0.25)
```

> **Why the placeholder meshes are mandatory, not cosmetic:** `DA_Item_Mat1/2/3` have **no
> `StaticMeshAsset`** — materials render purely from `PlaceholderMesh`. Without
> `material_placeholder_mesh` set, three of the four loot entries drop **invisible**: collectable,
> but with nothing rendered. There's a loud `UE_LOG` warning guarding this case.

---

## 2. ✅ DONE — call node placed and wired (2026-08-02)

User placed `Get LootDropper` (`0269EC9A439E35240E9A4FBE66DFA06C`) and `Drop Loot`
(`69B6AA0146251C8258E59BAF849AC467`). I inserted it between the `Branch`'s `true` pin and
`Play Montage`. Compiled clean; save verified (mtime advanced, `dirty=[]`, git modified).

Live chain:
```
Branch (IsPlayerDead?) --true--> [Drop Loot] --> Play Montage --> Play Sound --> Delay 2.0 --> Destroy Actor
```

**Next: PIE verification in section 3.**

### Original instructions (kept for reference)

Open **`BP_RPG_Enemy`** → **Event Graph**. Find the region around **`Event AnyDamage`**
(bottom of the graph, roughly y≈1550).

The existing death chain reads:

```
Decrease Health  -->  Set Percent  -->  Branch (IsPlayerDead?)
                                          |true
                                          +--> Play Montage (death)
                                          +--> Play Sound at Location
                                          +--> Delay 2.0
                                          +--> Destroy Actor
```

**Add the call:**

1. In the **Components** panel (top-left), find **`LootDropper`** — it's there now, added over MCP.
2. **Drag it into the Event Graph.** That creates a `Get LootDropper` node.
3. **Drag off its blue output pin**, release in empty space, and search **`Drop Loot`**.
4. Pick it. You now have `Get LootDropper` → `Drop Loot`, already wired on the data side.

Drop them somewhere clear near the `Branch`. **Don't wire the exec pins** — I'll do that, inserting
`Drop Loot` between the `Branch`'s `true` pin and `Play Montage`:

```
Branch (IsPlayerDead?)
  |true
  +--> [Drop Loot]          <-- inserted here, BEFORE the 2s delay
  +--> Play Montage (death)
  +--> Play Sound
  +--> Delay 2.0
  +--> Destroy Actor
```

Then tell me and I'll wire it, compile, and verify the save reached disk three ways.

---

## 3. Then: PIE verification

- [ ] Kill an enemy in **melee** → loot drops, scattered, visible
- [ ] Kill an enemy with an **arrow** → same, no crash
      *(this is the landmine: the whole death chain runs inside the projectile's overlap
      callback. `DropLoot` defers the spawn to the next tick specifically to get off that stack)*
- [ ] **Materials render as spheres, gear as cubes** — if a material is invisible, check the
      `LootDropper: MaterialPlaceholderMesh is unset` warning in the log
- [ ] Pick the drops up → they enter the inventory correctly
- [ ] **No double-drop** — the single-fire guard latches before the null-table check
- [ ] ~20 kills → rates look roughly like the table (sword ≈1 in 4, Mat3 ≈1 in 20)

---

## 4. Rollback

```
git checkout -- Content/RPG/Enemies/BP_RPG_Enemy.uasset
git clean -f  Content/RPG/Data/DA_Loot_Grunt.uasset
```
Restart the editor afterwards, or the in-memory copies re-save over the restore.
The C++ under `Source/Character_Creator/Loot/` is additive — nothing references it until the
call node exists, so it can be left in place.

---

## 5. Loose ends surfaced during this step

- **`Lvl_RPG_Test.umap` is now MODIFIED in git** and `ZZ_ArbTest_Priority50` is still in the level
  at `(360, 0, 75)`. The handoff said to leave that actor unsaved and delete it when done — it looks
  like the level got saved with it. Either run the priority-arbitration test (it's the one still
  unverified interaction path) or delete the actor and re-save.
- **Regenerating project files created hundreds of untracked `.vscode/compileCommands_*/*.rsp`
  files.** They're build artefacts and almost certainly want `.gitignore`-ing before the next commit.
- **No Blueprint child of `AWorldItem` exists** — the dropper spawns the raw C++ class. Fine today,
  but the moment you want per-item VFX via `OnItemVisualsApplied` (a `BlueprintImplementableEvent`),
  a BP child becomes necessary and `world_item_class` should be repointed at it.
