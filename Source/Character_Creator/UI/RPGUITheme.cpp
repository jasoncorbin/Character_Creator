#include "UI/RPGUITheme.h"

namespace
{
	/** Design hexes are authored in sRGB; UE stores linear. Same convention as RarityPalette.cpp. */
	FLinearColor Hex(const TCHAR* InHex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(InHex));
	}

	/** The design's translucent overlays are rgba(0,0,0,a) over white - keep the alpha, not a blend. */
	FLinearColor Ink(float Alpha)
	{
		return FLinearColor(0.0f, 0.0f, 0.0f, Alpha);
	}
}

URPGUITheme::URPGUITheme()
{
	ResetToDesignDefaults();
}

void URPGUITheme::ResetToDesignDefaults()
{
	// --- chrome ------------------------------------------------------------
	Chrome.PanelBg   = Hex(TEXT("FFFFFF"));
	Chrome.ScreenInk = Hex(TEXT("4B57C9"));
	Chrome.BodyInk   = Hex(TEXT("2A2A2E"));
	Chrome.MutedInk  = Hex(TEXT("8A8172"));

	Chrome.PillBg     = Hex(TEXT("FDF1DC"));
	Chrome.PillBorder = Hex(TEXT("F0A32E"));
	Chrome.PillText   = Hex(TEXT("B9740A"));

	Chrome.CloseBg     = Hex(TEXT("F5E3E3"));
	Chrome.CloseBorder = Hex(TEXT("E3A6A6"));
	Chrome.CloseGlyph  = Hex(TEXT("C05A5A"));

	Chrome.TabActiveBg     = Hex(TEXT("2E2A26"));
	Chrome.TabActiveText   = Hex(TEXT("FFFFFF"));
	Chrome.TabInactiveBg   = Ink(0.05f);
	Chrome.TabInactiveText = Hex(TEXT("7A7062"));

	Chrome.ButtonTop    = Hex(TEXT("5BD08A"));
	Chrome.ButtonBottom = Hex(TEXT("37B56B"));
	Chrome.ButtonText   = Hex(TEXT("FFFFFF"));
	Chrome.ButtonEdge   = Ink(0.12f);

	Chrome.InsetBg = Ink(0.035f);

	Chrome.StatDamage  = Hex(TEXT("E0872E"));
	Chrome.StatArmor   = Hex(TEXT("2E90E0"));
	Chrome.StatHealth  = Hex(TEXT("E0524E"));
	Chrome.StatStamina = Hex(TEXT("43B55F"));

	Chrome.SelectionOuter = Hex(TEXT("2A2A2E"));
	Chrome.SelectionInner = Hex(TEXT("FFFFFF"));

	// --- metrics -----------------------------------------------------------
	Metrics = FRPGMetrics();

	// --- type scale --------------------------------------------------------
	// Font faces are left null here on purpose: ResetToDesignDefaults must not clobber
	// asset references. GetFont() falls back to DisplayFont/BodyFont when a role's face
	// is unset, so assigning the two family fields is enough to make every role render.
	const int32 RoleCount = static_cast<int32>(ERPGTextRole::Count);
	TextStyles.SetNum(RoleCount);

	auto Set = [this](ERPGTextRole Role, int32 Size, const FLinearColor& Color, bool bUpper = false)
	{
		FRPGTextStyle& Style = TextStyles[static_cast<int32>(Role)];
		Style.Size = Size;
		Style.Color = Color;
		Style.bUppercase = bUpper;
	};

	Set(ERPGTextRole::ScreenTitle, 22, Chrome.ScreenInk);
	Set(ERPGTextRole::PanelHeader, 17, Chrome.ScreenInk);
	Set(ERPGTextRole::StatValue,   20, Chrome.BodyInk);        // per-stat colour applied by the widget
	Set(ERPGTextRole::DetailValue, 18, Chrome.StatDamage);
	Set(ERPGTextRole::PillValue,   13, Chrome.PillText);
	Set(ERPGTextRole::ItemName,    13, Chrome.BodyInk);
	Set(ERPGTextRole::StackBadge,  11, Hex(TEXT("FFFFFF")));
	Set(ERPGTextRole::FooterLabel, 12, Hex(TEXT("7A7062")));
	Set(ERPGTextRole::ButtonLabel, 13, Chrome.ButtonText);
	Set(ERPGTextRole::TabLabel,    12, Chrome.TabInactiveText);
	Set(ERPGTextRole::RarityLabel, 10, Chrome.BodyInk);        // rarity text colour applied by the widget
	Set(ERPGTextRole::StatLabel,    8, Chrome.MutedInk, true);
	Set(ERPGTextRole::SlotLabel,    7, Chrome.MutedInk, true);
}

FRPGTextStyle URPGUITheme::GetTextStyle(ERPGTextRole Role) const
{
	const int32 Index = static_cast<int32>(Role);
	if (TextStyles.IsValidIndex(Index))
	{
		FRPGTextStyle Style = TextStyles[Index];
		if (Style.FontFace == nullptr)
		{
			// Roles above PillValue are the display face; the rest are body. Keeping this
			// here rather than in data means adding a role can't silently render fontless.
			Style.FontFace = (Role <= ERPGTextRole::DetailValue) ? DisplayFont : BodyFont;
		}
		return Style;
	}
	return FRPGTextStyle();
}

FSlateFontInfo URPGUITheme::GetFont(ERPGTextRole Role) const
{
	const FRPGTextStyle Style = GetTextStyle(Role);

	FSlateFontInfo Info;
	Info.FontObject = Style.FontFace;
	Info.Size = Style.Size;
	return Info;
}
