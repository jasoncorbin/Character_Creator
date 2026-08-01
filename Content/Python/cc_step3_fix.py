# cc_step3_fix.py — adds the two components to BP_RPG_PlayerCharacter and verifies the rest.
#
#     py "E:/UE5 Projects/Character_Creator/Content/Python/cc_step3_fix.py"
#
# Supersedes the component step in cc_step3_wire.py, which threw:
#     TypeError: unbound method _ObjectBase.get_name() needs an argument
# `unreal.InventoryComponent` is a Python TYPE, not a UClass. Its UClass is
# `unreal.InventoryComponent.static_class()`. Use isinstance() for the "already there?" test
# and .static_class() for anything the C++ API wants.
#
# Idempotent. Writes Saved/CC_Probe/step3_fix.{json,log}.

import json
import os
import traceback

import unreal

OUT_DIR = r"E:/UE5 Projects/Character_Creator/Saved/CC_Probe"
JSON_PATH = os.path.join(OUT_DIR, "step3_fix.json")
LOG_PATH = os.path.join(OUT_DIR, "step3_fix.log")

PLAYER_BP = "/Game/RPG/Blueprints/BP_RPG_PlayerCharacter"

os.makedirs(OUT_DIR, exist_ok=True)
if os.path.exists(LOG_PATH):
    os.remove(LOG_PATH)


def log(msg):
    with open(LOG_PATH, "a", encoding="utf-8") as f:
        f.write(str(msg) + "\n")


R = {"subobjects_before": [], "actions": [], "subobjects_after": [],
     "world_items": [], "imc": None, "errors": []}


def fail(where, detail):
    R["errors"].append("{}: {}".format(where, detail))
    log("  !! {}: {}".format(where, detail))


EAL = unreal.EditorAssetLibrary
SUB = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)

log("=== cc_step3_fix start ===")


def list_subobjects(blueprint):
    """[(display name, class name)] for every component on the Blueprint."""
    out = []
    for handle in SUB.k2_gather_subobject_data_for_blueprint(blueprint):
        try:
            data = SUB.k2_find_subobject_data_from_handle(handle)
            obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
            if obj is not None:
                out.append((obj.get_name(), obj.get_class().get_name()))
        except Exception as list_error:
            out.append(("<unreadable>", str(list_error)))
    return out


def add_component(blueprint, component_type, desired_name):
    handles = SUB.k2_gather_subobject_data_for_blueprint(blueprint)
    if not handles:
        return ("error", "no subobject handles")

    for handle in handles:
        data = SUB.k2_find_subobject_data_from_handle(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        # isinstance against the Python type - no UClass juggling, and it also matches
        # a Blueprint-derived subclass, which a name comparison would miss.
        if obj is not None and isinstance(obj, component_type):
            return ("already_present", obj.get_name())

    params = unreal.AddNewSubobjectParams(
        parent_handle=handles[0],
        new_class=component_type.static_class(),
        blueprint_context=blueprint)

    new_handle, fail_reason = SUB.add_new_subobject(params)
    reason = str(fail_reason)
    if reason:
        return ("error", reason)

    try:
        SUB.rename_subobject(new_handle, desired_name)
    except Exception as rename_error:
        log("  (rename skipped, cosmetic: {})".format(rename_error))

    return ("added", desired_name)


# --------------------------------------------------------------------------- #
# 1. components
# --------------------------------------------------------------------------- #
try:
    player_bp = EAL.load_asset(PLAYER_BP)
    if not player_bp:
        raise RuntimeError("could not load " + PLAYER_BP)

    R["subobjects_before"] = list_subobjects(player_bp)
    log("before: " + json.dumps(R["subobjects_before"]))

    changed = False
    for component_type, name in ((unreal.InventoryComponent, "Inventory"),
                                 (unreal.InteractorComponent, "Interactor")):
        status, detail = add_component(player_bp, component_type, name)
        R["actions"].append({"class": component_type.static_class().get_name(),
                             "status": status, "detail": detail})
        log("  {} -> {} ({})".format(name, status, detail))
        changed = changed or (status == "added")

    if changed:
        unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
        EAL.save_loaded_asset(player_bp)
        log("  compiled + saved")

    R["subobjects_after"] = list_subobjects(player_bp)
    log("after: " + json.dumps(R["subobjects_after"]))

    have_inv = any("InventoryComponent" in c for _, c in R["subobjects_after"])
    have_int = any("InteractorComponent" in c for _, c in R["subobjects_after"])
    R["verified"] = {"InventoryComponent": have_inv, "InteractorComponent": have_int}
    log("verified: " + json.dumps(R["verified"]))
except Exception:
    fail("components", traceback.format_exc())

# --------------------------------------------------------------------------- #
# 2. are the placed pickups actually carrying an item?
# --------------------------------------------------------------------------- #
try:
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actor_subsystem.get_all_level_actors():
        if not actor or not isinstance(actor, unreal.WorldItem):
            continue
        item = actor.get_editor_property("item")
        location = actor.get_actor_location()
        R["world_items"].append({
            "label": actor.get_actor_label(),
            "item": item.get_name() if item else None,
            "count": actor.get_editor_property("count"),
            "location": [round(location.x, 1), round(location.y, 1), round(location.z, 1)],
        })
    log("world items: " + json.dumps(R["world_items"]))
except Exception:
    fail("world items", traceback.format_exc())

# --------------------------------------------------------------------------- #
# 3. IMC mappings — read-only, and known unreliable from Python
# --------------------------------------------------------------------------- #
try:
    imc = EAL.load_asset("/Game/RPG/Input/IMC_RPG_Default")
    if imc:
        mappings = imc.get_editor_property("mappings")
        entries = []
        for mapping in mappings:
            try:
                action = mapping.get_editor_property("action")
                key = mapping.get_editor_property("key")
                entries.append({"action": action.get_name() if action else None,
                                "key": str(key)})
            except Exception:
                continue
        R["imc"] = {"count": len(entries), "entries": entries,
                    "caveat": "Python has previously reported 0 mappings for this asset even "
                              "when 10 exist - trust MCP input_ops get_bindings, or the editor."}
    log("imc: " + json.dumps(R["imc"]))
except Exception:
    fail("imc", traceback.format_exc())

log("=== done: {} errors ===".format(len(R["errors"])))
with open(JSON_PATH, "w", encoding="utf-8") as f:
    json.dump(R, f, indent=1, default=str)

msg = "cc_step3_fix: {} | worlditems={} | errors={} -> {}".format(
    R.get("verified"), len(R["world_items"]), len(R["errors"]), JSON_PATH)
unreal.log(msg)
print(msg)
