#include "Player/RPGPlayerCharacter.h"

#include "Inventory/InventoryComponent.h"
#include "Items/ItemData.h"
#include "Items/ItemInstance.h"

ARPGPlayerCharacter::ARPGPlayerCharacter()
{
	// Size the tables so a freshly created child has 8 usable rows rather than empty arrays
	// that every IsValidIndex check would reject. The Blueprint's authored values overwrite
	// these on load; this only matters for a brand-new subclass.
	StanceRightMeshes.SetNum(NumStances);
	StanceLeftMeshes.SetNum(NumStances);
	StanceRightRotations.SetNum(NumStances);
	StanceLeftRotations.SetNum(NumStances);
	StanceIsRanged.SetNum(NumStances);
}

void ARPGPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UInventoryComponent* Inv = GetInventory())
	{
		Inv->OnEquipChanged.AddDynamic(this, &ARPGPlayerCharacter::HandleEquipChanged);
	}
}

UInventoryComponent* ARPGPlayerCharacter::GetInventory() const
{
	// Found rather than owned: the component is added in the Blueprint's component tree, and
	// moving it into C++ would mean rebuilding that tree for no gain.
	return FindComponentByClass<UInventoryComponent>();
}

const UItemData* ARPGPlayerCharacter::FindMountedItem(EMountPoint MountPoint,
	TArrayView<const EEquipSlot> SlotsInPriorityOrder, EEquipSlot& OutFoundSlot) const
{
	const UInventoryComponent* Inv = GetInventory();
	if (!Inv)
	{
		return nullptr;
	}

	for (const EEquipSlot Slot : SlotsInPriorityOrder)
	{
		const UItemInstance* Instance = Inv->GetEquipped(Slot);
		const UItemData* Data = Instance ? Instance->Template.Get() : nullptr;
		if (Data && Data->MountPoint == MountPoint)
		{
			OutFoundSlot = Slot;
			return Data;
		}
	}
	return nullptr;
}

bool ARPGPlayerCharacter::ResolveRightHandMount(UStaticMesh*& OutMesh, FRotator& OutRotation) const
{
	// Melee first, then Ranged - a wand is Ranged but mounts in the RIGHT hand, which is
	// exactly why an item declares its mount point instead of it being inferred from the slot.
	static constexpr EEquipSlot Order[] = { EEquipSlot::Melee, EEquipSlot::Ranged };

	EEquipSlot FoundSlot = EEquipSlot::Melee;
	if (const UItemData* Data = FindMountedItem(EMountPoint::RightHand, Order, FoundSlot))
	{
		OutMesh = Data->StaticMeshAsset.LoadSynchronous();
		OutRotation = Data->GetAttachRotationForSlot(FoundSlot);
		return true;
	}

	OutMesh = StanceRightMeshes.IsValidIndex(CurrentStance)
		? StanceRightMeshes[CurrentStance].Get() : nullptr;
	OutRotation = StanceRightRotations.IsValidIndex(CurrentStance)
		? StanceRightRotations[CurrentStance] : FRotator::ZeroRotator;
	return false;
}

bool ARPGPlayerCharacter::ResolveLeftHandMount(UStaticMesh*& OutMesh, FRotator& OutRotation) const
{
	static constexpr EEquipSlot Order[] = { EEquipSlot::OffHand };

	EEquipSlot FoundSlot = EEquipSlot::OffHand;
	if (const UItemData* Data = FindMountedItem(EMountPoint::LeftHand, Order, FoundSlot))
	{
		OutMesh = Data->StaticMeshAsset.LoadSynchronous();
		OutRotation = Data->GetAttachRotationForSlot(FoundSlot);
		return true;
	}

	OutMesh = StanceLeftMeshes.IsValidIndex(CurrentStance)
		? StanceLeftMeshes[CurrentStance].Get() : nullptr;
	OutRotation = StanceLeftRotations.IsValidIndex(CurrentStance)
		? StanceLeftRotations[CurrentStance] : FRotator::ZeroRotator;
	return false;
}

bool ARPGPlayerCharacter::ResolveBowRigMount(USkeletalMesh*& OutMesh, FRotator& OutRotation) const
{
	static constexpr EEquipSlot Order[] = { EEquipSlot::Ranged };

	EEquipSlot FoundSlot = EEquipSlot::Ranged;
	if (const UItemData* Data = FindMountedItem(EMountPoint::BowRig, Order, FoundSlot))
	{
		OutMesh = Data->SkeletalMeshAsset.LoadSynchronous();
		OutRotation = Data->GetAttachRotationForSlot(FoundSlot);
		return true;
	}

	// No table to fall back to - the Bow component already holds its authored mesh, and
	// ApplyStance only toggles its visibility. Null means "leave the mesh alone".
	OutMesh = nullptr;
	OutRotation = StanceLeftRotations.IsValidIndex(CurrentStance)
		? StanceLeftRotations[CurrentStance] : FRotator::ZeroRotator;
	return false;
}

int32 ARPGPlayerCharacter::GetMeleeDamage() const
{
	const UInventoryComponent* Inv = GetInventory();
	const UItemInstance* Instance = Inv ? Inv->GetEquipped(EEquipSlot::Melee) : nullptr;
	return Instance ? Instance->GetDamage() : UnarmedMeleeDamage;
}

int32 ARPGPlayerCharacter::GetRangedDamage() const
{
	const UInventoryComponent* Inv = GetInventory();
	const UItemInstance* Instance = Inv ? Inv->GetEquipped(EEquipSlot::Ranged) : nullptr;
	return Instance ? Instance->GetDamage() : DefaultRangedDamage;
}

bool ARPGPlayerCharacter::IsCurrentStanceRanged() const
{
	return StanceIsRanged.IsValidIndex(CurrentStance) && StanceIsRanged[CurrentStance];
}

int32 ARPGPlayerCharacter::DeriveStanceFromEquipment() const
{
	const UInventoryComponent* Inv = GetInventory();
	if (!Inv)
	{
		return CurrentStance;
	}

	const UItemInstance* Ranged = Inv->GetEquipped(EEquipSlot::Ranged);
	const UItemInstance* Melee = Inv->GetEquipped(EEquipSlot::Melee);
	const UItemInstance* OffHand = Inv->GetEquipped(EEquipSlot::OffHand);

	// Ranged beats melee.
	if (Ranged)
	{
		switch (Ranged->GetCategory())
		{
		case EWeaponCategory::Bow:  return 7;
		case EWeaponCategory::Wand: return 6;
		default: break;
		}
	}

	// Nothing equipped anywhere: leave the stance alone so the Q dev-cycle still owns it.
	if (!Melee && !OffHand)
	{
		return CurrentStance;
	}

	if (!Melee)
	{
		return 0; // off-hand only - unarmed main hand
	}

	switch (Melee->GetCategory())
	{
	case EWeaponCategory::THS:   return 2;
	case EWeaponCategory::Spear: return 5;
	case EWeaponCategory::OHS:
		if (OffHand)
		{
			switch (OffHand->GetCategory())
			{
			case EWeaponCategory::Shield: return 3;
			case EWeaponCategory::OHS:    return 4;
			default: break;
			}
		}
		return 1;
	default:
		return 0;
	}
}

void ARPGPlayerCharacter::HandleEquipChanged(EEquipSlot Slot, UItemInstance* NewItem,
	UItemInstance* OldItem)
{
	CurrentStance = DeriveStanceFromEquipment();
	OnEquipmentChanged(Slot);
}
