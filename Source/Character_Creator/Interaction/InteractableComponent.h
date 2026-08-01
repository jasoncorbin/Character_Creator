// The interactable side: a trigger sphere that registers itself with whatever interactor
// walks into it. Spec section 5.
//
// It IS the sphere rather than owning one, so an actor gets a working interaction volume by
// adding a single component.
//
// It does not decide anything itself. If the owning actor implements IRPGInteractable, every
// query forwards there; otherwise the component's own authored defaults answer. That means a
// static "Press E to open" prop needs no code at all, while AWorldItem can compute its label
// from live data.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Items/RPGItemTypes.h"
#include "InteractableComponent.generated.h"

class APawn;
class UInteractorComponent;

UCLASS(ClassGroup = (RPG), meta = (BlueprintSpawnableComponent),
	HideCategories = (Object, LOD, Lighting, TextureStreaming))
class CHARACTER_CREATOR_API UInteractableComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UInteractableComponent();

	/** Used only when the owner does not implement IRPGInteractable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EInteractPriority DefaultPriority = EInteractPriority::Pickup;

	/** Used only when the owner does not implement IRPGInteractable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText DefaultPromptLabel;

	/**
	 * Master switch. Turning this off deregisters from every interactor immediately rather
	 * than waiting for the pawn to walk out - otherwise a disabled interactable keeps
	 * winning arbitration until someone moves.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bInteractionEnabled = true;

	// --- resolved queries: owner interface first, authored defaults second ---------------

	UFUNCTION(BlueprintPure, Category = "Interaction")
	EInteractPriority ResolvePriority() const;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText ResolvePromptLabel() const;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool ResolveEligible(APawn* Interactor) const;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool ResolveInteract(APawn* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionEnabled(bool bEnabled);

	/** Interactors call these; they are not meant for gameplay code. */
	void NotifyObserverAdded(UInteractorComponent* Observer);
	void NotifyObserverRemoved(UInteractorComponent* Observer);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	/** Every interactor currently holding a registration for this component. */
	TArray<TWeakObjectPtr<UInteractorComponent>> Observers;

	/** Deregisters from all observers. Called on disable and on EndPlay. */
	void DetachFromAllObservers();
};
