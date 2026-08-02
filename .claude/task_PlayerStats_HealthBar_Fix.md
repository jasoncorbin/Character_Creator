# Task — `BPC_PlayerStats` "Accessed None / As Dummy" fix

**Status:** waiting on 3 user-placed nodes, then I do the rest remotely.
**Scope:** edits stay entirely inside `BPC_PlayerStats` → `Decrease Health`. No changes to
`Dummy`, `BP_CC_Character`, `BP_RPG_Enemy`, or the EventGraph.
**Asset:** `/Game/CharacterCreator/BluePrints/BPC_PlayerStats` — open the **`Decrease Health`** function graph.

---

## 1. What's actually wrong (one paragraph)

`Decrease Health` picks its target widget with two **`Select`** nodes keyed on `isPlayer`.
A Blueprint `Select` evaluates **every** option chain eagerly, so both the player chain and the
Dummy chain get dereferenced on every call regardless of which one wins.

`BPC_PlayerStats` is owned by three actors:

| Owner | `isPlayer` | `As Dummy` | Result |
|---|---|---|---|
| `BP_CC_Character` | `true` | null | works (its own chain is valid) |
| `Dummy` | `false` | set | works |
| `BP_RPG_Enemy` | `false` *(CDO default — never assigned)* | **null** | **6 errors per hit** |

`BP_RPG_Enemy` is a **duplicate of `Dummy`, not a subclass of it** — same components
(`Health Bar Wigget`, `AssassinateWidget`, `AssassinRef`, `AssassinateRadius`), but both parent to
`Character`. So `Cast To Dummy` in BeginPlay fails on it, `As Dummy` is never set, and it still
routes down the Dummy branch of both Selects.

**Its health bar is NOT broken.** `BP_RPG_Enemy` drives its own bar from its own `Event AnyDamage`
graph (`Get Health Bar Wigget → Get Widget → Cast To WB_Dummy_Health → Set Health Bar UI` at
BeginPlay, then `Set Percent` after calling `Decrease Health`). The widget code inside
`Decrease Health` is redundant dead weight for this owner — we are silencing dead code, not
repairing a feature.

### Log evidence (PIE, confirmed)
All six errors are on `BP_RPG_Enemy_C_1.BPC_PlayerStats`, at fixed script offsets, per hit:

| Offset | Node | Message |
|---|---|---|
| `0121` | `Set Percent` | reading `As Dummy` ← the null |
| `0136` | `Set Percent` | Accessed None (chained) |
| `018E` | `Set Percent` | Accessed None (chained) |
| `0220` | `SetText` | reading `As Dummy` ← the null |
| `0235` | `SetText` | Accessed None (chained) |
| `028D` | `SetText` | Accessed None (chained) |

---

## 2. Target shape

One branch pair covers both widget writes, since they share the same condition.

**REVISED 2026-08-02** — the `Is Valid` *macro* got placed instead of the pure function, and it's
the better node for this: it's already a branch (`exec` in → `Is Valid` / `Is Not Valid` exec outs),
so it replaces both the planned `Branch (2)` and the pure `IsValid`. One fewer node.

```
[Set CurrentHealth]
      |
   Branch (1)  -- Condition: isPlayer
      |
      |--true--> Set Percent (a) --> SetText (b) -----------------+   existing nodes, rewired
      |                                                           |
      |--false-> [Is Valid] macro  -- InputObject: As Dummy       |
                     |                                            |
                     |--Is Valid-----> Set Percent (c) --> SetText (d) --+   NEW nodes
                     |                                            |
                     |--Is Not Valid------------------------------+   RPG enemy lands here
                                                                  |
                                                                  v
                                              Branch (CurrentHealth <= 0)   existing death check
```

`BP_RPG_Enemy` takes `false → false` and touches no widget at all.
`Dummy` and the player keep byte-identical behaviour.

---

## 3. YOUR STEPS — place 3 nodes

Open `BPC_PlayerStats` → `Decrease Health`. Leave everything else alone; do **not** delete the
`Select` nodes (I'll do that remotely, after the rewire, so the graph never sits in a broken state).

Put all three in the empty space **below** the existing `Set Percent` / `SetText` row.
Exact position doesn't matter — you can tidy up afterwards.

### ~~Node 1 — `Is Valid`~~ ✅ DONE
The exec **macro** version got placed (`261BA16B444D507DF372548E72267A14`). Keeping it — it doubles
as the branch. No action needed.

### Node 2 — `Set Percent` (Progress Bar) — ⛔ REDO REQUIRED
The first attempt produced a **`K2Node_VariableSet`** (a `Percent` property setter, `self` + `Percent`
pins) instead of the `Set Percent` **function call**. Deleted.

> **Why it matters:** `UProgressBar::SetPercent()` invalidates the Slate widget so it redraws.
> Writing the `Percent` property directly sets the value but never repaints — it fails silently.

**Do this instead — do not use right-click search:**
1. Click the **existing** `Set Percent` node in the top row (the one wired between `Set CurrentHealth`
   and `SetText`, at roughly x=608) — single left-click to select it, so it gets a highlight border.
2. Press **Ctrl+W**.
3. A copy appears slightly offset. Drag it down into the empty space near the `Is Valid` macro.

Verify before telling me: the copy's title must read **`Set Percent` / `Target is Progress Bar`**
(two lines) and it must have **white exec pins** on both sides. If it has no exec pins, or the title
is one line, it's the property setter again.

### ~~Node 3 — `SetText (Text)`~~ ✅ DONE
Correct `K2Node_CallFunction` placed (`0C1B9FDD4E7382D0DBC45692C0D219F7`), pins match the existing
node exactly. No action needed.

> **Why Ctrl+W and not right-click → search:** duplicating guarantees the correct function
> overload and target class. Adding these fresh is exactly the case that goes wrong when authored
> remotely, and `Set Percent` / `SetText` both have look-alike overloads in the palette.

**Do not wire anything.** Unconnected duplicates are harmless; the graph still compiles.

### Then
Save is not required — just tell me they're placed and I'll read the graph back to pick up the new
node GUIDs.

---

## 4. MY STEPS — ✅ APPLIED 2026-08-02, compiles clean, saved to disk

**New Branch node:** `863D702C49A6FB72733E77ABB18FEA96`
**Deleted:** both `Select`s (`134993A2…`, `085DBA68…`), both orphaned knots (`56AB254C…`,
`383CAB46…`), and the now-unused second `Get isPlayer` (`3C0620F3…`).

**Save verified 3 ways:** `save_asset` → `true` · `.uasset` mtime advanced · `git status` shows it
modified · `get_dirty_content_packages()` → `[]`.

⚠ **The new Branch spawned at (0,0), overlapping `Set CurrentHealth`.** Nodes added over MCP can't be
positioned — drag it clear when you're next in the graph. Cosmetic only.

### Original spec (kept for reference)

Recorded here so you can follow along / audit.

### 4a. Add the two Branch nodes
`K2Node_IfThenElse` authors correctly over MCP (verified previously) — no user step needed.

### 4b. Rewire (existing node GUIDs)

| Node | GUID |
|---|---|
| `Set CurrentHealth` | `FBD8BC644B69F7B6006906840209A35A` |
| `Set Percent` (a, player) | `7487534E4D0D2664A7854490E2D8C4B9` |
| `SetText` (b, player) | `AF0D9AD9437A28650DCE529B6FAEAC78` |
| `float / float` (CurrentHealth÷MaxHealth) | `0F8E352945D20E89E506E28D41A9FF8C` |
| `To Text (Float)` | `276011364C6F8F0064A9109275638B72` |
| death-check `Branch` | `157AA13D40CFBA2366A691A6C9AD344A` |
| `Get Health_Bar` (player chain) | `5BE7DAB44AACE8FCAE7CE7B8F8A96849` |
| `Get HelathText` (player chain) | `DD2E92184B958F6B93C900A8AA488C2B` |
| `Get HealthBar` (dummy chain) | `344A4A834B70E36F6E5B5F9D50A2B472` |
| `Get HelathText` (dummy chain) | `EFC36CBD48EFF9A1B2D6D6B56ECF6C03` |
| `Get As Dummy` | `B4BD08B04DF3F30C6FB46E81ECE76D28` |
| `Get isPlayer` | `2CEEF945461FB1B4F4D26BA5FB8F2E30` |

**Exec wiring**
- `Set CurrentHealth.then` → `Branch(1).execute`
- `Branch(1).then` → `(a).execute` · `(a).then` → `(b).execute` · `(b).then` → death `Branch` *(already linked)*
- `Branch(1).else` → `Branch(2).execute`
- `Branch(2).then` → `(c).execute` · `(c).then` → `(d).execute` · `(d).then` → death `Branch`
- `Branch(2).else` → death `Branch`

*(An exec input fed by multiple outputs is legal in BP — the three paths merge on the death check.)*

**Data wiring**
- `Branch(1).Condition` ← `Get isPlayer`
- `IsValid.Object` ← `Get As Dummy` · `Branch(2).Condition` ← `IsValid.ReturnValue`
- `(a).self` ← `Get Health_Bar` **direct** (bypassing the Select)
- `(b).self` ← `Get HelathText` (player) **direct**
- `(c).self` ← `Get HealthBar` (dummy) · `(d).self` ← `Get HelathText` (dummy)
- `(a).InPercent` and `(c).InPercent` ← the **same** `float / float` node
- `(b).InText` and `(d).InText` ← the **same** `To Text (Float)` node

*(Pure nodes can legally feed multiple consumers; only the branch actually taken evaluates them —
which is the whole point of the fix.)*

### 4c. Delete the dead nodes
- `Select` (ProgressBar) — `134993A24A1D205840EE49B2A666FE65`
- `Select` (TextBlock) — `085DBA684DB91A1F7370DAA545D55668`
- orphaned reroute knots — `56AB254C4358CB7BC9A127B4A1BC703A`, `383CAB4646AD1BD85C4E0FA4719E96A4`

### 4d. Compile + verify the save actually hit disk
Read-backs from memory lie — this bit the project before (step 3's E binding "verified" then vanished).
Three independent checks, all required:
1. `EditorLoadingAndSavingUtils.get_dirty_content_packages()` returns `[]`
2. `BPC_PlayerStats.uasset` **mtime + size changed** on disk
3. `git status` shows it modified

---

## 5. Verification (PIE)

**Pass criteria:**
- [ ] Hit an RPG enemy → **zero** `Accessed None` in the log (was 6 per hit)
- [ ] RPG enemy health bar still lowers
- [ ] RPG enemy death animation still fires when depleted, actor still destroys
- [ ] `Dummy` health bar + text still update (CC level — regression check)
- [ ] Blueprint compiles with no new warnings

---

## 6. Rollback

Single asset, uncommitted:
```
git checkout -- "Content/CharacterCreator/BluePrints/BPC_PlayerStats.uasset"
```
Then **restart the editor** (or the in-memory copy will just re-save over the restore).

---

## 7. Loose ends found while investigating

- **`search_assets` MCP tool is broken in this plugin build** — returns `{"total":0}` for every query,
  including `class_filter=Blueprint` over `/Game`. Use `blueprint_query list` instead.
- **Duplicate asset on disk:** `Content/CharacterCreator/BPC_PlayerStats.uasset` exists alongside
  the real `Content/CharacterCreator/BluePrints/BPC_PlayerStats.uasset`, and is **not in the asset
  registry**. Unresolved — worth identifying before it gets loaded by accident.
- **`.claude/handoff.md` needs a correction:** it states `BP_RPG_Enemy`'s health bar "has never
  worked." That is false — verified in PIE. Line needs fixing so a future session doesn't
  re-plan around it.
