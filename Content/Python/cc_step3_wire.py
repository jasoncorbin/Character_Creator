# cc_step3_wire.py — everything about step 3's editor wiring that CAN be scripted.
#
# Run from the UE editor:  Output Log -> Cmd box -> paste:
#     py "E:/UE5 Projects/Character_Creator/Content/Python/cc_step3_wire.py"
#
# Does, idempotently:
#   1. verifies the new C++ classes registered
#   2. adds UInventoryComponent + UInteractorComponent to BP_RPG_PlayerCharacter
#   3. creates /Game/RPG/Input/IA_RPG_Interact (digital/bool) if it does not exist
#   4. spawns 4 test AWorldItems, but ONLY when Lvl_RPG_Test is the open level
#
# Does NOT (neither is reachable from script - both are manual, see the report at the end):
#   - map E to IA_RPG_Interact in IMC_RPG_Default
#   - wire the EnhancedInputAction node to Interactor -> TryInteract
#
# Re-running is safe: every step checks for its own result first.
# Writes Saved/CC_Probe/step3_wire.{json,log}.

import json
import os
import traceback

import unreal

OUT_DIR = r"E:/UE5 Projects/Character_Creator/Saved/CC_Probe"
JSON_PATH = os.path.join(OUT_DIR, "step3_wire.json")
LOG_PATH = os.path.join(OUT_DIR, "step3_wire.log")

PLAYER_BP = "/Game/RPG/Blueprints/BP_RPG_PlayerCharacter"
INPUT_DIR = "/Game/RPG/Input"
IA_NAME = "IA_RPG_Interact"
TEST_MAP = "Lvl_RPG_Test"

# item, count, offset from the spawn anchor
TEST_ITEMS = [
    ("/Game/RPG/Items/Assets/DA_Item_OHS03_Sword", 1, unreal.Vector(300.0, 0.0, 50.0)),
    ("/Game/RPG/Items/Assets/DA_Item_Shield04", 1, unreal.Vector(300.0, 150.0, 50.0)),
    ("/Game/RPG/Items/Assets/DA_Item_Bow01", 1, unreal.Vector(300.0, 300.0, 50.0)),
    ("/Game/RPG/Items/Assets/DA_Item_Mat1", 5, unreal.Vector(300.0, 450.0, 50.0)),
]

os.makedirs(OUT_DIR, exist_ok=True)
if os.path.exists(LOG_PATH):
    os.remove(LOG_PATH)


def log(msg):
    with open(LOG_PATH, "a", encoding="utf-8") as f:
        f.write(str(msg) + "\n")


R = {"classes": {}, "components": [], "input_action": None, "spawned": [],
     "manual_steps": [], "errors": [], "notes": []}


def fail(where, detail):
    R["errors"].append("{}: {}".format(where, detail))
    log("  !! {}: {}".format(where, detail))


EAL = unreal.EditorAssetLibrary

# --------------------------------------------------------------------------- #
# 1. did the new module load?
# --------------------------------------------------------------------------- #
log("=== cc_step3_wire start ===")

WANTED = ["InventoryComponent", "InteractorComponent", "InteractableComponent",
          "WorldItem", "RPGInteractable", "ItemData"]
R["classes"] = {name: hasattr(unreal, name) for name in WANTED}
log("classes: " + json.dumps(R["classes"]))

missing = [k for k, v in R["classes"].items() if not v]
if missing:
    R["aborted"] = ("C++ types missing from the Python bindings: {}. The editor is running an "
                    "older DLL - close it, rebuild, reopen.".format(missing))
    log(R["aborted"])
    with open(JSON_PATH, "w", encoding="utf-8") as f:
        json.dump(R, f, indent=1, default=str)
    raise SystemExit(R["aborted"])

# --------------------------------------------------------------------------- #
# 2. components onto the player Blueprint
# --------------------------------------------------------------------------- #
def add_component(blueprint, component_class, desired_name):
    """Idempotent. Returns (status, detail)."""
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(blueprint)
    if not handles:
        return ("error", "no subobject handles returned")

    # Already there?
    for handle in handles:
        data = subsystem.k2_find_subobject_data_from_handle(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if obj and obj.get_class().get_name() == component_class.get_name():
            return ("already_present", obj.get_name())

    params = unreal.AddNewSubobjectParams(
        parent_handle=handles[0],
        new_class=component_class,
        blueprint_context=blueprint)

    new_handle, fail_reason = subsystem.add_new_subobject(params)
    reason = str(fail_reason)
    if reason:
        return ("error", reason)

    try:
        subsystem.rename_subobject(handle=new_handle, new_name=desired_name)
    except Exception as rename_error:
        # Cosmetic only - the component exists either way.
        log("  (rename skipped: {})".format(rename_error))

    return ("added", desired_name)


log("player blueprint components")
try:
    player_bp = EAL.load_asset(PLAYER_BP)
    if not player_bp:
        fail("player bp", "could not load " + PLAYER_BP)
    else:
        for cls, name in ((unreal.InventoryComponent, "Inventory"),
                          (unreal.InteractorComponent, "Interactor")):
            status, detail = add_component(player_bp, cls, name)
            R["components"].append({"class": cls.get_name(), "status": status, "detail": detail})
            log("  {} -> {} ({})".format(cls.get_name(), status, detail))

        if any(c["status"] == "added" for c in R["components"]):
            unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
            EAL.save_loaded_asset(player_bp)
            log("  compiled + saved player BP")
except Exception:
    fail("components", traceback.format_exc())

# --------------------------------------------------------------------------- #
# 3. the Input Action asset
# --------------------------------------------------------------------------- #
log("input action")
ia_path = "{}/{}".format(INPUT_DIR, IA_NAME)
try:
    if EAL.does_asset_exist(ia_path):
        R["input_action"] = {"status": "already_present", "path": ia_path}
    else:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        factory = None
        for factory_name in ("InputActionFactory", "InputActionFactoryNew"):
            factory_class = getattr(unreal, factory_name, None)
            if factory_class is not None:
                factory = factory_class()
                break

        asset = tools.create_asset(IA_NAME, INPUT_DIR, unreal.InputAction, factory)
        if asset is None:
            R["input_action"] = {"status": "failed",
                                 "detail": "create_asset returned None - make it by hand"}
        else:
            try:
                asset.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
            except Exception as value_error:
                log("  (value_type not set: {})".format(value_error))
            EAL.save_loaded_asset(asset)
            R["input_action"] = {"status": "created", "path": ia_path}
    log("  " + json.dumps(R["input_action"]))
except Exception:
    fail("input action", traceback.format_exc())
    R["input_action"] = {"status": "failed", "detail": "see errors"}

# --------------------------------------------------------------------------- #
# 4. test pickups, only in the test map
# --------------------------------------------------------------------------- #
log("test items")
try:
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.EditorLevelLibrary.get_editor_world()
    level_name = world.get_name() if world else "<none>"
    R["notes"].append("open level: " + level_name)

    if TEST_MAP not in level_name:
        R["notes"].append(
            "Skipped spawning test pickups - open {} and re-run to place them.".format(TEST_MAP))
        log("  skipped, level is " + level_name)
    else:
        # Anchor on the PlayerStart so the items land where the player actually begins.
        anchor = unreal.Vector(0.0, 0.0, 100.0)
        for actor in actor_subsystem.get_all_level_actors():
            if actor and "PlayerStart" in actor.get_class().get_name():
                anchor = actor.get_actor_location()
                R["notes"].append("anchored on " + actor.get_name())
                break

        existing = [a for a in actor_subsystem.get_all_level_actors()
                    if a and a.get_class().get_name().startswith("WorldItem")]
        if existing:
            R["notes"].append(
                "{} AWorldItem(s) already placed - skipped spawning more.".format(len(existing)))
            log("  {} already placed".format(len(existing)))
        else:
            for item_path, count, offset in TEST_ITEMS:
                item = EAL.load_asset(item_path)
                if not item:
                    fail("spawn", "missing " + item_path)
                    continue

                location = unreal.Vector(anchor.x + offset.x, anchor.y + offset.y,
                                         anchor.z + offset.z)
                spawned = actor_subsystem.spawn_actor_from_class(
                    unreal.WorldItem, location, unreal.Rotator(0.0, 0.0, 0.0))
                if not spawned:
                    fail("spawn", "spawn_actor_from_class returned None for " + item_path)
                    continue

                spawned.set_editor_property("item", item)
                spawned.set_editor_property("count", count)
                spawned.set_actor_label("WorldItem_" + item.get_name().replace("DA_Item_", ""))
                R["spawned"].append({"item": item_path, "count": count,
                                     "location": [location.x, location.y, location.z]})
                log("  spawned " + item_path)

            if R["spawned"]:
                unreal.EditorLevelLibrary.save_current_level()
                log("  level saved")
except Exception:
    fail("test items", traceback.format_exc())

# --------------------------------------------------------------------------- #
# 5. what is left for a human
# --------------------------------------------------------------------------- #
R["manual_steps"] = [
    "IMC_RPG_Default: add a mapping for IA_RPG_Interact bound to the E key. "
    "The mapping API is not Python-exposed.",
    "BP_RPG_PlayerCharacter EventGraph: add an EnhancedInputAction IA_RPG_Interact node, "
    "drag Triggered -> Get Interactor -> Try Interact. The InputAction pin on that node "
    "cannot be set from script, which is why this stays manual.",
    "SAVE the IMC after editing - unsaved IMC bindings have caused phantom regressions here "
    "before (see the dodge-system notes).",
]

log("=== done: {} errors ===".format(len(R["errors"])))
with open(JSON_PATH, "w", encoding="utf-8") as f:
    json.dump(R, f, indent=1, default=str)

msg = "cc_step3_wire: components={} input_action={} spawned={} errors={} -> {}".format(
    [c["status"] for c in R["components"]],
    (R["input_action"] or {}).get("status"),
    len(R["spawned"]), len(R["errors"]), JSON_PATH)
unreal.log(msg)
print(msg)
