// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "Debug/PGConsoleVars.h"

#if PG_DEBUG_ENABLED

namespace
{
	void ResetGameModeOverridesConsoleCommand(const TArray<FString>& Args);

	TArray<FString> ParseArgs(const TArray<FString>& Args);

	FAutoConsoleCommand CResetGameModeOverrides(
		TEXT("pg.mode.resetVars"),
		TEXT("Resets all the console variable overrides on the pg.mode namespace"),
		FConsoleCommandWithArgsDelegate::CreateStatic(ResetGameModeOverridesConsoleCommand), ECVF_Default
	);
}

namespace PG
{
	TAutoConsoleVariable<bool> CShowForces(
		TEXT("pg.showForces"),
		false,
		TEXT("Toggle on/off"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	TAutoConsoleVariable<bool> CAutomaticVisualLoggerRecording(
		TEXT("pg.vislog.autorecord"),
		true,
		TEXT("Toggle on/off"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CStartHoleOverride(
		TEXT("pg.startHole"),
		-1,
		TEXT("Override the start hole number"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CPlayerAccuracyExponent(
		TEXT("pg.diff.pAccExp"),
		-1.0f,
		TEXT("Adjust client player shot accuracy exponent >= 1"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CPlayerMaxAccuracy(
		TEXT("pg.diff.pAccMax"),
		-1.0f,
		TEXT("Adjust client player worst case shot accuracy [0,1]. Higher is harder"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CGlobalPowerAccuracyExponent(
		TEXT("pg.diff.gAccExp"),
		-1.0f,
		TEXT("Adjust all players' shot accuracy exponent >= 1"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CGlobalPowerAccuracyDampenExponent(
		TEXT("pg.diff.gPowExp"),
		-1.0f,
		TEXT("Adjust all players' accuracy effect on shot power reduction >= 0. Larger is more difficult"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CGlobalMinPowerMultiplier(
		TEXT("pg.diff.gPowMin"),
		-1.0f,
		TEXT("Adjust all players' min power reduction from accuracy [0,1]"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	namespace GameMode
	{
		// These only apply before starting the game and are overriden by the game options url string

		TAutoConsoleVariable<int32> CAllowBots(
			TEXT("pg.mode.allowBots"),
			-1,
			TEXT("Override the game mode default allow bots settings -> -1: variable disabled, 0: don't allow bots, 1: allow bots"),
			ECVF_Scalability | ECVF_RenderThreadSafe
		);

		TAutoConsoleVariable<int32> CSkipHumanPlayers(
			TEXT("pg.mode.skipHumans"),
			-1,
			TEXT("Override the game mode default to skip human players -> -1: variable disabled, 0: don't skip all human players, 1: skip all human players"),
			ECVF_Scalability | ECVF_RenderThreadSafe
		);

		TAutoConsoleVariable<int32> CNumDesiredBots(
			TEXT("pg.mode.numBots"),
			-1,
			TEXT("Override the game mode default num bots settings -> -1: variable disabled, 0: don't allow bots, 1: allow bots"),
			ECVF_Scalability | ECVF_RenderThreadSafe
		);

		TAutoConsoleVariable<int32> CNumDesiredPlayers(
			TEXT("pg.mode.numPlayers"),
			-1,
			TEXT("Override the game mode default num players settings -> -1: variable disabled, game starts when numBots + numPlayers join"),
			ECVF_Scalability | ECVF_RenderThreadSafe
		);

		TAutoConsoleVariable<int32> CMinTotalPlayers(
			TEXT("pg.mode.minPlayers"),
			-1,
			TEXT("Override the game mode default min total players setting -> -1: variable disabled. If the players in game falls below this number, the game ends"),
			ECVF_Scalability | ECVF_RenderThreadSafe
		);

		TAutoConsoleVariable<int32> CMaxTotalPlayers(
			TEXT("pg.mode.maxPlayers"),
			-1,
			TEXT("Override the game mode default max total players setting -> -1: variable disabled. Total player count cannot exceed this number"),
			ECVF_Scalability | ECVF_RenderThreadSafe
		);
	}
}

namespace
{
	void ResetGameModeOverridesConsoleCommand(const TArray<FString>& Args)
	{
		PG::GameMode::CAllowBots->Set(-1, EConsoleVariableFlags::ECVF_SetByConsole);
		PG::GameMode::CSkipHumanPlayers->Set(-1, EConsoleVariableFlags::ECVF_SetByConsole);
		PG::GameMode::CNumDesiredBots->Set(-1, EConsoleVariableFlags::ECVF_SetByConsole);
		PG::GameMode::CNumDesiredPlayers->Set(-1, EConsoleVariableFlags::ECVF_SetByConsole);
		PG::GameMode::CMinTotalPlayers->Set(-1, EConsoleVariableFlags::ECVF_SetByConsole);
		PG::GameMode::CMaxTotalPlayers->Set(-1, EConsoleVariableFlags::ECVF_SetByConsole);
	}

	TArray<FString> ParseArgs(const TArray<FString>& Args)
	{
		if (Args.Num() != 1)
		{
			return Args;
		}

		TArray<FString> OutArray;

		// When reading multiple arguments from console they come in as a single comma delimited string
		constexpr const TCHAR* Delimiter = TEXT(",");
		const FString& SArgs = Args[0];

		SArgs.ParseIntoArray(OutArray, Delimiter);

		// trim all arguments
		for (FString& Arg : OutArray)
		{
			Arg.TrimStartAndEndInline();
		}

		return OutArray;
	}
}

#endif
