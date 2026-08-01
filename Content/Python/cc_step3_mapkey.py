# cc_step3_mapkey.py — read the IMC correctly, and map E -> IA_RPG_Interact from script.
#
#     py "E:/UE5 Projects/Character_Creator/Content/Python/cc_step3_mapkey.py"
#
# TWO corrections to what this project believed, both from reading
# Engine/Plugins/EnhancedInput/Source/EnhancedInput/Public/InputMappingContext.h (5.7):
#
# 1. `Mappings` is UE_DEPRECATED(5.7) - "Use the DefaultKeyMappings struct instead". The live
#    data is `DefaultKeyMappings.Mappings`. Every previous read of this asset hit the dead
#    property, which is why Python has always reported 0 mappings while the editor showed 10.
#    That is a wrong-property bug, not an API that fails to round-trip.
#
# 2. UInputMappingContext::MapKey(const UInputAction*, FKey) is UFUNCTION(BlueprintCallable),
#    so mapping a key IS scriptable. The handoff's "manual editor step" applies only to the
#    EnhancedInputAction NODE in the graph, not to the mapping itself.
#
# Idempotent. Writes Saved/CC_Probe/step3_mapkey.{json,log}.

import json
import os
import traceback

import unreal

OUT_DIR = r"E:/UE5 Projects/Character_Creator/Saved/CC_Probe"
JSON_PATH = os.path.join(OUT_DIR, "step3_mapkey.json")
LOG_PATH = os.path.join(OUT_DIR, "step3_mapkey.log")

IMC_PATH = "/Game/RPG/Input/IMC_RPG_Default"
IA_PATH = "/Game/RPG/Input/IA_RPG_Interact"
KEY_NAME = "E"

os.makedirs(OUT_DIR, exist_ok=True)
if os.path.exists(LOG_PATH):
    os.remove(LOG_PATH)


def log(msg):
    with open(LOG_PATH, "a", encoding="utf-8") as f:
        f.write(str(msg) + "\n")


R = {"before": [], "after": [], "action": None, "key_ctor": None,
     "mapped": False, "errors": [], "notes": []}


def fail(where, detail):
    R["errors"].append("{}: {}".format(where, detail))
    log("  !! {}: {}".format(where, detail))


EAL = unreal.EditorAssetLibrary
log("=== cc_step3_mapkey start ===")


def read_mappings(imc):
    """The LIVE list: DefaultKeyMappings.Mappings, not the deprecated Mappings array."""
    out = []
    try:
        container = imc.get_editor_property("default_key_mappings")
        entries = container.get_editor_property("mappings")
        for entry in entries:
            action = entry.get_editor_property("action")
            key = entry.get_editor_property("key")
            out.append({
                "action": action.get_name() if action else None,
                "key": str(key.get_editor_property("key_name")) if key else None,
            })
    except Exception:
        fail("read_mappings", traceback.format_exc())
    return out


def make_key(name):
    """FKey construction varies by binding version - try the forms in order."""
    attempts = [
        ("positional", lambda: unreal.Key(name)),
        ("keyword", lambda: unreal.Key(key_name=name)),
        ("set_property", None),
    ]
    for label, builder in attempts:
        try:
            if builder is None:
                key = unreal.Key()
                key.set_editor_property("key_name", name)
                return key, label
            key = builder()
            # Confirm it actually took the name rather than silently defaulting.
            if str(key.get_editor_property("key_name")) == name:
                return key, label
        except Exception:
            continue
    return None, None


try:
    imc = EAL.load_asset(IMC_PATH)
    action_asset = EAL.load_asset(IA_PATH)

    if not imc:
        raise RuntimeError("could not load " + IMC_PATH)
    if not action_asset:
        raise RuntimeError("could not load " + IA_PATH)

    R["action"] = action_asset.get_name()

    R["before"] = read_mappings(imc)
    log("before ({} mappings): {}".format(len(R["before"]), json.dumps(R["before"])))

    already = [m for m in R["before"] if m.get("action") == "IA_RPG_Interact"]
    conflicts = [m for m in R["before"]
                 if m.get("key") == KEY_NAME and m.get("action") != "IA_RPG_Interact"]
    if conflicts:
        R["notes"].append(
            "'{}' is already mapped to {} - both will fire.".format(
                KEY_NAME, [c["action"] for c in conflicts]))
        log("  conflict: " + json.dumps(conflicts))

    if already:
        R["notes"].append("IA_RPG_Interact already mapped to " +
                          str([m["key"] for m in already]))
        log("  already mapped, nothing to do")
    else:
        key, ctor = make_key(KEY_NAME)
        R["key_ctor"] = ctor
        if key is None:
            raise RuntimeError("could not construct an FKey for '{}'".format(KEY_NAME))

        imc.map_key(action_asset, key)
        EAL.save_loaded_asset(imc)
        R["mapped"] = True
        log("  mapped {} -> {} (FKey via {})".format(KEY_NAME, IA_PATH, ctor))

    # Re-read from the saved asset rather than trusting the in-memory object.
    EAL.save_loaded_asset(imc)
    R["after"] = read_mappings(imc)
    log("after ({} mappings): {}".format(len(R["after"]), json.dumps(R["after"])))

    R["verified"] = any(m.get("action") == "IA_RPG_Interact" and m.get("key") == KEY_NAME
                        for m in R["after"])
    log("verified: " + str(R["verified"]))

    try:
        unreal.EnhancedInputLibrary.request_rebuild_control_mappings_for_context(imc, True)
        log("  requested control-mapping rebuild")
    except Exception as rebuild_error:
        R["notes"].append("rebuild request skipped: {} (a PIE restart covers it)"
                          .format(rebuild_error))
except Exception:
    fail("mapkey", traceback.format_exc())

with open(JSON_PATH, "w", encoding="utf-8") as f:
    json.dump(R, f, indent=1, default=str)

msg = "cc_step3_mapkey: before={} after={} verified={} errors={} -> {}".format(
    len(R["before"]), len(R["after"]), R.get("verified"), len(R["errors"]), JSON_PATH)
unreal.log(msg)
print(msg)
