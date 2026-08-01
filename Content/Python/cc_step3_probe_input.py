# cc_step3_probe_input.py — is IA_RPG_Interact actually a well-formed Input Action?
#
#     py "E:/UE5 Projects/Character_Creator/Content/Python/cc_step3_probe_input.py"
#
# Read-only. It creates and changes nothing.
#
# Why: IA_RPG_Interact was created from script with a possibly-null factory. If that produced
# an asset that differs from a known-good one, the E press would never reach the graph no
# matter how the nodes are wired. IA_RPG_Dodge is the control - it demonstrably works in PIE.
#
# Writes Saved/CC_Probe/step3_input.{json,log}.

import json
import os
import traceback

import unreal

OUT_DIR = r"E:/UE5 Projects/Character_Creator/Saved/CC_Probe"
JSON_PATH = os.path.join(OUT_DIR, "step3_input.json")
LOG_PATH = os.path.join(OUT_DIR, "step3_input.log")

SUSPECT = "/Game/RPG/Input/IA_RPG_Interact"
CONTROLS = ["/Game/RPG/Input/IA_RPG_Dodge",
            "/Game/RPG/Input/IA_RPG_TargetLock",
            "/Game/RPG/Input/IA_RPG_SwitchStance"]
IMC = "/Game/RPG/Input/IMC_RPG_Default"

os.makedirs(OUT_DIR, exist_ok=True)
if os.path.exists(LOG_PATH):
    os.remove(LOG_PATH)


def log(msg):
    with open(LOG_PATH, "a", encoding="utf-8") as f:
        f.write(str(msg) + "\n")


R = {"actions": {}, "imc": {}, "diff": [], "errors": []}
EAL = unreal.EditorAssetLibrary

# Every UInputAction property worth comparing.
FIELDS = ["value_type", "trigger_when_paused", "consume_input",
          "reserve_all_mappings", "triggers", "modifiers",
          "accumulation_behavior", "consumes_action_and_axis_mappings"]


def coerce(value):
    if value is None or isinstance(value, (bool, int, float, str)):
        return value
    try:
        if isinstance(value, unreal.EnumBase):
            return "{}={}".format(type(value).__name__, int(value.value))
    except Exception:
        pass
    if isinstance(value, (unreal.Array, list, tuple)):
        return [coerce(v) for v in value]
    if isinstance(value, unreal.Object):
        return value.get_class().get_name()
    return str(value)


def dump_action(path):
    record = {"exists": EAL.does_asset_exist(path)}
    if not record["exists"]:
        return record

    asset = EAL.load_asset(path)
    if asset is None:
        record["load_failed"] = True
        return record

    record["class"] = asset.get_class().get_name()
    record["fields"] = {}
    for field in FIELDS:
        try:
            record["fields"][field] = coerce(asset.get_editor_property(field))
        except Exception:
            record["fields"][field] = "<no such property>"
    return record


log("=== cc_step3_probe_input start ===")

try:
    R["actions"][SUSPECT] = dump_action(SUSPECT)
    log("suspect: " + json.dumps(R["actions"][SUSPECT]))

    for control in CONTROLS:
        R["actions"][control] = dump_action(control)
        log("control: " + control)

    # Compare the suspect against the first control that loaded cleanly.
    suspect = R["actions"][SUSPECT]
    for control in CONTROLS:
        other = R["actions"][control]
        if not other.get("fields"):
            continue
        if suspect.get("class") != other.get("class"):
            R["diff"].append({"field": "<class>", "suspect": suspect.get("class"),
                              "control": other.get("class"), "control_asset": control})
        for field, control_value in other["fields"].items():
            suspect_value = suspect.get("fields", {}).get(field)
            if suspect_value != control_value:
                R["diff"].append({"field": field, "suspect": suspect_value,
                                  "control": control_value, "control_asset": control})
        break
    log("diff: " + json.dumps(R["diff"]))
except Exception:
    R["errors"].append(traceback.format_exc())
    log("  !! " + traceback.format_exc())

# --------------------------------------------------------------------------- #
# IMC — try several routes, because the plain property read has lied before.
# --------------------------------------------------------------------------- #
try:
    imc_asset = EAL.load_asset(IMC)
    R["imc"]["loaded"] = imc_asset is not None
    if imc_asset:
        R["imc"]["class"] = imc_asset.get_class().get_name()

        for prop in ("mappings", "Mappings"):
            try:
                mappings = imc_asset.get_editor_property(prop)
                entries = []
                for mapping in mappings:
                    action = mapping.get_editor_property("action")
                    key = mapping.get_editor_property("key")
                    entries.append({"action": action.get_name() if action else None,
                                    "key": str(key)})
                R["imc"][prop] = {"count": len(entries), "entries": entries}
            except Exception as prop_error:
                R["imc"][prop] = "<{}>".format(prop_error)

        # Independent route: ask the asset registry what this package references. If the IMC
        # references IA_RPG_Interact at all, the mapping was saved - regardless of what the
        # property read claims.
        try:
            registry = unreal.AssetRegistryHelpers.get_asset_registry()
            deps = registry.get_dependencies(
                "/Game/RPG/Input/IMC_RPG_Default",
                unreal.AssetRegistryDependencyOptions(include_hard_package_references=True))
            R["imc"]["package_dependencies"] = sorted(str(d) for d in deps)
            R["imc"]["references_interact"] = any(
                "IA_RPG_Interact" in str(d) for d in deps)
        except Exception as dep_error:
            R["imc"]["package_dependencies"] = "<{}>".format(dep_error)

    log("imc: " + json.dumps(R["imc"]))
except Exception:
    R["errors"].append(traceback.format_exc())
    log("  !! " + traceback.format_exc())

with open(JSON_PATH, "w", encoding="utf-8") as f:
    json.dump(R, f, indent=1, default=str)

msg = "cc_step3_probe_input: diffs={} imc_refs_interact={} errors={} -> {}".format(
    len(R["diff"]), R["imc"].get("references_interact"), len(R["errors"]), JSON_PATH)
unreal.log(msg)
print(msg)
