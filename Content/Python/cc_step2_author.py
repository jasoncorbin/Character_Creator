# cc_step2_author.py — regenerate the item catalogue + rarity palette against the C++ classes.
#
# Run from the UE editor:  Output Log -> Cmd box -> paste:
#     py "E:/UE5 Projects/Character_Creator/Content/Python/cc_step2_author.py"
#
# CLOSE every asset editor tab first (item DataAssets, BPC_RPG_Inventory, PDA_RPG_Item,
# the E_RPG_* enums). An open tab on a deleted asset is how editors die.
#
# Order, deliberately:
#   1. build all 10 new assets into /Game/RPG/_Staging  (nothing existing is touched)
#   2. read every one of them back and verify field-by-field
#   3. ONLY if 10/10 verify -> delete the superseded Blueprint assets, refusing any
#      asset that still has a referencer
#   4. move the new assets onto the real paths, drop the staging folder
#
# A failure in 1 or 2 aborts before anything is deleted.
#
# Writes Saved/CC_Probe/step2_author.{json,log}. The log flushes per line.

import json
import os
import traceback

import unreal

OUT_DIR = r"E:/UE5 Projects/Character_Creator/Saved/CC_Probe"
JSON_PATH = os.path.join(OUT_DIR, "step2_author.json")
LOG_PATH = os.path.join(OUT_DIR, "step2_author.log")

STAGE_ITEMS = "/Game/RPG/_Staging/Items"
STAGE_DATA = "/Game/RPG/_Staging/Data"
FINAL_ITEMS = "/Game/RPG/Items/Assets"
FINAL_DATA = "/Game/RPG/Data"

os.makedirs(OUT_DIR, exist_ok=True)
if os.path.exists(LOG_PATH):
    os.remove(LOG_PATH)


def log(msg):
    with open(LOG_PATH, "a", encoding="utf-8") as f:
        f.write(str(msg) + "\n")


R = {"created": [], "verified": [], "deleted": [], "moved": [], "errors": [], "aborted": None}


def fail(where, detail):
    R["errors"].append("{}: {}".format(where, detail))
    log("  !! {}: {}".format(where, detail))


# --------------------------------------------------------------------------- #
# the catalogue — values taken from the step-1 Blueprint assets via cc_step2_dump.json,
# cross-checked against the live BP_RPG_PlayerCharacter CDO stance arrays.
#
# ONE DELIBERATE CHANGE vs step 1: Bow01's yaw 170 moves from AttachRotationOffHand to
# AttachRotation. It came out of StanceLeftRotations[7], which is why it was filed
# off-hand, but the bow's slot is Ranged - so GetAttachRotationForSlot(Ranged) returns
# AttachRotation and the 170 would never have been read. ItemData.h states AttachRotation
# is the field used "when mounted in the RIGHT hand (or on the bow rig)".
# --------------------------------------------------------------------------- #
E = unreal

ITEMS = [
    dict(asset="DA_Item_OHS03_Sword", id="ohs03_sword", name="Iron Sword",
         desc="A dependable one-handed blade.",
         kind="GEAR", slot="MELEE", cat="OHS", rarity="COMMON", dmg=15, mount="RIGHT_HAND",
         sm="/Game/RPGTinyHeroWavePBR/Mesh/Weapon/OHS03_Sword_SM.OHS03_Sword_SM", sk=None,
         rot=(0.0, 0.0, 0.0), rot_off=(0.0, -180.0, -90.0)),

    dict(asset="DA_Item_THS01_Sword", id="ths01_sword", name="Greatsword",
         desc="Heavy two-handed steel. Slow, but it lands.",
         kind="GEAR", slot="MELEE", cat="THS", rarity="COMMON", dmg=25, mount="RIGHT_HAND",
         sm="/Game/RPGTinyHeroWavePBR/Mesh/Weapon/THS01_Sword_SM.THS01_Sword_SM", sk=None,
         rot=(0.0, 0.0, 0.0), rot_off=(0.0, 0.0, 0.0)),

    dict(asset="DA_Item_Spear01", id="spear01", name="Iron Spear",
         desc="Reach beats speed.",
         kind="GEAR", slot="MELEE", cat="SPEAR", rarity="UNCOMMON", dmg=20, mount="RIGHT_HAND",
         sm="/Game/RPGTinyHeroWavePBR/Mesh/Weapon/Spear01_SM.Spear01_SM", sk=None,
         rot=(0.0, 0.0, 10.0), rot_off=(0.0, 0.0, 0.0)),

    dict(asset="DA_Item_Shield04", id="shield04", name="Kite Shield",
         desc="Battered, but it has held before.",
         kind="GEAR", slot="OFF_HAND", cat="SHIELD", rarity="UNCOMMON", dmg=5, mount="LEFT_HAND",
         sm="/Game/RPGTinyHeroWavePBR/Mesh/Weapon/Shield04_SM.Shield04_SM", sk=None,
         rot=(0.0, 0.0, 0.0), rot_off=(0.0, -180.0, 0.0)),

    dict(asset="DA_Item_Wand01", id="wand01", name="Apprentice Wand",
         desc="Channels a modest spark.",
         kind="GEAR", slot="RANGED", cat="WAND", rarity="RARE", dmg=18, mount="RIGHT_HAND",
         sm="/Game/RPGTinyHeroWavePBR/Mesh/Weapon/Wand01_SM.Wand01_SM", sk=None,
         rot=(0.0, 0.0, 0.0), rot_off=(0.0, 0.0, 0.0)),

    dict(asset="DA_Item_Bow01", id="bow01", name="Hunting Bow",
         desc="Draw, hold, breathe out.",
         kind="GEAR", slot="RANGED", cat="BOW", rarity="RARE", dmg=18, mount="BOW_RIG",
         sm=None, sk="/Game/RPGTinyHeroWavePBR/Mesh/Weapon/Bows/Bow01_SK.Bow01_SK",
         rot=(0.0, 170.0, 0.0), rot_off=(0.0, 0.0, 0.0)),   # <-- the move

    dict(asset="DA_Item_Mat1", id="mat1", name="Mat 1",
         desc="Common crafting material.",
         kind="MATERIAL", slot="MELEE", cat="UNARMED", rarity="COMMON", dmg=0,
         mount="RIGHT_HAND", sm=None, sk=None, rot=(0.0, 0.0, 0.0), rot_off=(0.0, 0.0, 0.0)),

    dict(asset="DA_Item_Mat2", id="mat2", name="Mat 2",
         desc="Uncommon crafting material.",
         kind="MATERIAL", slot="MELEE", cat="UNARMED", rarity="RARE", dmg=0,
         mount="RIGHT_HAND", sm=None, sk=None, rot=(0.0, 0.0, 0.0), rot_off=(0.0, 0.0, 0.0)),

    dict(asset="DA_Item_Mat3", id="mat3", name="Mat 3",
         desc="Rare crafting material. Drops seldom.",
         kind="MATERIAL", slot="MELEE", cat="UNARMED", rarity="LEGENDARY", dmg=0,
         mount="RIGHT_HAND", sm=None, sk=None, rot=(0.0, 0.0, 0.0), rot_off=(0.0, 0.0, 0.0)),
]

# leaf-first. Every one is gated on having no remaining referencer at delete time.
DELETE_ORDER = [
    "/Game/RPG/Blueprints/BPC_RPG_Inventory",
    "/Game/RPG/Items/BP_RPG_ItemInstance",
] + ["/Game/RPG/Items/Assets/" + i["asset"] for i in ITEMS] + [
    "/Game/RPG/Items/PDA_RPG_Item",
    "/Game/RPG/Data/DA_RarityPalette",
    "/Game/RPG/Data/PDA_RPG_RarityPalette",
    "/Game/RPG/Data/E_RPG_ItemRarity",
    "/Game/RPG/Data/E_RPG_ItemKind",
    "/Game/RPG/Data/E_RPG_EquipSlot",
    "/Game/RPG/Data/E_RPG_WeaponCategory",
    "/Game/RPG/Data/E_RPG_MountPoint",
]
# NOT deleted: /Game/RPG/Data/E_RPG_Stance — that is the stance legend, still in use.


# --------------------------------------------------------------------------- #
# helpers
# --------------------------------------------------------------------------- #
EAL = unreal.EditorAssetLibrary


def snake(n):
    s = ""
    for i, ch in enumerate(n):
        if ch.isupper() and i > 0:
            s += "_"
        s += ch.lower()
    return s


def set_prop(obj, name, value):
    """Try snake_case then the literal name. Raises with context on failure."""
    last = None
    for variant in (snake(name), name):
        try:
            obj.set_editor_property(variant, value)
            return
        except Exception as e:
            last = e
    raise RuntimeError("set {} = {!r}: {}".format(name, value, last))


def get_prop(obj, name):
    for variant in (snake(name), name):
        try:
            return obj.get_editor_property(variant)
        except Exception:
            continue
    raise RuntimeError("get " + name)


def make_asset(name, folder, cls):
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.DataAssetFactory()
    try:
        factory.set_editor_property("data_asset_class", cls)
    except Exception as e:
        log("  (data_asset_class not settable: {}) — relying on asset_class".format(e))
    a = tools.create_asset(name, folder, cls, factory)
    if a is None:
        raise RuntimeError("create_asset returned None for {}/{}".format(folder, name))
    return a


def rot(t):
    return unreal.Rotator(pitch=t[0], yaw=t[1], roll=t[2])


def rot_tuple(r):
    return (round(r.pitch, 4), round(r.yaw, 4), round(r.roll, 4))


def soft_target(v):
    """Path string out of whatever a soft-object property hands back."""
    if v is None:
        return None
    for getter in ("get_path_name", "to_string"):
        f = getattr(v, getter, None)
        if callable(f):
            try:
                s = str(f())
                return s or None
            except Exception:
                pass
    s = str(v)
    return s or None


def set_soft(obj, prop, path):
    """Soft-object properties take a loaded object, a SoftObjectPath, or a string."""
    if path is None:
        return
    attempts = []
    loaded = unreal.load_asset(path)
    if loaded is not None:
        attempts.append(loaded)
    try:
        attempts.append(unreal.SoftObjectPath(path))
    except Exception:
        pass
    attempts.append(path)

    last = None
    for v in attempts:
        try:
            set_prop(obj, prop, v)
            return
        except Exception as e:
            last = e
    raise RuntimeError("set {} = {}: {}".format(prop, path, last))


# --------------------------------------------------------------------------- #
# 0. guard
# --------------------------------------------------------------------------- #
log("=== cc_step2_author start ===")
for t in ("ItemData", "RarityPalette", "ItemKind", "EquipSlot", "WeaponCategory",
          "MountPoint", "ItemRarity"):
    if not hasattr(unreal, t):
        R["aborted"] = "C++ type unreal.{} missing — module not loaded. Nothing done.".format(t)
        log(R["aborted"])
        with open(JSON_PATH, "w", encoding="utf-8") as f:
            json.dump(R, f, indent=1, default=str)
        raise SystemExit(R["aborted"])
log("module present")

if EAL.does_directory_exist("/Game/RPG/_Staging"):
    EAL.delete_directory("/Game/RPG/_Staging")
    log("cleared stale staging folder")

# --------------------------------------------------------------------------- #
# 1. build into staging
# --------------------------------------------------------------------------- #
built = {}
try:
    for spec in ITEMS:
        log("build " + spec["asset"])
        a = make_asset(spec["asset"], STAGE_ITEMS, unreal.ItemData)

        set_prop(a, "Id", spec["id"])
        set_prop(a, "DisplayName", spec["name"])
        set_prop(a, "Description", spec["desc"])
        set_prop(a, "Kind", getattr(unreal.ItemKind, spec["kind"]))
        set_prop(a, "Slot", getattr(unreal.EquipSlot, spec["slot"]))
        set_prop(a, "Category", getattr(unreal.WeaponCategory, spec["cat"]))
        set_prop(a, "Rarity", getattr(unreal.ItemRarity, spec["rarity"]))
        set_prop(a, "Damage", spec["dmg"])
        set_prop(a, "RequiredLevel", 0)
        set_prop(a, "MountPoint", getattr(unreal.MountPoint, spec["mount"]))
        set_soft(a, "StaticMeshAsset", spec["sm"])
        set_soft(a, "SkeletalMeshAsset", spec["sk"])
        set_prop(a, "AttachRotation", rot(spec["rot"]))
        set_prop(a, "AttachRotationOffHand", rot(spec["rot_off"]))

        EAL.save_loaded_asset(a)
        built[spec["asset"]] = STAGE_ITEMS + "/" + spec["asset"]
        R["created"].append(built[spec["asset"]])

    log("build DA_RarityPalette")
    p = make_asset("DA_RarityPalette", STAGE_DATA, unreal.RarityPalette)
    # the C++ constructor already ran ResetToDesignDefaults(); call it again so the
    # saved asset carries explicit values rather than relying on class defaults.
    p.reset_to_design_defaults()
    set_prop(p, "ActiveSet", unreal.PaletteSet.CANDY_WARM)
    EAL.save_loaded_asset(p)
    built["DA_RarityPalette"] = STAGE_DATA + "/DA_RarityPalette"
    R["created"].append(built["DA_RarityPalette"])

except Exception as e:
    fail("build", traceback.format_exc())
    R["aborted"] = "build failed — nothing deleted, staging left in /Game/RPG/_Staging"
    log(R["aborted"])
    with open(JSON_PATH, "w", encoding="utf-8") as f:
        json.dump(R, f, indent=1, default=str)
    raise SystemExit(R["aborted"])

# --------------------------------------------------------------------------- #
# 2. verify — read every field back off the saved asset
# --------------------------------------------------------------------------- #
ok = True
for spec in ITEMS:
    path = built[spec["asset"]]
    log("verify " + spec["asset"])
    rec = {"asset": spec["asset"], "mismatches": []}
    try:
        a = EAL.load_asset(path)
        checks = [
            ("Id", str(get_prop(a, "Id")), spec["id"]),
            ("DisplayName", str(get_prop(a, "DisplayName")), spec["name"]),
            ("Description", str(get_prop(a, "Description")), spec["desc"]),
            ("Kind", int(get_prop(a, "Kind").value), int(getattr(unreal.ItemKind, spec["kind"]).value)),
            ("Slot", int(get_prop(a, "Slot").value), int(getattr(unreal.EquipSlot, spec["slot"]).value)),
            ("Category", int(get_prop(a, "Category").value), int(getattr(unreal.WeaponCategory, spec["cat"]).value)),
            ("Rarity", int(get_prop(a, "Rarity").value), int(getattr(unreal.ItemRarity, spec["rarity"]).value)),
            ("Damage", int(get_prop(a, "Damage")), spec["dmg"]),
            ("MountPoint", int(get_prop(a, "MountPoint").value), int(getattr(unreal.MountPoint, spec["mount"]).value)),
            ("AttachRotation", rot_tuple(get_prop(a, "AttachRotation")),
             tuple(round(x, 4) for x in spec["rot"])),
            ("AttachRotationOffHand", rot_tuple(get_prop(a, "AttachRotationOffHand")),
             tuple(round(x, 4) for x in spec["rot_off"])),
        ]
        for field, got, want in checks:
            if got != want:
                rec["mismatches"].append({"field": field, "got": got, "want": want})

        for field, want in (("StaticMeshAsset", spec["sm"]), ("SkeletalMeshAsset", spec["sk"])):
            got = soft_target(get_prop(a, field))
            want_ok = (want is None and not got) or (want is not None and got and want.split(".")[0] in got)
            if not want_ok:
                rec["mismatches"].append({"field": field, "got": got, "want": want})

    except Exception:
        rec["mismatches"].append({"field": "<exception>", "got": traceback.format_exc(), "want": ""})

    if rec["mismatches"]:
        ok = False
        fail("verify " + spec["asset"], rec["mismatches"])
    R["verified"].append(rec)

log("verify DA_RarityPalette")
prec = {"asset": "DA_RarityPalette", "mismatches": []}
try:
    p = EAL.load_asset(built["DA_RarityPalette"])
    cw = get_prop(p, "CandyWarm")
    if len(cw) != 4:
        prec["mismatches"].append({"field": "CandyWarm", "got": len(cw), "want": 4})
    cb = get_prop(p, "ClassicBright")
    if len(cb) != 4:
        prec["mismatches"].append({"field": "ClassicBright", "got": len(cb), "want": 4})
    bc = get_prop(p, "BlockColors")
    if len(bc) != 4:
        prec["mismatches"].append({"field": "BlockColors", "got": len(bc), "want": 4})
    if int(get_prop(p, "ActiveSet").value) != 0:
        prec["mismatches"].append({"field": "ActiveSet", "got": int(get_prop(p, "ActiveSet").value), "want": 0})
    # spot-check one converted hex against the value the Blueprint palette carried
    em = get_prop(p, "EmptyMain")
    if abs(em.r - 0.70838) > 0.002 or abs(em.g - 0.66539) > 0.002 or abs(em.b - 0.58408) > 0.002:
        prec["mismatches"].append({"field": "EmptyMain", "got": (em.r, em.g, em.b),
                                   "want": (0.70838, 0.66539, 0.58408)})
    prec["candy_warm_common"] = str(get_prop(p, "CandyWarm")[unreal.ItemRarity.COMMON])
except Exception:
    prec["mismatches"].append({"field": "<exception>", "got": traceback.format_exc(), "want": ""})
if prec["mismatches"]:
    ok = False
    fail("verify palette", prec["mismatches"])
R["verified"].append(prec)

if not ok:
    R["aborted"] = ("verification failed — NOTHING was deleted. New assets are sitting in "
                    "/Game/RPG/_Staging for inspection. See errors in this file.")
    log(R["aborted"])
    with open(JSON_PATH, "w", encoding="utf-8") as f:
        json.dump(R, f, indent=1, default=str)
    raise SystemExit(R["aborted"])

log("all 10 verified")

# --------------------------------------------------------------------------- #
# 3. delete the superseded Blueprint assets, gated on referencers
# --------------------------------------------------------------------------- #
gone = set()
for path in DELETE_ORDER:
    log("delete " + path)
    try:
        if not EAL.does_asset_exist(path):
            log("  (already absent)")
            continue
        refs = [str(r) for r in EAL.find_package_referencers_for_asset(path, False)]
        remaining = [r for r in refs if r not in gone]
        if remaining:
            fail("delete " + path, "still referenced by " + str(remaining))
            R["aborted"] = ("stopped at {} — it still has referencers. Everything before it was "
                            "deleted; the new assets are still in /Game/RPG/_Staging and have NOT "
                            "been moved.".format(path))
            log(R["aborted"])
            break
        if EAL.delete_asset(path):
            gone.add(path)
            R["deleted"].append(path)
        else:
            fail("delete " + path, "delete_asset returned False")
            R["aborted"] = "delete_asset refused {} — staging untouched.".format(path)
            break
    except Exception:
        fail("delete " + path, traceback.format_exc())
        R["aborted"] = "exception deleting {} — staging untouched.".format(path)
        break

if R["aborted"]:
    with open(JSON_PATH, "w", encoding="utf-8") as f:
        json.dump(R, f, indent=1, default=str)
    raise SystemExit(R["aborted"])

# --------------------------------------------------------------------------- #
# 4. move staging onto the real paths
# --------------------------------------------------------------------------- #
for spec in ITEMS:
    src = built[spec["asset"]]
    dst = FINAL_ITEMS + "/" + spec["asset"]
    log("move " + spec["asset"])
    try:
        if EAL.rename_asset(src, dst):
            R["moved"].append(dst)
        else:
            fail("move " + spec["asset"], "rename_asset returned False")
    except Exception:
        fail("move " + spec["asset"], traceback.format_exc())

log("move DA_RarityPalette")
try:
    dst = FINAL_DATA + "/DA_RarityPalette"
    if EAL.rename_asset(built["DA_RarityPalette"], dst):
        R["moved"].append(dst)
    else:
        fail("move palette", "rename_asset returned False")
except Exception:
    fail("move palette", traceback.format_exc())

try:
    EAL.save_directory(FINAL_ITEMS, only_if_is_dirty=False, recursive=True)
    EAL.save_directory(FINAL_DATA, only_if_is_dirty=False, recursive=True)
except Exception:
    fail("save_directory", traceback.format_exc())

try:
    if EAL.does_directory_exist("/Game/RPG/_Staging"):
        EAL.delete_directory("/Game/RPG/_Staging")
        log("staging folder removed")
except Exception:
    fail("cleanup staging", traceback.format_exc())

# final state
R["final"] = {}
for spec in ITEMS:
    path = FINAL_ITEMS + "/" + spec["asset"]
    R["final"][spec["asset"]] = EAL.does_asset_exist(path)
R["final"]["DA_RarityPalette"] = EAL.does_asset_exist(FINAL_DATA + "/DA_RarityPalette")

log("=== done: {} created, {} deleted, {} moved, {} errors ===".format(
    len(R["created"]), len(R["deleted"]), len(R["moved"]), len(R["errors"])))

with open(JSON_PATH, "w", encoding="utf-8") as f:
    json.dump(R, f, indent=1, default=str)

msg = "cc_step2_author: {} created, {} deleted, {} moved, {} errors -> {}".format(
    len(R["created"]), len(R["deleted"]), len(R["moved"]), len(R["errors"]), JSON_PATH)
unreal.log(msg)
print(msg)
