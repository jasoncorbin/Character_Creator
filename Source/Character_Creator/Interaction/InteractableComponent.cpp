#include "Interaction/InteractableComponent.h"

#include "Engine/HitResult.h"
#include "GameFramework/Pawn.h"
#include "Interaction/InteractorComponent.h"
#include "Interaction/RPGInteractable.h"

UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 1.5 m, per the plan.
	InitSphereRadius(150.0f);

	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetGenerateOverlapEvents(true);

	// A trigger volume has no business in the depth pass or the navmesh.
	SetHiddenInGame(true);
	SetCanEverAffectNavigation(false);
}

void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &UInteractableComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UInteractableComponent::HandleEndOverlap);

	// A pawn already standing inside the sphere at BeginPlay never fires a begin-overlap
	// event, which is how spawned-under-your-feet loot ends up silently uninteractable.
	TArray<AActor*> Overlapping;
	GetOverlappingActors(Overlapping, APawn::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		if (UInteractorComponent* Interactor = Actor ? Actor->FindComponentByClass<UInteractorComponent>() : nullptr)
		{
			Interactor->Register(this);
		}
	}
}

void UInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DetachFromAllObservers();
	Super::EndPlay(EndPlayReason);
}

void UInteractableComponent::HandleBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (!bInteractionEnabled || !OtherActor)
	{
		return;
	}

	if (UInteractorComponent* Interactor = OtherActor->FindComponentByClass<UInteractorComponent>())
	{
		Interactor->Register(this);
	}
}

void UInteractableComponent::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	if (!OtherActor)
	{
		return;
	}

	if (UInteractorComponent* Interactor = OtherActor->FindComponentByClass<UInteractorComponent>())
	{
		Interactor->Unregister(this);
	}
}

void UInteractableComponent::SetInteractionEnabled(bool bEnabled)
{
	if (bInteractionEnabled == bEnabled)
	{
		return;
	}

	bInteractionEnabled = bEnabled;

	if (!bInteractionEnabled)
	{
		// Drop out of arbitration now. Waiting for the pawn to move would leave a disabled
		// interactable holding the prompt.
		DetachFromAllObservers();
		return;
	}

	// Re-enabling: pick up anyone already standing inside.
	TArray<AActor*> Overlapping;
	GetOverlappingActors(Overlapping, APawn::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		if (UInteractorComponent* Interactor = Actor ? Actor->FindComponentByClass<UInteractorComponent>() : nullptr)
		{
			Interactor->Register(this);
		}
	}
}

void UInteractableComponent::NotifyObserverAdded(UInteractorComponent* Observer)
{
	if (Observer)
	{
		Observers.AddUnique(Observer);
	}
}

void UInteractableComponent::NotifyObserverRemoved(UInteractorComponent* Observer)
{
	Observers.RemoveAll([Observer](const TWeakObjectPtr<UInteractorComponent>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Observer;
	});
}

void UInteractableComponent::DetachFromAllObservers()
{
	// Copy first: Unregister calls back into NotifyObserverRemoved and mutates Observers.
	TArray<TWeakObjectPtr<UInteractorComponent>> Snapshot = Observers;
	for (const TWeakObjectPtr<UInteractorComponent>& Entry : Snapshot)
	{
		if (UInteractorComponent* Interactor = Entry.Get())
		{
			Interactor->Unregister(this);
		}
	}
	Observers.Reset();
}

// --- resolved queries ------------------------------------------------------------------- //

EInteractPriority UInteractableComponent::ResolvePriority() const
{
	// GetOwner() is const-qualified but returns a non-const AActor*, which is what the
	// generated Execute_ dispatchers take.
	AActor* Owner = GetOwner();
	if (Owner && Owner->GetClass()->ImplementsInterface(URPGInteractable::StaticClass()))
	{
		return IRPGInteractable::Execute_GetInteractPriority(Owner);
	}
	return DefaultPriority;
}

FText UInteractableComponent::ResolvePromptLabel() const
{
	// GetOwner() is const-qualified but returns a non-const AActor*, which is what the
	// generated Execute_ dispatchers take.
	AActor* Owner = GetOwner();
	if (Owner && Owner->GetClass()->ImplementsInterface(URPGInteractable::StaticClass()))
	{
		return IRPGInteractable::Execute_GetPromptLabel(Owner);
	}
	return DefaultPromptLabel;
}

bool UInteractableComponent::ResolveEligible(APawn* Interactor) const
{
	if (!bInteractionEnabled)
	{
		return false;
	}

	// GetOwner() is const-qualified but returns a non-const AActor*, which is what the
	// generated Execute_ dispatchers take.
	AActor* Owner = GetOwner();
	if (Owner && Owner->GetClass()->ImplementsInterface(URPGInteractable::StaticClass()))
	{
		return IRPGInteractable::Execute_IsInteractEligible(Owner, Interactor);
	}
	return true;
}

bool UInteractableComponent::ResolveInteract(APawn* Interactor)
{
	if (!bInteractionEnabled)
	{
		return false;
	}

	AActor* Owner = GetOwner();
	if (Owner && Owner->GetClass()->ImplementsInterface(URPGInteractable::StaticClass()))
	{
		return IRPGInteractable::Execute_Interact(Owner, Interactor);
	}

	// No implementer: the component is a pure trigger. Nothing to do, and saying so is
	// honest - returning true here would make failure indistinguishable from success.
	return false;
}
