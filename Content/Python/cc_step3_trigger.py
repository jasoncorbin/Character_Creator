# cc_step3_trigger.py — make E fire ONCE per press, and clear the duplicate E binding.
#
#     py "E:/UE5 Projects/Character_Creator/Content/Python/cc_step3_trigger.py"
#
# Two separate faults found in the first working PIE run:
#
# 1. IA_RPG_Interact has NO triggers. With none, Enhanced Input's `Triggered` fires on EVERY
#    FRAME the key is held - which is why one press logged ~50 "picked up" lines. Adding
#    UInputTriggerPressed makes Triggered fire exactly once per press. Fixing it on the ACTION
#    rather than switching the graph to the Started pin means every future consumer of this
#    action inherits single-fire semantics; "interact" is discrete by nature.
#
# 2. IMC_RPG_Default has TWO mappings on E: a stray IA_Interact (not ours - IA_RPG_Interact is
#    the one the graph node uses) and ours. Both dispatch. The stray does nothing today because
#    nothing listens to it, but a second listener on E later would fire invisibly.
#    This unmaps it. The IA_Interact ASSET is left alone - deleting it is your call.
#
# Idempotent. Writes Saved/CC_Probe/step3_trigger.{json,log}.

import json
import os
import traceback

import unreal

OUT_DIR = r"E:/UE5 Projects/Character_Creator/Saved/CC_Probe"
JSON_PATH = os.path.join(OUT_DIR, "step3_trigger.json")
LOG_PATH = os.path.join(OUT_DIR, "step3_trigger.log")

IMC_PATH = "/Game/RPG/Input/IMC_RPG_Default"
IA_PATH = "/Game/RPG/Input/IA_RPG_Interact"
STRAY_ACTION = "IA_Interact"
KEY_NAME = "E"

os.makedirs(OUT_DIR, exist_ok=True)
if os.path.exists(LOG_PATH):
    os.remove(LOG_PATH)


def log(msg):
    with open(LOG_PATH, "a", encoding="utf-8") as f:
        f.write(str(msg) + "\n")


R = {"triggers_before": [], "triggers_after": [], "mappings_before": [],
     "mappings_after": [], "unmapped": [], "stray_asset": None, "errors": [], "notes": []}


def fail(where, detail):
    R["errors"].append("{}: {}".format(where, detail))
    log("  !! {}: {}".format(where, detail))


EAL = unreal.EditorAssetLibrary
log("=== cc_step3_trigger start ===")


def read_mappings(imc):
    out = []
    container = imc.get_editor_property("default_key_mappings")
    for entry in container.get_editor_property("mappings"):
        action = entry.get_editor_property("action")
        key = entry.get_editor_property("key")
        out.append({"action": action.get_name() if action else None,
                    "key": str(key.get_editor_property("key_name")) if key else None})
    return out


def trigger_names(action_asset):
    names = []
    for trigger in action_asset.get_editor_property("triggers"):
        names.append(trigger.get_class().get_name() if trigger else "<null>")
    return names


# --------------------------------------------------------------------------- #
# 1. single-fire trigger on the action
# --------------------------------------------------------------------------- #
try:
    action_asset = EAL.load_asset(IA_PATH)
    if not action_asset:
        raise RuntimeError("could not load " + IA_PATH)

    R["triggers_before"] = trigger_names(action_asset)
    log("triggers before: " + json.dumps(R["triggers_before"]))

    if any("Pressed" in name for name in R["triggers_before"]):
        R["notes"].append("A Pressed trigger is already present - left as is.")
    else:
        trigger = unreal.new_object(unreal.InputTriggerPressed, outer=action_asset)
        action_asset.set_editor_property("triggers", [trigger])
        EAL.save_loaded_asset(action_asset)
        log("  added InputTriggerPressed")

    R["triggers_after"] = trigger_names(action_asset)
    log("triggers after: " + json.dumps(R["triggers_after"]))
except Exception:
    fail("trigger", traceback.format_exc())

# --------------------------------------------------------------------------- #
# 2. drop the duplicate E binding
# --------------------------------------------------------------------------- #
try:
    imc = EAL.load_asset(IMC_PATH)
    if not imc:
        raise RuntimeError("could not load " + IMC_PATH)

    R["mappings_before"] = read_mappings(imc)
    log("mappings before ({}): {}".format(len(R["mappings_before"]),
                                           json.dumps(R["mappings_before"])))

    stray = [m for m in R["mappings_before"]
             if m["action"] == STRAY_ACTION and m["key"] == KEY_NAME]

    if not stray:
        R["notes"].append("No stray {} -> {} mapping. Nothing to unmap.".format(
            STRAY_ACTION, KEY_NAME))
    else:
        stray_asset = None
        for candidate in ("/Game/RPG/Input/" + STRAY_ACTION,
                          "/Game/Input/" + STRAY_ACTION):
            if EAL.does_asset_exist(candidate):
                stray_asset = EAL.load_asset(candidate)
                R["stray_asset"] = candidate
                break

        if not stray_asset:
            # Fall back to the instance the mapping itself points at.
            container = imc.get_editor_property("default_key_mappings")
            for entry in container.get_editor_property("mappings"):
                action = entry.get_editor_property("action")
                if action and action.get_name() == STRAY_ACTION:
                    stray_asset = action
                    R["stray_asset"] = action.get_path_name()
                    break

        if stray_asset:
            key = unreal.Key()
            key.set_editor_property("key_name", KEY_NAME)
            imc.unmap_key(stray_asset, key)
            EAL.save_loaded_asset(imc)
            R["unmapped"].append({"action": STRAY_ACTION, "key": KEY_NAME})
            log("  unmapped {} from {}".format(STRAY_ACTION, KEY_NAME))
        else:
            fail("unmap", "could not resolve the " + STRAY_ACTION + " asset")

    R["mappings_after"] = read_mappings(imc)
    log("mappings after ({}): {}".format(len(R["mappings_after"]),
                                         json.dumps(R["mappings_after"])))

    e_bindings = [m["action"] for m in R["mappings_after"] if m["key"] == KEY_NAME]
    R["e_bindings"] = e_bindings
    R["verified"] = (e_bindings == ["IA_RPG_Interact"]
                     and any("Pressed" in n for n in R["triggers_after"]))
    log("E now bound to: {} | verified: {}".format(e_bindings, R["verified"]))
except Exception:
    fail("unmap", traceback.format_exc())

with open(JSON_PATH, "w", encoding="utf-8") as f:
    json.dump(R, f, indent=1, default=str)

msg = "cc_step3_trigger: triggers={} E={} verified={} errors={} -> {}".format(
    R["triggers_after"], R.get("e_bindings"), R.get("verified"), len(R["errors"]), JSON_PATH)
unreal.log(msg)
print(msg)
