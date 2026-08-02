#include "Loot/LootTable.h"

#include "Items/ItemData.h"

TArray<FLootDrop> ULootTable::RollLoot() const
{
	TArray<FLootDrop> Drops;
	Drops.Reserve(Entries.Num());

	for (const FLootEntry& Entry : Entries)
	{
		if (!Entry.Item || Entry.DropChance <= 0.0f)
		{
			continue;
		}

		// FRand() is [0,1), so DropChance 1.0 always passes and 0.0 never does.
		if (FMath::FRand() > Entry.DropChance)
		{
			continue;
		}

		// Authoring guard: a table with Max < Min would otherwise invert the range silently.
		const int32 Min = FMath::Max(1, Entry.MinCount);
		const int32 Max = FMath::Max(Min, Entry.MaxCount);

		Drops.Emplace(Entry.Item, FMath::RandRange(Min, Max)); // RandRange is inclusive both ends
	}

	return Drops;
}

FPrimaryAssetId ULootTable::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("LootTable"), GetFName());
}
