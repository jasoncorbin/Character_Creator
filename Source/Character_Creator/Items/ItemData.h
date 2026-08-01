// One asset per item archetype. Pure data - zero logic lives here (spec pillar 1).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/RPGItemTypes.h"
#include "ItemData.generated.h"

class UStaticMesh;
class USkeletalMesh;
class UTexture2D;

/**
 * An item archetype. Every behaviour lives in a consumer that reads this.
 *
 * Meshes and icons are SOFT references so the catalogue does not drag every mesh
 * into memory - see spec section 2.
 */
UCLASS(BlueprintType, Const)
class CHARACTER_CREATOR_API UItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Stable primary key, snake_case (e.g. "ohs03_sword"). Never change once save data exists. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FName Id;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classification")
	EItemKind Kind = EItemKind::Gear;

	/** Gear only. Ignored when Kind is Material. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classification",
		meta = (EditCondition = "Kind == EItemKind::Gear", EditConditionHides))
	EEquipSlot Slot = EEquipSlot::Melee;

	/** Drives the stance bridge. Switch on this, never on asset name prefixes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classification",
		meta = (EditCondition = "Kind == EItemKind::Gear", EditConditionHides))
	EWeaponCategory Category = EWeaponCategory::Unarmed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classification")
	EItemRarity Rarity = EItemRarity::Common;

	/** Base damage, read by the melee swing at swing time (spec section 6b). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	int32 Damage = 0;

	/** 0 = no requirement. Authored but not enforced - no player-level system exists yet. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	int32 RequiredLevel = 0;

	// --- visuals -------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals",
		meta = (EditCondition = "Kind == EItemKind::Gear", EditConditionHides))
	EMountPoint MountPoint = EMountPoint::RightHand;

	/** Inventory icon. Unset on every asset today - UI falls back to name initials. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Used when MountPoint is RightHand or LeftHand. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TSoftObjectPtr<UStaticMesh> StaticMeshAsset;

	/** Used when MountPoint is BowRig. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TSoftObjectPtr<USkeletalMesh> SkeletalMeshAsset;

	/** Relative rotation when mounted in the RIGHT hand (or on the bow rig). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	FRotator AttachRotation = FRotator::ZeroRotator;

	/**
	 * Relative rotation when mounted in the LEFT hand.
	 * A one-hand sword is valid in either hand (Melee, or OffHand for the DoubleSword
	 * stance) and the off-hand bone is mirrored, so one rotation cannot cover both.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	FRotator AttachRotationOffHand = FRotator::ZeroRotator;

	// --- derived -------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsMaterial() const { return Kind == EItemKind::Material; }

	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsGear() const { return Kind == EItemKind::Gear; }

	/** The attach rotation appropriate to the slot this item is equipped in. */
	UFUNCTION(BlueprintPure, Category = "Item")
	FRotator GetAttachRotationForSlot(EEquipSlot InSlot) const
	{
		return InSlot == EEquipSlot::OffHand ? AttachRotationOffHand : AttachRotation;
	}

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
