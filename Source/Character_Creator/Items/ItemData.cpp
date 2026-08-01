#include "Items/ItemData.h"

FPrimaryAssetId UItemData::GetPrimaryAssetId() const
{
	// Stable id even if the asset is renamed on disk, provided Id is authored.
	const FName AssetName = Id.IsNone() ? GetFName() : Id;
	return FPrimaryAssetId(TEXT("ItemData"), AssetName);
}
