#include "Interaction/WorldItem.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Interaction/InteractableComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Items/ItemData.h"
#include "Items/RarityPalette.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "RPGWorldItem"

AWorldItem::AWorldItem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // enabled in BeginPlay only when bSpin

	PivotRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PivotRoot"));
	SetRootComponent(PivotRoot);

	StaticMeshDisplay = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshDisplay"));
	StaticMeshDisplay->SetupAttachment(PivotRoot);
	StaticMeshDisplay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshDisplay->SetCanEverAffectNavigation(false);

	SkeletalMeshDisplay = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshDisplay"));
	SkeletalMeshDisplay->SetupAttachment(PivotRoot);
	SkeletalMeshDisplay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshDisplay->SetCanEverAffectNavigation(false);
	// Nothing animates a dropped bow. Leaving it ticking costs a full anim update per pickup.
	SkeletalMeshDisplay->PrimaryComponentTick.bCanEverTick = false;
	SkeletalMeshDisplay->SetComponentTickEnabled(false);

	InteractionVolume = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(PivotRoot);
	InteractionVolume->DefaultPriority = EInteractPriority::Pickup;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		PlaceholderMesh = CubeFinder.Object;
	}
}

void AWorldItem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshVisuals();
}

void AWorldItem::BeginPlay()
{
	Super::BeginPlay();
	RefreshVisuals();
	SetActorTickEnabled(bSpin && !FMath::IsNearlyZero(SpinDegreesPerSecond));
}

void AWorldItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bSpin && PivotRoot)
	{
		// Spin the display, not the actor - rotating the actor would drag the trigger sphere
		// with it, and an off-centre sphere would make the pickup radius direction-dependent.
		StaticMeshDisplay->AddLocalRotation(FRotator(0.0f, SpinDegreesPerSecond * DeltaSeconds, 0.0f));
		SkeletalMeshDisplay->AddLocalRotation(FRotator(0.0f, SpinDegreesPerSecond * DeltaSeconds, 0.0f));
	}
}

void AWorldItem::Initialize(UItemData* InItem, int32 InCount)
{
	Item = InItem;
	Count = FMath::Max(1, InCount);
	RefreshVisuals();
}

AWorldItem* AWorldItem::SpawnWorldItem(UObject* WorldContextObject,
	TSubclassOf<AWorldItem> WorldItemClass, UItemData* InItem, int32 InCount,
	const FTransform& SpawnTransform, AActor* SpawnOwner)
{
	if (!WorldContextObject || !InItem)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const TSubclassOf<AWorldItem> ClassToSpawn =
		WorldItemClass ? WorldItemClass : TSubclassOf<AWorldItem>(AWorldItem::StaticClass());

	AWorldItem* Spawned = World->SpawnActorDeferred<AWorldItem>(
		ClassToSpawn, SpawnTransform, SpawnOwner, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!Spawned)
	{
		return nullptr;
	}

	Spawned->Item = InItem;
	Spawned->Count = FMath::Max(1, InCount);
	Spawned->FinishSpawning(SpawnTransform);
	return Spawned;
}

void AWorldItem::RefreshVisuals()
{
	if (!StaticMeshDisplay || !SkeletalMeshDisplay)
	{
		return;
	}

	UStaticMesh* ResolvedStatic = nullptr;
	USkeletalMesh* ResolvedSkeletal = nullptr;

	if (Item)
	{
		ResolvedSkeletal = Item->SkeletalMeshAsset.LoadSynchronous();
		if (!ResolvedSkeletal)
		{
			ResolvedStatic = Item->StaticMeshAsset.LoadSynchronous();
		}
	}

	const bool bUsingSkeletal = ResolvedSkeletal != nullptr;
	const bool bUsingItemStatic = !bUsingSkeletal && ResolvedStatic != nullptr;

	// SetSkeletalMeshAsset, not SetSkeletalMesh: the latter is the pre-5.1 spelling and the
	// build runs warnings-as-errors, so a deprecation here is a hard failure.
	SkeletalMeshDisplay->SetSkeletalMeshAsset(ResolvedSkeletal);
	SkeletalMeshDisplay->SetVisibility(bUsingSkeletal);

	if (bUsingItemStatic)
	{
		StaticMeshDisplay->SetStaticMesh(ResolvedStatic);
		StaticMeshDisplay->SetRelativeScale3D(MeshRelativeScale);
	}
	else if (!bUsingSkeletal)
	{
		// Materials, and anything not yet given a mesh, show the placeholder block.
		StaticMeshDisplay->SetStaticMesh(PlaceholderMesh);
		StaticMeshDisplay->SetRelativeScale3D(PlaceholderScale);
	}
	else
	{
		StaticMeshDisplay->SetStaticMesh(nullptr);
	}

	StaticMeshDisplay->SetVisibility(!bUsingSkeletal);
	StaticMeshDisplay->SetRelativeRotation(MeshRelativeRotation);
	SkeletalMeshDisplay->SetRelativeRotation(MeshRelativeRotation);
	SkeletalMeshDisplay->SetRelativeScale3D(MeshRelativeScale);

	FLinearColor BlockColor = FLinearColor::White;
	if (Palette && Item)
	{
		BlockColor = Palette->GetBlockColor(Item->Rarity);
	}
	OnItemVisualsApplied(Item, BlockColor);
}

// --- IRPGInteractable --------------------------------------------------------------------- //

EInteractPriority AWorldItem::GetInteractPriority_Implementation() const
{
	return EInteractPriority::Pickup;
}

FText AWorldItem::GetPromptLabel_Implementation() const
{
	if (!Item)
	{
		return FText::GetEmpty();
	}

	if (Count > 1)
	{
		return FText::Format(LOCTEXT("PickUpStack", "Pick Up {0} x{1}"),
			Item->DisplayName, FText::AsNumber(Count));
	}

	return FText::Format(LOCTEXT("PickUpSingle", "Pick Up {0}"), Item->DisplayName);
}

bool AWorldItem::IsInteractEligible_Implementation(APawn* Interactor) const
{
	// Note what is NOT checked here: whether the bag has room. A full bag still shows the
	// prompt and fails loudly on press - hiding it would read as "there is nothing here".
	return Item != nullptr && Count > 0 && Interactor != nullptr
		&& Interactor->FindComponentByClass<UInventoryComponent>() != nullptr;
}

bool AWorldItem::Interact_Implementation(APawn* Interactor)
{
	if (!Item || Count <= 0 || !Interactor)
	{
		return false;
	}

	UInventoryComponent* Inventory = Interactor->FindComponentByClass<UInventoryComponent>();
	if (!Inventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldItem] %s has no UInventoryComponent."),
			*Interactor->GetName());
		return false;
	}

	// AddItem returns how many actually fit. Treating it as a bool is exactly the Unity bug
	// the C++ API was shaped to prevent: a partial add would destroy the pickup and silently
	// eat the remainder.
	const int32 Added = Inventory->AddItem(Item, Count);

	if (Added <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldItem] Bag full - '%s' left on the ground."),
			*Item->DisplayName.ToString());
		return false;
	}

	Count -= Added;

	if (Count > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[WorldItem] Partial pickup of '%s': took %d, %d left behind."),
			*Item->DisplayName.ToString(), Added, Count);
		RefreshVisuals();
		return true;
	}

	Destroy();
	return true;
}

#undef LOCTEXT_NAMESPACE
