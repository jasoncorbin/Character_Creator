// The player side: holds every interactable in range and arbitrates a single winner.
// Spec section 5.
//
// Unity's version kept the candidates in a HashSet and its own source flags the iteration
// order as not-to-be-relied-upon, which makes a tie between two equal-priority pickups
// resolve differently run to run. Here the winner is fully determined:
//
//     highest priority  ->  then nearest  ->  then lowest full object name
//
// The name tiebreak only ever fires for two objects at identical priority AND identical
// distance, but it means there is no such thing as an undefined outcome.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractorComponent.generated.h"

class APawn;
class UInteractableComponent;

/** Fires whenever the winning interactable changes, including to null. Drive the prompt from this. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActiveInteractableChanged,
	UInteractableComponent*, NewActive, FText, PromptLabel);

/** Fires on every interact attempt, successful or not. Useful for SFX / failure feedback. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractAttempted,
	UInteractableComponent*, Target, bool, bSucceeded);

UCLASS(ClassGroup = (RPG), meta = (BlueprintSpawnableComponent))
class CHARACTER_CREATOR_API UInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractorComponent();

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnActiveInteractableChanged OnActiveInteractableChanged;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnInteractAttempted OnInteractAttempted;

	/**
	 * Eligibility is not static - a bag fills up, a chest gets emptied - so the winner is
	 * re-derived on a slow tick as well as on register/deregister. 5 Hz is well under the
	 * threshold where a player notices, over a list that is realistically 0-3 entries.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.02"))
	float ReevaluateInterval = 0.2f;

	/** On-screen prompt text while no UMG widget exists yet. Turn off once step 6 lands. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Debug")
	bool bShowDebugPrompt = true;

	// --- registration, called by UInteractableComponent ---------------------------------

	void Register(UInteractableComponent* Interactable);
	void Unregister(UInteractableComponent* Interactable);

	// --- API ----------------------------------------------------------------------------

	/** Bind IA_RPG_Interact (E) to this. @return whether the interaction actually completed. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool TryInteract();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	UInteractableComponent* GetActiveInteractable() const { return ActiveInteractable; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetActivePromptLabel() const;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool HasActiveInteractable() const { return ActiveInteractable != nullptr; }

	/** Force an immediate re-derivation. Call after anything that changes eligibility. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Reevaluate();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** The current winner. Null when nothing in range is eligible. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction", Transient)
	TObjectPtr<UInteractableComponent> ActiveInteractable = nullptr;

private:
	/** Everything overlapping, eligible or not. Strong refs; entries are pruned on use. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInteractableComponent>> Registered;

	APawn* GetOwnerPawn() const;

	/**
	 * Sole writer of ActiveInteractable. Broadcasts only on an actual change.
	 * NOT named SetActive - UActorComponent::SetActive(bool, bool) already exists, and an
	 * overload that hides a base virtual trips C4263/C4264.
	 */
	void SetActiveInteractable(UInteractableComponent* NewActive);
};
