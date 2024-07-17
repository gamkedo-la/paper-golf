// Copyright Game Salutes. All Rights Reserved.

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

            // Engine modules
            "AIModule",
        });

        CppStandard = CppStandardVersion.Cpp20;

        // Editor build
        if (Target.Type == TargetRules.TargetType.Editor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] {
                "UnrealEd",
            });
        }
    }
}
