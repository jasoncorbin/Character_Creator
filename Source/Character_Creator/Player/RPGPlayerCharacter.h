// The C++ base for BP_RPG_PlayerCharacter. Plan step 4.
//
// WHY THIS EXISTS: to close the Blueprint/C++ seam. The stance tables used to be Blueprint
// variables, which C++ could not read - so every equip consumer would have had to be a
// Blueprint Select between "the equipped item" and "the stance array". With the tables down
// here, the resolvers below answer that question in one call and the graph just uses the
// result.
//
// This is a BASE, not a replacement. BP_RPG_PlayerCharacter keeps its component tree, its
// 189-node EventGraph, and every montage / dodge / combo table - all the tuned behaviour that
// took several sessions to get right. Only the stance TABLES and the equip LOGIC moved.
//
// The property names here deliberately MATCH the old Blueprint variable names, so the
// existing Get/Set nodes rebind to these on reparent instead of breaking.

#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "GameFramework/Character.h"
#include "Items/RPGItemTypes.h"
#include "RPGPlayerCharacter.generated.h"

class UInventoryComponent;
class UItemInstance;
class USkeletalMesh;
class UStaticMesh;

UCLASS(Blueprintable)
class CHARACTER_CREATOR_API ARPGPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ARPGPlayerCharacter();

	/** Authored stance count. Every table below is sized to this. */
	static constexpr int32 NumStances = 8;

	// --- stance tables -------------------------------------------------------------------
	// Moved down from Blueprint. Still EditAnywhere (they were instance-editable as BP vars),
	// so they stay tunable in the Details / Class Defaults panel with no rebuild.

	/** Runtime stance index. Stays an int32 on purpose - there is no Int->UserDefinedEnum
	 *  conversion in Blueprint, and the AnimBP's Blend Poses by Int consumes this directly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Stance")
	int32 CurrentStance = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Stance")
	TArray<TObjectPtr<UStaticMesh>> StanceRightMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Stance")
	TArray<TObjectPtr<UStaticMesh>> StanceLeftMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Stance")
	TArray<FRotator> StanceRightRotations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Stance")
	TArray<FRotator> StanceLeftRotations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Stance")
	TArray<bool> StanceIsRanged;

	// --- combat defaults -----------------------------------------------------------------

	/** Damage dealt with nothing equipped. This is the hard-coded 20 that used to sit as a
	 *  literal on the ApplyDamage node inside MeleeHit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Combat", meta = (ClampMin = "0"))
	int32 UnarmedMeleeDamage = 20;

	/** Arrow damage with no ranged item equipped. Matches BP_Arrow's authored default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Combat", meta = (ClampMin = "0"))
	int32 DefaultRangedDamage = 30;

	// --- equip resolvers (plan step 4a) --------------------------------------------------
	//
	// Contract for all three: the OUT params are ALWAYS written with the value to use, so the
	// caller never needs a fallback branch of its own. Equipped items win; the stance tables
	// are the fallback. That is what keeps the Q dev-cycle working with an empty inventory,
	// which spec 6c explicitly requires.
	//
	// These return VOID deliberately. An earlier revision returned bool ("did this come from
	// an item?"), which was a trap: UE's Python binding maps a bool-returning UFUNCTION with
	// out-params to "tuple if true, None if false", so the fallback path - the common one -
	// silently dropped both outputs and could not be tested. A bool return also just reads as
	// success/failure to anyone else. The provenance bit was never needed: the only caller
	// that cares is the bow rig, and that case is already expressed by OutMesh being null.

	UFUNCTION(BlueprintPure, Category = "RPG|Equip")
	void ResolveRightHandMount(UStaticMesh*& OutMesh, FRotator& OutRotation) const;

	UFUNCTION(BlueprintPure, Category = "RPG|Equip")
	void ResolveLeftHandMount(UStaticMesh*& OutMesh, FRotator& OutRotation) const;

	/**
	 * The bow rig is the odd one out: there is no StanceBowMeshes table, because the Bow
	 * component carries its authored mesh and ApplyStance only toggles visibility. So on the
	 * fallback path OutMesh is null, meaning "leave the component's mesh alone".
	 */
	UFUNCTION(BlueprintPure, Category = "RPG|Equip")
	void ResolveBowRigMount(USkeletalMesh*& OutMesh, FRotator& OutRotation) const;

	// --- damage source (plan step 4b) ----------------------------------------------------
	// Pulled at swing time, never cached: an equip swap mid-combo then lands on the very next
	// hit with no invalidation logic.

	UFUNCTION(BlueprintPure, Category = "RPG|Combat")
	int32 GetMeleeDamage() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Combat")
	int32 GetRangedDamage() const;

	// --- stance bridge (plan step 4c) ----------------------------------------------------

	/**
	 * Re-derives the stance index from what is equipped. Switches on the item's Category,
	 * never on mesh or asset name prefixes (spec 6c). Ranged beats melee.
	 * Returns the CURRENT stance unchanged when nothing is equipped at all, so an empty
	 * inventory never fights the Q dev-cycle.
	 */
	UFUNCTION(BlueprintPure, Category = "RPG|Equip")
	int32 DeriveStanceFromEquipment() const;

	/** True when the current stance is a ranged one. Table-driven, bounds-safe. */
	UFUNCTION(BlueprintPure, Category = "RPG|Stance")
	bool IsCurrentStanceRanged() const;

	/** The inventory living on this pawn. Null-safe; the component is added in Blueprint. */
	UFUNCTION(BlueprintPure, Category = "RPG|Equip")
	UInventoryComponent* GetInventory() const;

protected:
	virtual void BeginPlay() override;

	/** Bound to the inventory's OnEquipChanged. Re-derives the stance, then calls the
	 *  Blueprint hook below so the graph can run ApplyStance. */
	UFUNCTION()
	void HandleEquipChanged(EEquipSlot Slot, UItemInstance* NewItem, UItemInstance* OldItem);

	/**
	 * Blueprint hook, fired after CurrentStance has been re-derived. Wire this to ApplyStance.
	 * Kept as an event rather than calling ApplyStance from C++ because ApplyStance is still a
	 * Blueprint graph - this is the single seam between the two halves.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "RPG|Equip")
	void OnEquipmentChanged(EEquipSlot Slot);

private:
	/** Shared by both hand resolvers: returns the equipped item that mounts at MountPoint,
	 *  searching the given slots in order, or null. */
	const class UItemData* FindMountedItem(EMountPoint MountPoint,
		TArrayView<const EEquipSlot> SlotsInPriorityOrder, EEquipSlot& OutFoundSlot) const;
};
