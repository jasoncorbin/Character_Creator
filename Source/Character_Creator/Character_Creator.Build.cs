// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Character_Creator : ModuleRules
{
	public Character_Creator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// This module has no Public/Private split - sources sit in Items/ and Inventory/
		// directly under the module root. UBT does NOT put the module root on the include
		// path by itself under BuildSettingsVersion.V6 (which turns off the legacy
		// public/parent include-path behaviour), so "Items/ItemData.h" only resolves
		// because of this line. Removing it breaks every cross-folder include.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			// Public because UI/RPGUITheme.h exposes FSlateFontInfo in its public API, and
			// UI/RPGUISettings.h derives from UDeveloperSettings. A module including ours
			// needs both - keeping them private breaks any downstream include.
			"SlateCore",
			"DeveloperSettings",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UMG",
			"Slate",
		});
	}
}
