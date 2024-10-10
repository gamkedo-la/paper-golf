// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

using UnrealBuildTool;

public class PaperGolf : ModuleRules
{
	public PaperGolf(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    
        PublicDependencyModuleNames.AddRange(new string[] 
        { 
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[] 
        {
            "PGCore",
            "PGGameplay",
            "PGPlayer",
            "PGAI",
            "PGUI",
            "PGPawn",
            "MultiplayerSessions",
            "PGSettings",

            // Engine modules
            "AIModule",
            "MoviePlayer", // Loading Screen

            // Controller detection support in TRGameInstance.cpp
            "Slate",
            "SlateCore",
            "ApplicationCore",
            "CoreOnline" // FUniqueNetIdWrapper in PreLogin of GameMode
        });

        CppStandard = CppStandardVersion.Cpp20;

        // Editor build
        if (Target.Type == TargetRules.TargetType.Editor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] {
                "UnrealEd",
                "PaperGolfEditor",
            });
        }
    }
}
