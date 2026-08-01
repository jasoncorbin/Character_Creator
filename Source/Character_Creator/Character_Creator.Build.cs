// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Character_Creator : ModuleRules
{
	public Character_Creator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UMG",
			"Slate",
			"SlateCore",
		});

		// No Public/Private split in this module, so UBT already treats the module
		// directory as an include root - "Items/ItemData.h" resolves without extra paths.
	}
}
