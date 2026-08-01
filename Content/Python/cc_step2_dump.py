# cc_step2_dump.py — READ-ONLY probe. Creates nothing, deletes nothing, saves nothing.
#
# Run from the UE editor:  Output Log -> Cmd box -> mode "Python" -> paste:
#     py "E:/UE5 Projects/Character_Creator/Content/Python/cc_step2_dump.py"
#
# Writes:
#     E:/UE5 Projects/Character_Creator/Saved/CC_Probe/step2_dump.json   (the data)
#     E:/UE5 Projects/Character_Creator/Saved/CC_Probe/step2_dump.log    (per-line progress)
#
# The log is opened/closed per write, so if the editor dies the last surviving line
# pinpoints where.

import json
import os
import traceback

import unreal

OUT_DIR = r"E:/UE5 Projects/Character_Creator/Saved/CC_Probe"
JSON_PATH = os.path.join(OUT_DIR, "step2_dump.json")
LOG_PATH = os.path.join(OUT_DIR, "step2_dump.log")

os.makedirs(OUT_DIR, exist_ok=True)
if os.path.exists(LOG_PATH):
    os.remove(LOG_PATH)


def log(msg):
    with open(LOG_PATH, "a", encoding="utf-8") as f:
        f.write(str(msg) + "\n")


D = {"errors": []}


def err(where, e):
    msg = "{}: {}".format(where, e)
    D["errors"].append(msg)
    log("  !! " + msg)


# --------------------------------------------------------------------------- #
# value coercion — turn any UE property value into something JSON can hold
# --------------------------------------------------------------------------- #
def coerce(v):
    if v is None:
        return None
    if isinstance(v, (bool, int, float, str)):
        return v

    # enums (C++ or user-defined) --------------------------------------------
    try:
        if isinstance(v, unreal.EnumBase):
            return {"__enum__": type(v).__name__, "value": int(v.value), "name": str(v)}
    except Exception:
        pass

    # structs ----------------------------------------------------------------
    if isinstance(v, unreal.Rotator):
        return {"__rotator__": True, "pitch": v.pitch, "yaw": v.yaw, "roll": v.roll}
    if isinstance(v, unreal.Vector):
        return {"__vector__": True, "x": v.x, "y": v.y, "z": v.z}
    if isinstance(v, unreal.LinearColor):
        return {"__color__": True, "r": v.r, "g": v.g, "b": v.b, "a": v.a}
    if isinstance(v, unreal.Text):
        return {"__text__": str(v)}
    if isinstance(v, unreal.Name):
        return {"__name__": str(v)}

    # soft refs ---------------------------------------------------------------
    for attr in ("SoftObjectPath", "SoftClassPath"):
        cls = getattr(unreal, attr, None)
        if cls is not None and isinstance(v, cls):
            return {"__soft__": str(v)}
    if type(v).__name__ in ("SoftObjectPath", "SoftClassPath"):
        return {"__soft__": str(v)}

    # containers --------------------------------------------------------------
    if isinstance(v, (unreal.Array, list, tuple)):
        return [coerce(x) for x in v]
    if isinstance(v, (unreal.Map, dict)):
        out = []
        try:
            for k in v.keys():
                out.append({"key": coerce(k), "value": coerce(v[k])})
        except Exception as e:
            err("map-iter", e)
        return {"__map__": out}
    if isinstance(v, unreal.Set):
        return [coerce(x) for x in v]

    # objects -----------------------------------------------------------------
    if isinstance(v, unreal.Object):
        return {"__obj__": v.get_path_name(), "class": v.get_class().get_name()}

    return {"__repr__": repr(v), "type": type(v).__name__}


def props(obj, names):
    """Read a fixed list of property names, tolerating snake_case / PascalCase."""
    out = {}
    for n in names:
        got = False
        for variant in (n, n[0].lower() + n[1:], _snake(n)):
            try:
                out[n] = coerce(obj.get_editor_property(variant))
                got = True
                break
            except Exception:
                continue
        if not got:
            out[n] = {"__missing__": True}
    return out


def _snake(n):
    s = ""
    for i, ch in enumerate(n):
        if ch.isupper() and i > 0:
            s += "_"
        s += ch.lower()
    return s


# --------------------------------------------------------------------------- #
# 0. environment
# --------------------------------------------------------------------------- #
log("=== cc_step2_dump start ===")
try:
    D["engine_version"] = unreal.SystemLibrary.get_engine_version()
except Exception as e:
    err("engine_version", e)

CPP_TYPES = ["ItemData", "RarityPalette", "ItemInstance", "InventoryComponent",
             "ItemRarity", "ItemKind", "EquipSlot", "WeaponCategory", "MountPoint",
             "InteractPriority", "RarityColors", "PaletteSet"]
D["cpp_module"] = {t: hasattr(unreal, t) for t in CPP_TYPES}
log("cpp_module loaded? " + json.dumps(D["cpp_module"]))

# enum entry order, as the module actually registered it
D["cpp_enum_entries"] = {}
for ename in ["ItemRarity", "ItemKind", "EquipSlot", "WeaponCategory", "MountPoint",
              "InteractPriority", "PaletteSet"]:
    try:
        et = getattr(unreal, ename, None)
        if et is None:
            continue
        entries = []
        for attr in dir(et):
            if attr.startswith("_"):
                continue
            val = getattr(et, attr)
            try:
                entries.append([attr, int(val.value)])
            except Exception:
                pass
        D["cpp_enum_entries"][ename] = sorted(entries, key=lambda p: p[1])
    except Exception as e:
        err("enum " + ename, e)
log("cpp enum entries captured")

# --------------------------------------------------------------------------- #
# 1. the 9 Blueprint DataAssets
# --------------------------------------------------------------------------- #
ITEM_FIELDS = ["Id", "DisplayName", "Description", "Kind", "Slot", "Category", "Rarity",
               "Damage", "RequiredLevel", "MountPoint", "Icon", "StaticMeshAsset",
               "SkeletalMeshAsset", "AttachRotation", "AttachRotationOffHand"]

ITEM_ASSETS = ["DA_Item_OHS03_Sword", "DA_Item_THS01_Sword", "DA_Item_Spear01",
               "DA_Item_Shield04", "DA_Item_Wand01", "DA_Item_Bow01",
               "DA_Item_Mat1", "DA_Item_Mat2", "DA_Item_Mat3"]

D["items"] = {}
for name in ITEM_ASSETS:
    path = "/Game/RPG/Items/Assets/" + name
    log("item " + name)
    try:
        a = unreal.EditorAssetLibrary.load_asset(path)
        if a is None:
            D["items"][name] = {"__load_failed__": path}
            continue
        rec = props(a, ITEM_FIELDS)
        rec["__class__"] = a.get_class().get_name()
        D["items"][name] = rec
    except Exception as e:
        err("item " + name, e)
        D["items"][name] = {"__error__": traceback.format_exc()}

# --------------------------------------------------------------------------- #
# 2. the rarity palette instance
# --------------------------------------------------------------------------- #
PALETTE_FIELDS = ["ActiveSet", "PaletteSet", "CandyWarm", "ClassicBright",
                  "CandyWarmColors", "ClassicBrightColors",
                  "EmptyMain", "EmptySoft", "LockedMain", "LockedSoft",
                  "BlockColors"]
log("palette")
try:
    p = unreal.EditorAssetLibrary.load_asset("/Game/RPG/Data/DA_RarityPalette")
    if p is None:
        D["palette"] = {"__load_failed__": True}
    else:
        rec = props(p, PALETTE_FIELDS)
        rec["__class__"] = p.get_class().get_name()
        D["palette"] = rec
except Exception as e:
    err("palette", e)
    D["palette"] = {"__error__": traceback.format_exc()}

# --------------------------------------------------------------------------- #
# 3. the live player CDO — authoritative mesh + rotation source
# --------------------------------------------------------------------------- #
CDO_FIELDS = ["StanceRightMeshes", "StanceLeftMeshes", "StanceRightRotations",
              "StanceLeftRotations", "StanceIsRanged", "StanceComboLight",
              "StanceComboHeavy", "CurrentStance"]
log("player CDO")
try:
    bp_cls = unreal.EditorAssetLibrary.load_blueprint_class(
        "/Game/RPG/Blueprints/BP_RPG_PlayerCharacter")
    cdo = unreal.get_default_object(bp_cls)
    D["player_cdo"] = props(cdo, CDO_FIELDS)
except Exception as e:
    err("player_cdo", e)
    D["player_cdo"] = {"__error__": traceback.format_exc()}

# also grab the bow component's default skeletal mesh, since bows live on their own component
log("player components")
try:
    D["player_components"] = []
    sub = unreal.SubobjectDataSubsystem
    # cheap path: just record the Bow component's mesh off the CDO if reachable
    for cname in ("Bow", "Weapon_R", "Weapon_L"):
        try:
            comp = cdo.get_editor_property(cname)
            D["player_components"].append({"name": cname, "value": coerce(comp)})
        except Exception:
            D["player_components"].append({"name": cname, "value": "__not a CDO property__"})
except Exception as e:
    err("player_components", e)

# --------------------------------------------------------------------------- #
# 4. what still references the assets we plan to delete
# --------------------------------------------------------------------------- #
DELETE_CANDIDATES = [
    "/Game/RPG/Items/PDA_RPG_Item",
    "/Game/RPG/Items/BP_RPG_ItemInstance",
    "/Game/RPG/Blueprints/BPC_RPG_Inventory",
    "/Game/RPG/Data/PDA_RPG_RarityPalette",
    "/Game/RPG/Data/DA_RarityPalette",
    "/Game/RPG/Data/E_RPG_ItemRarity",
    "/Game/RPG/Data/E_RPG_ItemKind",
    "/Game/RPG/Data/E_RPG_EquipSlot",
    "/Game/RPG/Data/E_RPG_WeaponCategory",
    "/Game/RPG/Data/E_RPG_MountPoint",
] + ["/Game/RPG/Items/Assets/" + n for n in ITEM_ASSETS]

D["referencers"] = {}
for path in DELETE_CANDIDATES:
    log("refs " + path)
    try:
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            D["referencers"][path] = "__missing__"
            continue
        refs = unreal.EditorAssetLibrary.find_package_referencers_for_asset(path, False)
        D["referencers"][path] = [str(r) for r in refs]
    except Exception as e:
        err("refs " + path, e)
        D["referencers"][path] = {"__error__": str(e)}

# --------------------------------------------------------------------------- #
# 5. does the player already have an inventory component attached?
# --------------------------------------------------------------------------- #
log("inventory component usage")
try:
    D["player_has_inventory_component"] = "BPC_RPG_Inventory" in json.dumps(
        D["referencers"].get("/Game/RPG/Blueprints/BPC_RPG_Inventory", []))
    D["bpc_inventory_referencers"] = D["referencers"].get(
        "/Game/RPG/Blueprints/BPC_RPG_Inventory", [])
except Exception as e:
    err("inv usage", e)

# --------------------------------------------------------------------------- #
# 6. weapon mesh library, for cross-checking mesh paths
# --------------------------------------------------------------------------- #
log("weapon mesh library")
try:
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    found = reg.get_assets_by_path("/Game/RPGTinyHeroWavePBR/Mesh/Weapon", True, False)
    D["weapon_meshes"] = sorted(str(a.package_name) for a in found)
except Exception as e:
    err("weapon_meshes", e)

# --------------------------------------------------------------------------- #
log("writing json")
with open(JSON_PATH, "w", encoding="utf-8") as f:
    json.dump(D, f, indent=1, default=str, sort_keys=False)

log("=== done, {} errors ===".format(len(D["errors"])))
unreal.log("cc_step2_dump: wrote {} ({} errors)".format(JSON_PATH, len(D["errors"])))
print("cc_step2_dump: wrote {} ({} errors)".format(JSON_PATH, len(D["errors"])))
