// The one API every widget in steps 6-7 talks to.
//
// THE RULE: no widget contains a hex literal, a font reference, or a spacing number.
// It calls in here, and the answer comes from DA_RarityPalette (rarity) or DA_RPGUITheme
// (chrome/type/metrics), resolved through project settings. That is what keeps the
// palette swap a one-field change.
//
// Everything is BlueprintPure and null-safe: if an asset is unassigned you get a loud,
// obviously-wrong magenta rather than a crash or a silent white, so a missing setting is
// visible on screen in the first PIE run instead of being mistaken for a design choice.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Fonts/SlateFontInfo.h"
#include "Items/RPGItemTypes.h"
#include "UI/RPGUITheme.h"
#include "RPGUIStyle.generated.h"

class URarityPalette;

/** Non-rarity cell/slot states. Rarity states come from EItemRarity instead. */
UENUM(BlueprintType)
enum class ERPGSlotState : uint8
{
	Empty   UMETA(DisplayName = "Empty"),
	Locked  UMETA(DisplayName = "Locked")
};

/**
 * The four cells in the stats row.
 *
 * Deliberately NOT EEquipSlot: only Damage and Armor have anything to do with a slot, and
 * reusing the slot enum here would read as "the off-hand slot means health".
 *
 * NB as of step 6, only Damage has a real source (ARPGPlayerCharacter::GetMeleeDamage).
 * Armor/Health/Stamina render placeholder values - the RPG player has no stats layer yet,
 * and UItemData carries no armour value. See the plan's step-6 notes.
 */
UENUM(BlueprintType)
enum class ERPGStat : uint8
{
	Damage   UMETA(DisplayName = "Damage"),
	Armor    UMETA(DisplayName = "Armor"),
	Health   UMETA(DisplayName = "Health"),
	Stamina  UMETA(DisplayName = "Stamina"),

	Count    UMETA(Hidden)
};

UCLASS()
class CHARACTER_CREATOR_API URPGUIStyle : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The live palette, or null if the project setting is unassigned. */
	UFUNCTION(BlueprintPure, Category = "RPG UI")
	static URarityPalette* GetPalette();

	/** The live theme, or null if the project setting is unassigned. */
	UFUNCTION(BlueprintPure, Category = "RPG UI")
	static URPGUITheme* GetTheme();

	/** Border / soft-fill / text colours for a rarity. Magenta if the palette is missing. */
	UFUNCTION(BlueprintPure, Category = "RPG UI")
	static FRarityColors GetRarityColors(EItemRarity Rarity);

	/** The same triple for a non-rarity cell - an empty bag slot or a locked equipment slot. */
	UFUNCTION(BlueprintPure, Category = "RPG UI")
	static FRarityColors GetSlotStateColors(ERPGSlotState State);

	UFUNCTION(BlueprintPure, Category = "RPG UI")
	static FRPGChrome GetChrome();

	UFUNCTION(BlueprintPure, Category = "RPG UI")
	static FRPGMetrics GetMetrics();

	UFUNCTION(BlueprintPure, Category = "RPG UI")
	static FRPGTextStyle GetTextStyle(ERPGTextRole Role);

	/** Ready to bind straight onto a TextBlock's Font. */
	UFUNCTION(BlueprintPure, Category = "RPG UI")
	static FSlateFontInfo GetFont(ERPGTextRole Role);

	/** Accent colour for one of the four stat cells. */
	UFUNCTION(BlueprintPure, Category = "RPG UI")
	static FLinearColor GetStatColor(ERPGStat Stat);

	/** True when both style assets resolve - worth asserting once on screen open. */
	UFUNCTION(BlueprintPure, Category = "RPG UI")
	static bool IsStyleConfigured();

private:
	/** Deliberately hideous: an unassigned setting must not look like a design decision. */
	static FRarityColors MissingColors();
};
