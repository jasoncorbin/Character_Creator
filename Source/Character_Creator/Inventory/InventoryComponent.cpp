#include "Inventory/InventoryComponent.h"
#include "Items/ItemData.h"
#include "Items/ItemInstance.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UInventoryComponent::AddItem(UItemData* Item, int32 Count)
{
	if (!Item || Count <= 0)
	{
		return 0;
	}

	if (Item->IsMaterial())
	{
		int32& Stack = Materials.FindOrAdd(Item);
		Stack += Count;
		OnItemAdded.Broadcast(Item);
		return Count;
	}

	// Gear: one instance per copy, stopping at capacity. Report the true count so the
	// caller can decide what to do with the remainder.
	int32 Added = 0;
	for (int32 i = 0; i < Count; ++i)
	{
		if (GearBag.Num() >= Capacity)
		{
			break;
		}

		if (UItemInstance* Instance = UItemInstance::Create(this, Item))
		{
			GearBag.Add(Instance);
			++Added;
			OnItemAdded.Broadcast(Item);
		}
	}

	return Added;
}

bool UInventoryComponent::RemoveItem(UItemInstance* Instance)
{
	if (!Instance)
	{
		return false;
	}

	const int32 Index = GearBag.IndexOfByKey(Instance);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	UItemData* Template = Instance->Template;
	GearBag.RemoveAt(Index);
	OnItemRemoved.Broadcast(Template);
	return true;
}

bool UInventoryComponent::HasItem(UItemData* Item) const
{
	if (!Item)
	{
		return false;
	}

	for (const TObjectPtr<UItemInstance>& Instance : GearBag)
	{
		if (Instance && Instance->Template == Item)
		{
			return true;
		}
	}
	return false;
}

TArray<UItemInstance*> UInventoryComponent::GetGearBag() const
{
	TArray<UItemInstance*> Out;
	Out.Reserve(GearBag.Num());
	for (const TObjectPtr<UItemInstance>& Instance : GearBag)
	{
		Out.Add(Instance);
	}
	return Out;
}

int32 UInventoryComponent::GetMaterialCount(UItemData* Item) const
{
	if (!Item)
	{
		return 0;
	}

	const int32* Found = Materials.Find(Item);
	return Found ? *Found : 0;
}

TMap<UItemData*, int32> UInventoryComponent::GetMaterials() const
{
	TMap<UItemData*, int32> Out;
	Out.Reserve(Materials.Num());
	for (const TPair<TObjectPtr<UItemData>, int32>& Pair : Materials)
	{
		Out.Add(Pair.Key, Pair.Value);
	}
	return Out;
}

bool UInventoryComponent::SpendMaterial(UItemData* Item, int32 Amount)
{
	if (!Item || Amount <= 0)
	{
		return false;
	}

	int32* Stack = Materials.Find(Item);
	if (!Stack || *Stack < Amount)
	{
		return false;
	}

	*Stack -= Amount;
	if (*Stack <= 0)
	{
		Materials.Remove(Item);
	}

	OnItemRemoved.Broadcast(Item);
	return true;
}

bool UInventoryComponent::Equip(UItemInstance* Instance)
{
	if (!Instance || !Instance->Template)
	{
		return false;
	}

	const EEquipSlot Slot = Instance->GetSlot();

	if (Instance->Template->IsMaterial())
	{
		return false; // materials are never equipped
	}

	// Take it out of the bag if it is there (equipping moves, it does not duplicate).
	GearBag.Remove(Instance);

	UItemInstance* Previous = nullptr;
	if (TObjectPtr<UItemInstance>* Existing = Equipped.Find(Slot))
	{
		Previous = *Existing;
	}

	Equipped.Add(Slot, Instance);

	// The fix for Unity's silent drop: the displaced item goes back to the bag.
	if (Previous && Previous != Instance)
	{
		if (GearBag.Num() < Capacity)
		{
			GearBag.Add(Previous);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Inventory] Bag full - displaced item '%s' was discarded on equip."),
				*Previous->GetDisplayName().ToString());
		}
	}

	OnEquipChanged.Broadcast(Slot, Instance, Previous);
	return true;
}

bool UInventoryComponent::Unequip(EEquipSlot Slot)
{
	TObjectPtr<UItemInstance>* Existing = Equipped.Find(Slot);
	if (!Existing || !*Existing)
	{
		return false;
	}

	UItemInstance* Previous = *Existing;
	Equipped.Remove(Slot);

	if (GearBag.Num() < Capacity)
	{
		GearBag.Add(Previous);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Inventory] Bag full - unequipped item '%s' was discarded."),
			*Previous->GetDisplayName().ToString());
	}

	OnEquipChanged.Broadcast(Slot, nullptr, Previous);
	return true;
}

UItemInstance* UInventoryComponent::GetEquipped(EEquipSlot Slot) const
{
	const TObjectPtr<UItemInstance>* Found = Equipped.Find(Slot);
	return Found ? *Found : nullptr;
}

bool UInventoryComponent::IsSlotEquipped(EEquipSlot Slot) const
{
	return GetEquipped(Slot) != nullptr;
}

int32 UInventoryComponent::GetEquippedDamage(EEquipSlot Slot) const
{
	const UItemInstance* Instance = GetEquipped(Slot);
	return Instance ? Instance->GetDamage() : 0;
}
