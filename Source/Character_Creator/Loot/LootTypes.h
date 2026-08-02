// Loot roll data. Spec section 9, plan step 5.
//
// Naming note: the plan calls these S_RPG_LootEntry / PDA_RPG_LootTable, which is Blueprint
// asset naming. They are C++ types here because user-defined structs cannot be authored or
// populated from Python at all (FStructureEditorUtils is C++-only), so a BP struct would have
// been hand-built and hand-maintained for no benefit.

#pragma once

#include "CoreMinimal.h"
#include "LootTypes.generated.h"

class UItemData;

/**
 * One line of a loot table, rolled INDEPENDENTLY of every other line.
 *
 * That independence is the whole design: a single kill can drop several entries or none.
 * It is not a weighted pick-one.
 */
USTRUCT(BlueprintType)
struct CHARACTER_CREATOR_API FLootEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UItemData> Item = nullptr;

	/** 0 = never, 1 = always. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 1.0f;

	/** Inclusive lower bound on stack size when this entry hits. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MinCount = 1;

	/** Inclusive upper bound. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MaxCount = 1;
};

/** A resolved drop: what to spawn and how many. Output of ULootTable::RollLoot. */
USTRUCT(BlueprintType)
struct CHARACTER_CREATOR_API FLootDrop
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UItemData> Item = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	int32 Count = 0;

	FLootDrop() = default;

	FLootDrop(UItemData* InItem, int32 InCount)
		: Item(InItem), Count(InCount) {}
};
