// Shared enums and small structs for the item / loot / inventory layer.
// Mirrors docs/ItemsLootUI_MechanicsSpec_ForUE5.md sections 2 and 3.

#pragma once

#include "CoreMinimal.h"
#include "RPGItemTypes.generated.h"

/** 4-tier rarity ladder. Drives loot value and every UI colour. */
UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common     UMETA(DisplayName = "Common"),
	Uncommon   UMETA(DisplayName = "Uncommon"),
	Rare       UMETA(DisplayName = "Rare"),
	Legendary  UMETA(DisplayName = "Legendary"),

	Count      UMETA(Hidden)
};

/** Gear is unique and equippable; Material is a stackable resource that is never equipped. */
UENUM(BlueprintType)
enum class EItemKind : uint8
{
	Gear      UMETA(DisplayName = "Gear"),
	Material  UMETA(DisplayName = "Material")
};

/** The four wired equipment slots. Head/Chest/Feet/Charm are UI-only placeholders. */
UENUM(BlueprintType)
enum class EEquipSlot : uint8
{
	Melee    UMETA(DisplayName = "Melee"),
	OffHand  UMETA(DisplayName = "Off-Hand"),
	Ranged   UMETA(DisplayName = "Ranged"),
	Armor    UMETA(DisplayName = "Armor"),

	Count    UMETA(Hidden)
};

/**
 * Explicit weapon category. The stance bridge switches on THIS, never on mesh/prefab
 * name prefixes - see spec section 6c.
 */
UENUM(BlueprintType)
enum class EWeaponCategory : uint8
{
	Unarmed  UMETA(DisplayName = "Unarmed"),
	OHS      UMETA(DisplayName = "One-Hand Sword"),
	THS      UMETA(DisplayName = "Two-Hand Sword"),
	Spear    UMETA(DisplayName = "Spear"),
	Shield   UMETA(DisplayName = "Shield"),
	Bow      UMETA(DisplayName = "Bow"),
	Wand     UMETA(DisplayName = "Wand"),
	Arrows   UMETA(DisplayName = "Arrows")
};

/**
 * Where an equipped item's mesh is mounted on the player.
 *
 * Not in the original spec - forced by this project's rig: bows are SkeletalMesh on a
 * dedicated Bow component, while swords/shields/wands are StaticMesh on Weapon_R / Weapon_L.
 * A wand is Ranged but right-handed, so the mount cannot be inferred from the slot alone.
 */
UENUM(BlueprintType)
enum class EMountPoint : uint8
{
	RightHand  UMETA(DisplayName = "Right Hand"),
	LeftHand   UMETA(DisplayName = "Left Hand"),
	BowRig     UMETA(DisplayName = "Bow Rig")
};

/**
 * Priority ladder for the interaction system (spec section 5). Higher wins.
 *
 * The gaps are deliberate - they leave room to slot new interaction kinds between the
 * named tiers without renumbering. None MUST stay at 0: UHT requires a zero entry so a
 * default-initialised value is meaningful, and it doubles as the "nothing here" result
 * for the interactor's arbitration.
 */
UENUM(BlueprintType)
enum class EInteractPriority : uint8
{
	None         = 0   UMETA(DisplayName = "None"),
	Pickup       = 10  UMETA(DisplayName = "Pickup"),
	Open         = 50  UMETA(DisplayName = "Open"),
	Assassinate  = 100 UMETA(DisplayName = "Assassinate")
};

/** The three colours every rarity-tinted widget needs: frame, soft fill, label text. */
USTRUCT(BlueprintType)
struct CHARACTER_CREATOR_API FRarityColors
{
	GENERATED_BODY()

	/** 2px frame / border. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FLinearColor Main = FLinearColor::Gray;

	/** Soft cell background. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FLinearColor Soft = FLinearColor::White;

	/** Rarity label text. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FLinearColor Text = FLinearColor::Black;

	FRarityColors() = default;

	FRarityColors(const FLinearColor& InMain, const FLinearColor& InSoft, const FLinearColor& InText)
		: Main(InMain), Soft(InSoft), Text(InText) {}
};
