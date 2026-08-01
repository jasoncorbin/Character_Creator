// A runtime, per-copy instance of a gear item.
//
// Front-loaded deliberately (spec section 8 warns the instance model "ripples" through the
// inventory list and every UI read of Damage/Rarity). Level is 0 and the accessors simply
// forward to the template until the forge lands; at that point ONLY this file changes.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Items/RPGItemTypes.h"
#include "ItemInstance.generated.h"

class UItemData;

UCLASS(BlueprintType)
class CHARACTER_CREATOR_API UItemInstance : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Instance")
	TObjectPtr<UItemData> Template = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Instance", meta = (ClampMin = "0"))
	int32 Level = 0;

	/** Set from the template at creation. Diverges from it once upgrades land. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Instance")
	int32 BaseDamage = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Instance")
	int32 DamagePerLevel = 0;

	/** Factory. Outer should be the inventory component so lifetime follows the owner. */
	static UItemInstance* Create(UObject* Outer, UItemData* InTemplate);

	// --- accessors: every consumer reads through these, never the template directly ---

	UFUNCTION(BlueprintPure, Category = "Instance")
	int32 GetDamage() const;

	UFUNCTION(BlueprintPure, Category = "Instance")
	EItemRarity GetRarity() const;

	UFUNCTION(BlueprintPure, Category = "Instance")
	FText GetDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "Instance")
	EEquipSlot GetSlot() const;

	UFUNCTION(BlueprintPure, Category = "Instance")
	EWeaponCategory GetCategory() const;

	UFUNCTION(BlueprintPure, Category = "Instance")
	bool IsValidInstance() const { return Template != nullptr; }
};
