// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

using UnrealBuildTool;
using System.Collections.Generic;

public class PaperGolfEditorTarget : TargetRules
{
	public PaperGolfEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.Add("PaperGolf");
	}
}
