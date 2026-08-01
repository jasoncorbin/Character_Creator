// Single source of rarity colour for every view. Spec section 3.
// Widgets must read through this - never hard-code hex, so the palette swap stays one field.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/RPGItemTypes.h"
#include "RarityPalette.generated.h"

/** Which authored palette is live. Candy Warm is the chosen set; Classic Bright matches the mock PNGs. */
UENUM(BlueprintType)
enum class EPaletteSet : uint8
{
	CandyWarm      UMETA(DisplayName = "Candy Warm"),
	ClassicBright  UMETA(DisplayName = "Classic Bright")
};

UCLASS(BlueprintType)
class CHARACTER_CREATOR_API URarityPalette : public UDataAsset
{
	GENERATED_BODY()

public:
	/** The one-field swap. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	EPaletteSet ActiveSet = EPaletteSet::CandyWarm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	TMap<EItemRarity, FRarityColors> CandyWarm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	TMap<EItemRarity, FRarityColors> ClassicBright;

	// --- non-rarity states ---------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot States")
	FLinearColor EmptyMain = FLinearColor::Gray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot States")
	FLinearColor EmptySoft = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot States")
	FLinearColor LockedMain = FLinearColor::Gray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot States")
	FLinearColor LockedSoft = FLinearColor::White;

	/** Flat tints for the 3D placeholder loot blocks - deliberately NOT the UI palette. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot Blocks")
	TMap<EItemRarity, FLinearColor> BlockColors;

	/** Every view goes through this. */
	UFUNCTION(BlueprintPure, Category = "Palette")
	FRarityColors GetColors(EItemRarity Rarity) const;

	UFUNCTION(BlueprintPure, Category = "Palette")
	FLinearColor GetBlockColor(EItemRarity Rarity) const;

	/** Restores both palettes to the approved design hexes. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Palette")
	void ResetToDesignDefaults();

	URarityPalette();
};
