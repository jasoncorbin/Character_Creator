#include "Loot/LootDropperComponent.h"

#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interaction/WorldItem.h"
#include "Items/ItemData.h"
#include "Loot/LootTable.h"
#include "TimerManager.h"

ULootDropperComponent::ULootDropperComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULootDropperComponent::DropLoot()
{
	// Guard FIRST - before validating anything. If the table is null we still want the
	// component latched, otherwise a misconfigured enemy stays armed for a second call.
	if (bHasDropped)
	{
		return;
	}
	bHasDropped = true;

	if (!LootTable || !WorldItemClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// An arrow kill runs this entire death chain from inside the projectile's overlap
	// callback. Spawning actors on that stack is the known landmine from the Unity port
	// (spec section 9), so hop out to the next tick and do the work cleanly.
	//
	// The owner survives ~2s after death before DestroyActor, so the margin is wide. If it
	// somehow dies sooner the timer is dropped with the component and nothing spawns, which
	// is the correct failure direction.
	World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &ULootDropperComponent::SpawnRolledLoot));
}

void ULootDropperComponent::SpawnRolledLoot()
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();

	if (!OwnerActor || !World || !LootTable || !WorldItemClass)
	{
		return;
	}

	const TArray<FLootDrop> Drops = LootTable->RollLoot();
	const FVector Origin = OwnerActor->GetActorLocation();

	for (const FLootDrop& Drop : Drops)
	{
		if (!Drop.Item || Drop.Count <= 0)
		{
			continue;
		}

		// Uniform point in a disc. The sqrt matters - without it samples bunch toward the
		// centre and the scatter looks like a pile. Rotating a vector avoids needing PI.
		const float AngleDegrees = FMath::FRandRange(0.0f, 360.0f);
		const float Distance = ScatterRadius * FMath::Sqrt(FMath::FRand());

		const FVector Offset =
			FRotator(0.0f, AngleDegrees, 0.0f).RotateVector(FVector(Distance, 0.0f, 0.0f))
			+ FVector(0.0f, 0.0f, SpawnHeight);

		const FTransform SpawnTransform(FRotator::ZeroRotator, Origin + Offset);

		AWorldItem* Spawned = AWorldItem::SpawnWorldItem(
			this, WorldItemClass, Drop.Item, Drop.Count, SpawnTransform, nullptr);

		if (!Spawned)
		{
			continue;
		}

		// SpawnWorldItem takes neither a palette nor a placeholder mesh, so both land one
		// step late - OnItemVisualsApplied has already fired once with white and no mesh by
		// now. RefreshVisuals below re-runs it with the real values.
		if (Palette)
		{
			Spawned->Palette = Palette;
		}

		UStaticMesh* Placeholder =
			Drop.Item->IsMaterial() ? MaterialPlaceholderMesh : GearPlaceholderMesh;

		if (Placeholder)
		{
			Spawned->PlaceholderMesh = Placeholder;
			Spawned->PlaceholderScale = PlaceholderScale;
		}
		else if (Drop.Item->IsMaterial())
		{
			// Materials have no mesh of their own, so this is not cosmetic - the pickup will
			// be invisible. Loud, because it is otherwise a silent "loot isn't dropping" bug.
			UE_LOG(LogTemp, Warning,
				TEXT("LootDropper on %s: MaterialPlaceholderMesh is unset, so dropped material "
				     "'%s' will be INVISIBLE. Set it on the component."),
				*OwnerActor->GetName(), *Drop.Item->Id.ToString());
		}

		Spawned->RefreshVisuals();
	}
}
