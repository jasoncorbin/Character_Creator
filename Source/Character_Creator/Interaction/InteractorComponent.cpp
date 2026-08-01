#include "Interaction/InteractorComponent.h"

#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "Interaction/InteractableComponent.h"

UInteractorComponent::UInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.2f;
}

void UInteractorComponent::BeginPlay()
{
	Super::BeginPlay();
	PrimaryComponentTick.TickInterval = ReevaluateInterval;
}

void UInteractorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Tell each interactable to forget us, or they hold a dangling observer entry.
	TArray<TObjectPtr<UInteractableComponent>> Snapshot = Registered;
	for (const TObjectPtr<UInteractableComponent>& Entry : Snapshot)
	{
		if (Entry)
		{
			Entry->NotifyObserverRemoved(this);
		}
	}
	Registered.Reset();
	SetActiveInteractable(nullptr);

	Super::EndPlay(EndPlayReason);
}

void UInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Reevaluate();

	if (bShowDebugPrompt && ActiveInteractable && GEngine)
	{
		// Keyed so it replaces itself rather than stacking up the log.
		GEngine->AddOnScreenDebugMessage(
			static_cast<uint64>(GetUniqueID()), ReevaluateInterval * 1.5f, FColor::Cyan,
			FString::Printf(TEXT("[E] %s"), *GetActivePromptLabel().ToString()));
	}
}

APawn* UInteractorComponent::GetOwnerPawn() const
{
	return Cast<APawn>(GetOwner());
}

void UInteractorComponent::Register(UInteractableComponent* Interactable)
{
	if (!Interactable || Registered.Contains(Interactable))
	{
		return;
	}

	Registered.Add(Interactable);
	Interactable->NotifyObserverAdded(this);
	Reevaluate();
}

void UInteractorComponent::Unregister(UInteractableComponent* Interactable)
{
	if (!Interactable)
	{
		return;
	}

	const int32 Removed = Registered.Remove(Interactable);
	Interactable->NotifyObserverRemoved(this);

	if (Removed > 0)
	{
		Reevaluate();
	}
}

void UInteractorComponent::Reevaluate()
{
	APawn* Pawn = GetOwnerPawn();

	// Prune anything destroyed since the last pass, then keep only what is eligible NOW.
	Registered.RemoveAll([](const TObjectPtr<UInteractableComponent>& Entry)
	{
		return !IsValid(Entry.Get());
	});

	UInteractableComponent* Best = nullptr;
	uint8 BestPriority = 0;
	double BestDistanceSq = 0.0;
	FString BestName;

	const FVector Origin = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;

	for (const TObjectPtr<UInteractableComponent>& Entry : Registered)
	{
		UInteractableComponent* Candidate = Entry.Get();
		if (!Candidate || !Candidate->ResolveEligible(Pawn))
		{
			continue;
		}

		const uint8 Priority = static_cast<uint8>(Candidate->ResolvePriority());
		const double DistanceSq = FVector::DistSquared(Origin, Candidate->GetComponentLocation());
		const FString Name = Candidate->GetFullName();

		// Strict ordering: priority desc, then distance asc, then name asc. The last term
		// exists purely so two identical candidates can never resolve arbitrarily.
		bool bWins = false;
		if (!Best)
		{
			bWins = true;
		}
		else if (Priority != BestPriority)
		{
			bWins = Priority > BestPriority;
		}
		else if (!FMath::IsNearlyEqual(DistanceSq, BestDistanceSq, 1.0))
		{
			bWins = DistanceSq < BestDistanceSq;
		}
		else
		{
			bWins = FCString::Strcmp(*Name, *BestName) < 0;
		}

		if (bWins)
		{
			Best = Candidate;
			BestPriority = Priority;
			BestDistanceSq = DistanceSq;
			BestName = Name;
		}
	}

	SetActiveInteractable(Best);
}

void UInteractorComponent::SetActiveInteractable(UInteractableComponent* NewActive)
{
	if (ActiveInteractable == NewActive)
	{
		return;
	}

	ActiveInteractable = NewActive;
	OnActiveInteractableChanged.Broadcast(ActiveInteractable, GetActivePromptLabel());
}

FText UInteractorComponent::GetActivePromptLabel() const
{
	return ActiveInteractable ? ActiveInteractable->ResolvePromptLabel() : FText::GetEmpty();
}

bool UInteractorComponent::TryInteract()
{
	// Re-derive first: the press may be the first thing to happen in this 0.2 s window.
	Reevaluate();

	UInteractableComponent* Target = ActiveInteractable;
	if (!Target)
	{
		return false;
	}

	const bool bSucceeded = Target->ResolveInteract(GetOwnerPawn());
	OnInteractAttempted.Broadcast(Target, bSucceeded);

	// The interaction may have destroyed the target or changed what is eligible.
	Reevaluate();

	return bSucceeded;
}
