#include "Items/ItemInstance.h"
#include "Items/ItemData.h"

UItemInstance* UItemInstance::Create(UObject* Outer, UItemData* InTemplate)
{
	if (!InTemplate)
	{
		return nullptr;
	}

	UItemInstance* Instance = NewObject<UItemInstance>(Outer ? Outer : GetTransientPackage());
	Instance->Template = InTemplate;
	Instance->Level = 0;
	Instance->BaseDamage = InTemplate->Damage;
	Instance->DamagePerLevel = 0; // authored per-weapon once the forge lands (spec section 8)
	return Instance;
}

int32 UItemInstance::GetDamage() const
{
	if (!Template)
	{
		return 0;
	}

	// Until the forge lands Level is always 0, so this reduces to the template's damage.
	// When upgrades arrive, ONLY this line changes - every consumer already reads through here.
	return BaseDamage + (Level * DamagePerLevel);
}

EItemRarity UItemInstance::GetRarity() const
{
	// Post-forge this becomes a function of Level against tunable thresholds.
	return Template ? Template->Rarity : EItemRarity::Common;
}

FText UItemInstance::GetDisplayName() const
{
	return Template ? Template->DisplayName : FText::GetEmpty();
}

EEquipSlot UItemInstance::GetSlot() const
{
	return Template ? Template->Slot : EEquipSlot::Melee;
}

EWeaponCategory UItemInstance::GetCategory() const
{
	return Template ? Template->Category : EWeaponCategory::Unarmed;
}
