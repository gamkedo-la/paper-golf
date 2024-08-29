// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

using UnrealBuildTool;
using System.Collections.Generic;

public class PaperGolfTarget : TargetRules
{
	public PaperGolfTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("PaperGolf");
	}
}
