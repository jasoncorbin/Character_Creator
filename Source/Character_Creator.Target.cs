// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Character_CreatorTarget : TargetRules
{
	public Character_CreatorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		// V7 = UE 5.8 defaults. See the Editor target for why V6 no longer builds on 5.8.
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("Character_Creator");
	}
}
