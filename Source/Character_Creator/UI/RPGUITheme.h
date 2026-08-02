// Step 6A - the UI style foundation.
//
// URarityPalette owns colour that varies BY RARITY and swaps with the palette set.
// This asset owns everything that does NOT: screen chrome, type scale, and layout metrics.
// The two are deliberately separate - Candy Warm vs Classic Bright changes rarity colour only,
// while the Candy Cloud chrome is shared by both.
//
// Widgets must never hard-code a hex, a font, or a spacing number. They go through
// URPGUIStyle, which resolves this asset. Values are the design handoff's, converted
// sRGB -> linear in code (see RarityPalette.cpp for the same convention).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Fonts/SlateFontInfo.h"
#include "RPGUITheme.generated.h"

/**
 * Every distinct text treatment in the inventory/forge screens, from the design handoff.
 * A role carries font + size + colour together so a widget asks once and gets a complete answer.
 */
UENUM(BlueprintType)
enum class ERPGTextRole : uint8
{
	ScreenTitle    UMETA(DisplayName = "Screen Title"),      // Fredoka 22, indigo
	PanelHeader    UMETA(DisplayName = "Panel Header"),      // Fredoka 17, indigo
	StatValue      UMETA(DisplayName = "Stat Value"),        // Fredoka 20, per-stat colour
	DetailValue    UMETA(DisplayName = "Detail Value"),      // Fredoka 18, damage orange
	PillValue      UMETA(DisplayName = "Currency Pill"),     // Nunito 13, pill brown
	ItemName       UMETA(DisplayName = "Item Name"),         // Nunito 13, body ink
	StackBadge     UMETA(DisplayName = "Stack Badge"),       // Nunito 11, white on dark
	FooterLabel    UMETA(DisplayName = "Footer Label"),      // Nunito 12, muted
	ButtonLabel    UMETA(DisplayName = "Button Label"),      // Nunito 13, white
	TabLabel       UMETA(DisplayName = "Tab Label"),         // Nunito 12
	RarityLabel    UMETA(DisplayName = "Rarity Label"),      // Nunito 10, rarity text colour
	StatLabel      UMETA(DisplayName = "Stat Label"),        // Nunito 8, muted, uppercase
	SlotLabel      UMETA(DisplayName = "Slot Label"),        // Nunito 7, muted

	Count          UMETA(Hidden)
};

/** A complete text treatment: which face, how big, what colour. */
USTRUCT(BlueprintType)
struct CHARACTER_CREATOR_API FRPGTextStyle
{
	GENERATED_BODY()

	/** A UFontFace asset. FSlateFontInfo accepts a face directly - we do NOT build UFont assets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text",
		meta = (AllowedClasses = "/Script/Engine.FontFace"))
	TObjectPtr<UObject> FontFace = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	int32 Size = 12;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	FLinearColor Color = FLinearColor::Black;

	/** The design sets some labels in caps; kept as data so widgets don't branch on role. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	bool bUppercase = false;
};

/** Screen chrome - everything that is not rarity-driven. */
USTRUCT(BlueprintType)
struct CHARACTER_CREATOR_API FRPGChrome
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Panels")
	FLinearColor PanelBg = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Panels")
	FLinearColor ScreenInk = FLinearColor::Black;      // #4B57C9 title indigo

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Panels")
	FLinearColor BodyInk = FLinearColor::Black;        // #2A2A2E

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Panels")
	FLinearColor MutedInk = FLinearColor::Gray;        // #8A8172

	// --- currency pill ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pill")
	FLinearColor PillBg = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pill")
	FLinearColor PillBorder = FLinearColor::Gray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pill")
	FLinearColor PillText = FLinearColor::Black;

	// --- close button ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Close")
	FLinearColor CloseBg = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Close")
	FLinearColor CloseBorder = FLinearColor::Gray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Close")
	FLinearColor CloseGlyph = FLinearColor::Black;

	// --- tabs ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tabs")
	FLinearColor TabActiveBg = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tabs")
	FLinearColor TabActiveText = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tabs")
	FLinearColor TabInactiveBg = FLinearColor::Gray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tabs")
	FLinearColor TabInactiveText = FLinearColor::Black;

	// --- chunky primary button (vertical gradient + hard bottom edge) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button")
	FLinearColor ButtonTop = FLinearColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button")
	FLinearColor ButtonBottom = FLinearColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button")
	FLinearColor ButtonText = FLinearColor::White;

	/** The 3D lip under the button - design uses 0 4px 0 rgba(0,0,0,.12). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button")
	FLinearColor ButtonEdge = FLinearColor::Black;

	// --- inset surfaces (stat cells, footer) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surfaces")
	FLinearColor InsetBg = FLinearColor::Gray;

	// --- per-stat accents ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	FLinearColor StatDamage = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	FLinearColor StatArmor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	FLinearColor StatHealth = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	FLinearColor StatStamina = FLinearColor::Black;

	// --- selection ring on the chosen grid cell ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selection")
	FLinearColor SelectionOuter = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selection")
	FLinearColor SelectionInner = FLinearColor::White;
};

/** Spacing and sizing, so no widget carries a magic number either. */
USTRUCT(BlueprintType)
struct CHARACTER_CREATOR_API FRPGMetrics
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	FVector2D ReferenceCanvas = FVector2D(980.0, 600.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	float PanelGap = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	float GridGap = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	float SlotColumnGap = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	float SlotSize = 58.0f;

	/** Left Character panel : right Items Bag panel. Design is 1.25 : 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	float CharacterPanelFlex = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	int32 GridColumns = 5;

	/** The grid pads out to this many cells so a near-empty bag still reads as a grid. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	int32 MinGridCells = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radii")
	float RadiusPanel = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radii")
	float RadiusCell = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radii")
	float RadiusSlot = 14.0f;

	/** Accessibility floor from the design handoff - interactive elements must not go below this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	float MinTouchTarget = 44.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Borders")
	float BorderThin = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Borders")
	float BorderRarity = 2.0f;
};

/**
 * The non-rarity half of the design system. One asset, edited in the editor,
 * with a button to restore the authored design values.
 */
UCLASS(BlueprintType)
class CHARACTER_CREATOR_API URPGUITheme : public UDataAsset
{
	GENERATED_BODY()

public:
	URPGUITheme();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme")
	FRPGChrome Chrome;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme")
	FRPGMetrics Metrics;

	/** Indexed by ERPGTextRole. Sized to ERPGTextRole::Count by ResetToDesignDefaults. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme")
	TArray<FRPGTextStyle> TextStyles;

	/** Fredoka - the display face. Assign /Game/RPG/UI/Fonts/Fredoka_*. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fonts",
		meta = (AllowedClasses = "/Script/Engine.FontFace"))
	TObjectPtr<UObject> DisplayFont = nullptr;

	/** Nunito - the body face. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fonts",
		meta = (AllowedClasses = "/Script/Engine.FontFace"))
	TObjectPtr<UObject> BodyFont = nullptr;

	UFUNCTION(BlueprintPure, Category = "Theme")
	FRPGTextStyle GetTextStyle(ERPGTextRole Role) const;

	/** Ready-to-bind FSlateFontInfo for a role. Falls back to the face on the style entry. */
	UFUNCTION(BlueprintPure, Category = "Theme")
	FSlateFontInfo GetFont(ERPGTextRole Role) const;

	/** Restores the authored design values. Fonts are NOT touched - those are asset refs. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Theme")
	void ResetToDesignDefaults();
};
