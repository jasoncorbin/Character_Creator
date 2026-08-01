// A pickup lying in the world. Spec section 5, plan step 3.
//
// Auto-equip is deliberately OFF: picking something up must never change what the player is
// holding mid-fight.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/RPGInteractable.h"
#include "Items/RPGItemTypes.h"
#include "WorldItem.generated.h"

class UInteractableComponent;
class UItemData;
class URarityPalette;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(Blueprintable, ClassGroup = (RPG))
class CHARACTER_CREATOR_API AWorldItem : public AActor, public IRPGInteractable
{
	GENERATED_BODY()

public:
	AWorldItem();

	// --- payload ------------------------------------------------------------------------

	/** What this pickup grants. Null = inert; the prompt will not show. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Item")
	TObjectPtr<UItemData> Item = nullptr;

	/** Stack size. Materials stack; gear adds one instance per count. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Item", meta = (ClampMin = "1"))
	int32 Count = 1;

	/**
	 * Optional. Supplies the rarity tint handed to OnItemVisualsApplied for the placeholder
	 * block. Nothing breaks when it is unset - the colour is just white.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Item")
	TObjectPtr<URarityPalette> Palette = nullptr;

	// --- presentation -------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Item|Visuals")
	FRotator MeshRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Item|Visuals")
	FVector MeshRelativeScale = FVector(1.0f, 1.0f, 1.0f);

	/** Used when the item has no mesh of its own - materials, mainly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Item|Visuals")
	TObjectPtr<UStaticMesh> PlaceholderMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Item|Visuals")
	FVector PlaceholderScale = FVector(0.25f, 0.25f, 0.25f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Item|Visuals")
	bool bSpin = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Item|Visuals")
	float SpinDegreesPerSecond = 45.0f;

	/**
	 * Hook for a Blueprint child to tint the placeholder block or add a rarity VFX.
	 * Kept out of C++ on purpose - material parameter names are a per-project decision and
	 * hard-coding one here would fail silently against any other material.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "World Item")
	void OnItemVisualsApplied(UItemData* AppliedItem, FLinearColor RarityBlockColor);

	// --- API ----------------------------------------------------------------------------

	/** Configure before FinishSpawning. Use SpawnWorldItem unless you need the deferred window. */
	UFUNCTION(BlueprintCallable, Category = "World Item")
	void Initialize(UItemData* InItem, int32 InCount = 1);

	/**
	 * SpawnActorDeferred -> configure -> FinishSpawning, so the actor's BeginPlay already sees
	 * its payload. Spawning first and assigning after leaves a one-frame window where the
	 * pickup exists with no item, and an overlap in that frame registers an inert prompt.
	 */
	// NOTE: the owner parameter is SpawnOwner, not Owner. UHT rejects a parameter that shadows
	// a property in scope, and AActor already has an Owner.
	UFUNCTION(BlueprintCallable, Category = "World Item",
		meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "SpawnOwner"))
	static AWorldItem* SpawnWorldItem(UObject* WorldContextObject,
		TSubclassOf<AWorldItem> WorldItemClass, UItemData* InItem, int32 InCount,
		const FTransform& SpawnTransform, AActor* SpawnOwner = nullptr);

	UFUNCTION(BlueprintCallable, Category = "World Item")
	void RefreshVisuals();

	// --- IRPGInteractable ---------------------------------------------------------------

	virtual EInteractPriority GetInteractPriority_Implementation() const override;
	virtual FText GetPromptLabel_Implementation() const override;
	virtual bool IsInteractEligible_Implementation(APawn* Interactor) const override;
	virtual bool Interact_Implementation(APawn* Interactor) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Item|Components")
	TObjectPtr<USceneComponent> PivotRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Item|Components")
	TObjectPtr<UStaticMeshComponent> StaticMeshDisplay;

	/** Bows are skeletal; everything else is static. Only one is ever visible. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Item|Components")
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshDisplay;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Item|Components")
	TObjectPtr<UInteractableComponent> InteractionVolume;
};
