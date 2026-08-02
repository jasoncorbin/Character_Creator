// Drops a loot table's contents into the world on death. Spec section 9, plan step 5.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Templates/SubclassOf.h"
#include "LootDropperComponent.generated.h"

class AWorldItem;
class ULootTable;
class URarityPalette;
class UStaticMesh;

/**
 * Sits on an enemy and scatters loot when that enemy dies.
 *
 * Deliberately NOT wired to BPC_PlayerStats: that is shared CC-archive code, and the RPG
 * enemy is a duplicate of Dummy rather than a subclass, so anything hung off the shared
 * component fires for owners it was never meant to serve.
 */
UCLASS(ClassGroup = (RPG), meta = (BlueprintSpawnableComponent))
class CHARACTER_CREATOR_API ULootDropperComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULootDropperComponent();

	/** Null = this enemy drops nothing. Not an error. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TObjectPtr<ULootTable> LootTable = nullptr;

	/** The pickup actor class to spawn. Usually a Blueprint child of AWorldItem. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TSubclassOf<AWorldItem> WorldItemClass;

	/** Optional rarity tint source, handed to each spawned pickup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TObjectPtr<URarityPalette> Palette = nullptr;

	// --- placeholder visuals ---------------------------------------------------------------
	//
	// These are NOT optional in practice. Material archetypes (DA_Item_Mat1/2/3) carry no
	// StaticMeshAsset of their own, so a spawned pickup with no PlaceholderMesh renders
	// nothing at all - the loot drops, is collectable, and is invisible. Gear has its own
	// mesh and ignores these.

	/** Spec section 9: cube = gear. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Placeholder")
	TObjectPtr<UStaticMesh> GearPlaceholderMesh = nullptr;

	/** Spec section 9: sphere = material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Placeholder")
	TObjectPtr<UStaticMesh> MaterialPlaceholderMesh = nullptr;

	/** Spec section 9: 0.3. Note the hand-placed pickups in the test level use 0.25. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Placeholder")
	FVector PlaceholderScale = FVector(0.3f, 0.3f, 0.3f);

	/** Horizontal scatter radius in cm. Spec: 0.6 m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Scatter", meta = (ClampMin = "0.0"))
	float ScatterRadius = 60.0f;

	/** Spawn height above the owner's origin in cm. Spec: 0.6 m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Scatter")
	float SpawnHeight = 60.0f;

	/**
	 * Roll the table and scatter the results.
	 *
	 * Call this from the enemy's death branch. SINGLE-FIRE: every call after the first is
	 * ignored, so a re-raised death event cannot double-drop.
	 *
	 * Safe to call from inside a collision callback - the spawning itself is deferred to the
	 * next tick. See the .cpp for why that matters.
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void DropLoot();

	UFUNCTION(BlueprintPure, Category = "Loot")
	bool HasDropped() const { return bHasDropped; }

private:
	/** The actual spawn work, run one tick after DropLoot. */
	void SpawnRolledLoot();

	/**
	 * Set before the null-table check in DropLoot, on purpose: a misconfigured dropper must
	 * still latch, or it stays armed and fires again on the next death event.
	 */
	bool bHasDropped = false;
};
