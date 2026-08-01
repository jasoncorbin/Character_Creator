#include "Items/RarityPalette.h"

namespace
{
	/** Design hexes are authored in sRGB; UE stores linear. Convert explicitly. */
	FLinearColor Hex(const TCHAR* InHex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(InHex));
	}

	FRarityColors Set(const TCHAR* Main, const TCHAR* Soft, const TCHAR* Text)
	{
		return FRarityColors(Hex(Main), Hex(Soft), Hex(Text));
	}
}

URarityPalette::URarityPalette()
{
	ResetToDesignDefaults();
}

void URarityPalette::ResetToDesignDefaults()
{
	// --- Candy Warm (chosen) ---
	CandyWarm.Empty();
	CandyWarm.Add(EItemRarity::Common,    Set(TEXT("A99B86"), TEXT("F3EFE8"), TEXT("7C7060")));
	CandyWarm.Add(EItemRarity::Uncommon,  Set(TEXT("54C97E"), TEXT("E6F8ED"), TEXT("2F9455")));
	CandyWarm.Add(EItemRarity::Rare,      Set(TEXT("9B6BE6"), TEXT("F0E9FB"), TEXT("6E42C0")));
	CandyWarm.Add(EItemRarity::Legendary, Set(TEXT("FF9A3D"), TEXT("FFEEDC"), TEXT("D26A0E")));

	// --- Classic Bright (fallback; matches the exported reference PNGs) ---
	ClassicBright.Empty();
	ClassicBright.Add(EItemRarity::Common,    Set(TEXT("7C8698"), TEXT("EEF1F5"), TEXT("5A6474")));
	ClassicBright.Add(EItemRarity::Uncommon,  Set(TEXT("43B55F"), TEXT("E7F6EC"), TEXT("2E7D46")));
	ClassicBright.Add(EItemRarity::Rare,      Set(TEXT("2E90E0"), TEXT("E4F1FC"), TEXT("1E6FB8")));
	ClassicBright.Add(EItemRarity::Legendary, Set(TEXT("F0A32E"), TEXT("FDF1DC"), TEXT("B9740A")));

	EmptyMain  = Hex(TEXT("DBD5C9"));
	EmptySoft  = Hex(TEXT("F1EDE4"));
	LockedMain = Hex(TEXT("D9D3C6"));
	LockedSoft = Hex(TEXT("F3EFE7"));

	// Flat linear values straight from the spec - these are NOT sRGB hexes.
	BlockColors.Empty();
	BlockColors.Add(EItemRarity::Common,    FLinearColor(0.85f, 0.85f, 0.85f, 1.0f));
	BlockColors.Add(EItemRarity::Uncommon,  FLinearColor(0.35f, 0.82f, 0.38f, 1.0f));
	BlockColors.Add(EItemRarity::Rare,      FLinearColor(0.34f, 0.58f, 1.00f, 1.0f));
	BlockColors.Add(EItemRarity::Legendary, FLinearColor(1.00f, 0.74f, 0.20f, 1.0f));
}

FRarityColors URarityPalette::GetColors(EItemRarity Rarity) const
{
	const TMap<EItemRarity, FRarityColors>& Active =
		(ActiveSet == EPaletteSet::ClassicBright) ? ClassicBright : CandyWarm;

	if (const FRarityColors* Found = Active.Find(Rarity))
	{
		return *Found;
	}
	return FRarityColors();
}

FLinearColor URarityPalette::GetBlockColor(EItemRarity Rarity) const
{
	if (const FLinearColor* Found = BlockColors.Find(Rarity))
	{
		return *Found;
	}
	return FLinearColor::White;
}
