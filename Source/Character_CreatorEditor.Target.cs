// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Character_CreatorEditorTarget : TargetRules
{
	public Character_CreatorEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		// V7 = UE 5.8 defaults. V6 is rejected outright on 5.8 for targets that share build
		// products with the installed engine, because V7 flips ReturnType/Dangling/UnreachableCode
		// warnings from Off to Error and the engine was built with them on.
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("Character_Creator");
	}
}
