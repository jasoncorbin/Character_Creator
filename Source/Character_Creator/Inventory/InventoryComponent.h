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

/**
 * The bag grid's category tabs (design handoff, Items Bag panel).
 *
 * Potions is authored but has no backing data - there is no potion EItemKind - so it always
 * yields an empty list. The design shows it dimmed/disabled for exactly this reason; keeping
 * the entry means the tab strip is data-driven rather than special-cased in the widget.
 */
UENUM(BlueprintType)
enum class EItemTab : uint8
{
	All       UMETA(DisplayName = "All"),
	Weapons   UMETA(DisplayName = "Weapons"),
	Armor     UMETA(DisplayName = "Armor"),
	Materials UMETA(DisplayName = "Materials"),
	Potions   UMETA(DisplayName = "Potions"),

	Count     UMETA(Hidden)
};

/**
 * One cell of the bag grid. Covers both halves of the inventory:
 *   gear     -> Instance set, Count 1
 *   material -> Instance null, Count = stack size
 * The x N badge shows only when Count > 1, so gear never gets one.
 */
USTRUCT(BlueprintType)
struct CHARACTER_CREATOR_API FInventoryRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UItemData> Item = nullptr;

	/** Null for material stacks - materials have no per-copy identity. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UItemInstance> Instance = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Count = 0;

	FInventoryRow() = default;

	FInventoryRow(UItemData* InItem, UItemInstance* InInstance, int32 InCount)
		: Item(InItem), Instance(InInstance), Count(InCount) {}
};

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

	// --- UI views (step 6) ---------------------------------------------------

	/**
	 * One tab's worth of bag contents, gear and materials in a single ordered list.
	 *
	 * The grid mixes unique gear instances with stacked materials, so the widget needs one
	 * row type covering both rather than zipping two arrays in Blueprint. Gear rows carry
	 * an Instance and Count 1; material rows carry a null Instance and the stack size.
	 * Ordering is gear first (bag order), then materials - matching the design's reference grid.
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory|UI")
	TArray<FInventoryRow> GetRowsForTab(EItemTab Tab) const;

	/** Does this item belong under the given tab? Exposed so the UI can badge counts per tab. */
	UFUNCTION(BlueprintPure, Category = "Inventory|UI")
	static bool ItemMatchesTab(const UItemData* Item, EItemTab Tab);

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
