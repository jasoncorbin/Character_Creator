// The contract every interactable satisfies. Spec section 5.
//
// Deliberately an interface rather than a base actor class: the loot pickup, a chest and a
// future assassination target have nothing in common structurally, only behaviourally.
//
// NOTE the function is called Interact, not Execute - UHT generates a static
// IRPGInteractable::Execute_<Name>() dispatcher for every BlueprintNativeEvent, and a
// function literally named Execute would produce Execute_Execute.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Items/RPGItemTypes.h"
#include "RPGInteractable.generated.h"

class APawn;

UINTERFACE(MinimalAPI, Blueprintable, BlueprintType)
class URPGInteractable : public UInterface
{
	GENERATED_BODY()
};

class CHARACTER_CREATOR_API IRPGInteractable
{
	GENERATED_BODY()

public:
	/** Higher wins. Ties are broken by distance, then by name - never by container order. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	EInteractPriority GetInteractPriority() const;

	/** What the prompt reads, e.g. "Pick Up Iron Sword". */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetPromptLabel() const;

	/**
	 * Can this pawn interact right now? Re-checked on a cadence, so it may change while the
	 * pawn stands still. An interactable that is merely going to FAIL should still be
	 * eligible - refusing here hides the prompt, which reads as "nothing here".
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool IsInteractEligible(APawn* Interactor) const;

	/** Do the thing. @return false if it could not be completed (bag full, and so on). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool Interact(APawn* Interactor);
};
