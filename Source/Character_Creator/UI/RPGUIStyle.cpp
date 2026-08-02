#include "UI/RPGUIStyle.h"

#include "Items/RarityPalette.h"
#include "UI/RPGUISettings.h"

FRarityColors URPGUIStyle::MissingColors()
{
	// Magenta on magenta - unmissable in PIE, and nothing in the Candy Cloud palette is close.
	return FRarityColors(FLinearColor(1.0f, 0.0f, 1.0f, 1.0f),
	                     FLinearColor(1.0f, 0.0f, 1.0f, 0.35f),
	                     FLinearColor(1.0f, 0.0f, 1.0f, 1.0f));
}

URarityPalette* URPGUIStyle::GetPalette()
{
	const URPGUISettings* Settings = URPGUISettings::Get();
	if (Settings == nullptr)
	{
		return nullptr;
	}
	// LoadSynchronous is right here: the screen is opened on demand and the asset is tiny.
	return Settings->RarityPalette.LoadSynchronous();
}

URPGUITheme* URPGUIStyle::GetTheme()
{
	const URPGUISettings* Settings = URPGUISettings::Get();
	if (Settings == nullptr)
	{
		return nullptr;
	}
	return Settings->Theme.LoadSynchronous();
}

bool URPGUIStyle::IsStyleConfigured()
{
	return GetPalette() != nullptr && GetTheme() != nullptr;
}

FRarityColors URPGUIStyle::GetRarityColors(EItemRarity Rarity)
{
	if (const URarityPalette* Palette = GetPalette())
	{
		return Palette->GetColors(Rarity);
	}
	return MissingColors();
}

FRarityColors URPGUIStyle::GetSlotStateColors(ERPGSlotState State)
{
	const URarityPalette* Palette = GetPalette();
	if (Palette == nullptr)
	{
		return MissingColors();
	}

	// Text colour reuses the muted ink so an empty slot's label matches every other label.
	const FLinearColor Text = GetChrome().MutedInk;

	return (State == ERPGSlotState::Locked)
		? FRarityColors(Palette->LockedMain, Palette->LockedSoft, Text)
		: FRarityColors(Palette->EmptyMain, Palette->EmptySoft, Text);
}

FRPGChrome URPGUIStyle::GetChrome()
{
	if (const URPGUITheme* Theme = GetTheme())
	{
		return Theme->Chrome;
	}
	return FRPGChrome();
}

FRPGMetrics URPGUIStyle::GetMetrics()
{
	if (const URPGUITheme* Theme = GetTheme())
	{
		return Theme->Metrics;
	}
	// The struct's own defaults ARE the design metrics, so layout still lands correctly
	// even with no theme assigned. Only colour goes magenta.
	return FRPGMetrics();
}

FRPGTextStyle URPGUIStyle::GetTextStyle(ERPGTextRole Role)
{
	if (const URPGUITheme* Theme = GetTheme())
	{
		return Theme->GetTextStyle(Role);
	}
	return FRPGTextStyle();
}

FSlateFontInfo URPGUIStyle::GetFont(ERPGTextRole Role)
{
	if (const URPGUITheme* Theme = GetTheme())
	{
		return Theme->GetFont(Role);
	}
	return FSlateFontInfo();
}

FLinearColor URPGUIStyle::GetStatColor(ERPGStat Stat)
{
	const FRPGChrome Chrome = GetChrome();
	switch (Stat)
	{
	case ERPGStat::Damage:  return Chrome.StatDamage;
	case ERPGStat::Armor:   return Chrome.StatArmor;
	case ERPGStat::Health:  return Chrome.StatHealth;
	case ERPGStat::Stamina: return Chrome.StatStamina;
	default:                return Chrome.BodyInk;
	}
}
