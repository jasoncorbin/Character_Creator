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

bool UInventoryComponent::CanEquipToSlot(UItemInstance* Instance, EEquipSlot Slot) const
{
	if (!Instance || !Instance->Template || Instance->Template->IsMaterial())
	{
		return false; // materials are never equipped
	}

	if (Slot == EEquipSlot::Count)
	{
		return false;
	}

	// The natural case: the item goes where its template says.
	if (Instance->GetSlot() == Slot)
	{
		return true;
	}

	// The one authored exception - the off-hand accepts a shield or a one-hand sword,
	// which is what makes stances 3 (sword+shield) and 4 (double sword) reachable.
	if (Slot == EEquipSlot::OffHand)
	{
		const EWeaponCategory Category = Instance->GetCategory();
		return Category == EWeaponCategory::Shield || Category == EWeaponCategory::OHS;
	}

	return false;
}

bool UInventoryComponent::Equip(UItemInstance* Instance)
{
	if (!Instance)
	{
		return false;
	}
	return EquipToSlot(Instance, Instance->GetSlot());
}

bool UInventoryComponent::EquipToSlot(UItemInstance* Instance, EEquipSlot Slot)
{
	if (!CanEquipToSlot(Instance, Slot))
	{
		return false;
	}

	// Take it out of the bag if it is there (equipping moves, it does not duplicate).
	GearBag.Remove(Instance);

	// ...and out of any OTHER slot it already occupies, so moving a one-hand sword from
	// Melee to OffHand cannot leave the same instance mounted in both hands.
	EEquipSlot VacatedSlot = EEquipSlot::Count;
	for (const TPair<EEquipSlot, TObjectPtr<UItemInstance>>& Pair : Equipped)
	{
		if (Pair.Key != Slot && Pair.Value == Instance)
		{
			VacatedSlot = Pair.Key;
			break;
		}
	}
	if (VacatedSlot != EEquipSlot::Count)
	{
		Equipped.Remove(VacatedSlot);
		OnEquipChanged.Broadcast(VacatedSlot, nullptr, Instance);
	}

	UItemInstance* Previous = nullptr;
	if (TObjectPtr<UItemInstance>* Existing = Equipped.Find(Slot))
	{
		Previous = *Existing;
	}

	Equipped.Add(Slot, Instance);

	// The fix for Unity's silent drop: the displaced item goes back to the bag.
	// It always fits - Instance just vacated a bag slot, so the net change is zero.
	if (Previous && Previous != Instance)
	{
		GearBag.Add(Previous);
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

	// Refuse rather than destroy the player's gear. The caller can make room and retry.
	if (GearBag.Num() >= Capacity)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Inventory] Bag full - '%s' stays equipped. Free a slot and retry."),
			*Previous->GetDisplayName().ToString());
		return false;
	}

	Equipped.Remove(Slot);
	GearBag.Add(Previous);

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

// --- UI views (step 6) -------------------------------------------------------

bool UInventoryComponent::ItemMatchesTab(const UItemData* Item, EItemTab Tab)
{
	if (Item == nullptr)
	{
		return false;
	}

	switch (Tab)
	{
	case EItemTab::All:
		return true;

	case EItemTab::Materials:
		return Item->Kind == EItemKind::Material;

	case EItemTab::Armor:
		// Slot, not weapon category: a shield is worn in the off-hand and reads as a weapon.
		return Item->Kind == EItemKind::Gear && Item->Slot == EEquipSlot::Armor;

	case EItemTab::Weapons:
		return Item->Kind == EItemKind::Gear && Item->Slot != EEquipSlot::Armor;

	case EItemTab::Potions:
		// No potion kind exists yet - deliberately empty rather than faked. See EItemTab.
		return false;

	default:
		return false;
	}
}

TArray<FInventoryRow> UInventoryComponent::GetRowsForTab(EItemTab Tab) const
{
	TArray<FInventoryRow> Rows;

	// Gear first, in bag order, so a picked-up weapon appears where the player expects it.
	for (const TObjectPtr<UItemInstance>& Instance : GearBag)
	{
		if (Instance == nullptr)
		{
			continue;
		}
		UItemData* ItemTemplate = Instance->Template;
		if (ItemMatchesTab(ItemTemplate, Tab))
		{
			Rows.Emplace(ItemTemplate, Instance, 1);
		}
	}

	// Then material stacks. TMap iteration order is not stable, so sort by Id for a grid
	// that does not reshuffle itself between opens.
	TArray<FInventoryRow> MaterialRows;
	for (const TPair<TObjectPtr<UItemData>, int32>& Pair : Materials)
	{
		if (Pair.Value > 0 && ItemMatchesTab(Pair.Key, Tab))
		{
			MaterialRows.Emplace(Pair.Key, nullptr, Pair.Value);
		}
	}
	MaterialRows.Sort([](const FInventoryRow& A, const FInventoryRow& B)
	{
		return A.Item->Id.LexicalLess(B.Item->Id);
	});

	Rows.Append(MaterialRows);
	return Rows;
}
