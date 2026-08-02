// A named loot table asset. Pure data plus the roll, matching UItemData's shape.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Loot/LootTypes.h"
#include "LootTable.generated.h"

/**
 * One asset per enemy archetype (DA_Loot_Grunt, etc).
 *
 * Items are HARD references here, unlike UItemData's soft mesh/icon refs: a loot table is
 * tiny, is only loaded alongside the enemy that owns it, and must resolve synchronously at
 * the moment of death.
 */
UCLASS(BlueprintType)
class CHARACTER_CREATOR_API ULootTable : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TArray<FLootEntry> Entries;

	/**
	 * Rolls every entry independently and returns the ones that hit.
	 * May return an empty array - that is a normal outcome, not a failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	TArray<FLootDrop> RollLoot() const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
