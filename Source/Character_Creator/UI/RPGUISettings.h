// Where the UI style assets are found.
//
// Before this, URarityPalette was only reachable as a per-actor TObjectPtr that had to be
// set by hand on every actor - which is exactly why AWorldItem::Palette was left unset on
// all four placed pickups and their rarity tint came out white. A project-level setting
// gives every consumer one place to resolve from, UI and gameplay alike.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RPGUISettings.generated.h"

class URarityPalette;
class URPGUITheme;

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "RPG UI"))
class CHARACTER_CREATOR_API URPGUISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Rarity colour, and the Candy Warm / Classic Bright swap. */
	UPROPERTY(config, EditAnywhere, Category = "Assets")
	TSoftObjectPtr<URarityPalette> RarityPalette;

	/** Chrome, type scale and layout metrics. */
	UPROPERTY(config, EditAnywhere, Category = "Assets")
	TSoftObjectPtr<URPGUITheme> Theme;

	virtual FName GetCategoryName() const override;

	static const URPGUISettings* Get();
};
