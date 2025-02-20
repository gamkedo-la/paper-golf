// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

using UnrealBuildTool;

public class PGGameplay : ModuleRules
{
	public PGGameplay(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		///////////////////////// Public dependency modules //////////////////////////////////////////////
		// These are modules that have at least one header included in a header file in our public folder.
		// This creates a transitive dependency for header includes, but users of this module still need to specify
		// that dependency in their module file if they use it explicitly (no transitivity in linking).
		var modulePublicDependencyModuleNames = new string[]
		{
			"PGPawn",
        };

		var enginePublicDependencyModuleNames = new string[] 
		{ 
			"Core",
			"CoreUObject",
			"Engine",
		};

		PublicDependencyModuleNames.AddRange(enginePublicDependencyModuleNames);
		PublicDependencyModuleNames.AddRange(modulePublicDependencyModuleNames);

		////////////////////// Private dependency modules ////////////////////////////////////////////////
		// These are modules that our private code depends on but nothing in our public include files depend on.
		// Private dependencies do not create transitive header dependencies.
		var modulePrivateDependencyModuleNames = new string[]
		{
			"PGCore",
		};

		var enginePrivateDependencyModuleNames = new string[] 
		{
			"LevelSequence",
        };

		PrivateDependencyModuleNames.AddRange(enginePrivateDependencyModuleNames);
		PrivateDependencyModuleNames.AddRange(modulePrivateDependencyModuleNames);

		// Following https://forums.unrealengine.com/t/whats-needed-for-using-c-17/124529/2
		// also be sure to refresh the project in the unreal editor: Make sure to close VS and in UE4 do File -> Refresh Visual Studio project Files after this change
		CppStandard = CppStandardVersion.Cpp20;
	}
}
