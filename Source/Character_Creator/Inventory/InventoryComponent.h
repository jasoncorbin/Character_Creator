// Player inventory + equipment. Spec section 4.
//
// Three deliberately separate containers. Two of Unity's known bugs are FIXED here rather
// than replicated: Equip returns the displaced item to the bag, and the equip delegate
// carries the slot so consumers can tell which slot emptied.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/RPGItemTypes.h"
#include "InventoryComponent.generated.h"

class UItemData;
class UItemInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAdded, UItemData*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, UItemData*, Item);

/**
 * Fired on every equip AND unequip. NewItem is null on unequip, OldItem is null when the
 * slot was empty. Slot is always valid - this is the fix for Unity's OnWeaponEquipped(null),
 * which left consumers unable to tell which slot changed.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEquipChanged, EEquipSlot, Slot,
	UItemInstance*, NewItem, UItemInstance*, OldItem);

UCLASS(ClassGroup = (RPG), meta = (BlueprintSpawnableComponent))
class CHARACTER_CREATOR_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	// --- events --------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemAdded OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemRemoved OnItemRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnEquipChanged OnEquipChanged;

	// --- config --------------------------------------------------------------

	/** Gear capacity. Materials are uncapped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 Capacity = 20;

	// --- API -----------------------------------------------------------------

	/**
	 * Material -> stack += Count. Gear -> appends Count instances, stopping at capacity.
	 * @return how many were actually added (0 = nothing fit). Callers MUST check this:
	 *         returning a bool is what lets Unity's pickup destroy itself while silently
	 *         losing the remainder of a partial add.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(UItemData* Item, int32 Count = 1);

	/** Removes a specific gear instance from the bag. Materials cannot be removed this way. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(UItemInstance* Instance);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(UItemData* Item) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetGearCount() const { return GearBag.Num(); }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsFull() const { return GearBag.Num() >= Capacity; }

	/** Returned by value - Blueprint-exposed functions cannot return const references. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<UItemInstance*> GetGearBag() const;

	// --- materials -----------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Inventory|Materials")
	int32 GetMaterialCount(UItemData* Item) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Materials")
	TMap<UItemData*, int32> GetMaterials() const;

	/** Fails if the stack is short. Removes the key at 0. The forge's spend path. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Materials")
	bool SpendMaterial(UItemData* Item, int32 Amount);

	// --- equipment -----------------------------------------------------------

	/**
	 * Equips an instance into its item's DEFAULT slot (Template->Slot).
	 * Convenience wrapper over EquipToSlot.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equipment")
	bool Equip(UItemInstance* Instance);

	/**
	 * Equips an instance into an EXPLICIT slot. The instance leaves the bag (and any other
	 * slot it occupied); anything previously in the target slot is returned to the bag
	 * (Unity dropped it on the floor). Fires OnEquipChanged.
	 *
	 * The explicit-slot form exists because a one-hand sword is valid in EITHER hand -
	 * Melee, or OffHand for the DoubleSword stance (plan step 4, stance 4). Template->Slot
	 * is a single value, so Equip() alone can never reach the off-hand path, which is also
	 * the whole reason UItemData carries AttachRotationOffHand.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equipment")
	bool EquipToSlot(UItemInstance* Instance, EEquipSlot Slot);

	/** Gear-only, plus the off-hand exception for shields and one-hand swords. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Equipment")
	bool CanEquipToSlot(UItemInstance* Instance, EEquipSlot Slot) const;

	/**
	 * Clears a slot and returns its occupant to the bag. Fires OnEquipChanged.
	 * @return false (and the item stays equipped) if the bag is full - never destroys gear.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equipment")
	bool Unequip(EEquipSlot Slot);

	UFUNCTION(BlueprintPure, Category = "Inventory|Equipment")
	UItemInstance* GetEquipped(EEquipSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Equipment")
	bool IsSlotEquipped(EEquipSlot Slot) const;

	/** Convenience for the damage source (spec 6b) - 0 when nothing is equipped. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Equipment")
	int32 GetEquippedDamage(EEquipSlot Slot) const;

protected:
	/** Ordered, duplicates allowed, capped at Capacity. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<TObjectPtr<UItemInstance>> GearBag;

	/** Uncapped stacks, keyed by asset identity. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TMap<TObjectPtr<UItemData>, int32> Materials;

	/** Exactly one item per slot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TMap<EEquipSlot, TObjectPtr<UItemInstance>> Equipped;
};
