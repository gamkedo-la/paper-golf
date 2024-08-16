// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "GameMode/LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"
#include "MultiplayerSessionsSubsystem.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PaperGolfLogging.h"

#include "GameMode/PaperGolfGameModeBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LobbyGameMode)


void ALobbyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitGame - %s"), *GetName(), *MapName);

	Super::InitGame(MapName, Options, ErrorMessage);

#if !UE_BUILD_SHIPPING
	ValidateMaps();
	ValidateGameModes();
#endif
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: PostLogin - %s"), *GetName(), *LoggingUtils::GetName(NewPlayer));

	if(!ensure(!Maps.IsEmpty()) || !ensure(!MatchTypesToModes.IsEmpty()))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: Maps or MatchTypesToModes is empty: MapsSize=%d; MatchTypesToModesSize=%d"),
			*GetName(), Maps.Num(), MatchTypesToModes.Num());
		return;
	}

	const auto NumberOfPlayers = GameState.Get()->PlayerArray.Num();

	UGameInstance* GameInstance = GetGameInstance();
	if (!ensure(GameInstance))
	{
		return;
	}

	auto Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	if (!ensure(Subsystem))
	{
		return;
	}

	if (NumberOfPlayers == Subsystem->GetDesiredNumPublicConnections())
	{
		auto World = GetWorld();
		if(!ensure(World))
		{
			return;
		}

		bUseSeamlessTravel = !WITH_EDITOR;

		const auto& MatchType = Subsystem->GetDesiredMatchType();
		const auto FoundMatchTypeGameMode = MatchTypesToModes.Find(MatchType);
		if (!FoundMatchTypeGameMode)
		{
			UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: MatchType=%s not found in MatchTypesToMode"), *GetName(), *MatchType);
			return;
		}

		check(!FoundMatchTypeGameMode->IsNull());

		const FString GameModeName = FoundMatchTypeGameMode->ToSoftObjectPath().ToString();
		const FString MapPath = GetPathForMap(GetRandomMap());

		const FString MatchUrl = FString::Printf(TEXT("%s?game=%s"), *MapPath, *GameModeName);

		UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: ServerTravel to Map=%s with GameMode=%s - "),
			*GetName(), *MapPath, *GameModeName, *MatchUrl);

		World->ServerTravel(MatchUrl);
	}
}

FString ALobbyGameMode::GetPathForMap(const TSoftObjectPtr<UWorld>& World) const
{
	// For some reason an odd extension is being added : e.g. House.house
	// remove the extension
	FString Path = World.ToSoftObjectPath().ToString();
	const auto Index = Path.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if(Index == INDEX_NONE)
	{
		return Path;
	}

	return Path.Left(Index);
}

FString ALobbyGameMode::GetPathForGameMode(const TSoftClassPtr<APaperGolfGameModeBase>& GameMode) const
{
	return GameMode.ToSoftObjectPath().ToString();
}

TSoftObjectPtr<UWorld> ALobbyGameMode::GetRandomMap() const
{
	check(!Maps.IsEmpty());
	return Maps[FMath::RandRange(0, Maps.Num() - 1)];
}

void ALobbyGameMode::ValidateMaps()
{
	for(auto It = Maps.CreateIterator(); It; ++It)
	{
		if (It->IsNull())
		{
			UE_VLOG_UELOG(this, LogPaperGolfGame, Warning, TEXT("%s: Invalid map in Maps array at index %d"), *GetName(), It.GetIndex());
			It.RemoveCurrent();
		}
	}

	if(!ensureMsgf(!Maps.IsEmpty(), TEXT("%s: No valid maps defined!"), *GetName()))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: No valid maps defined!"), *GetName());
	}
}

void ALobbyGameMode::ValidateGameModes()
{
	for(auto It = MatchTypesToModes.CreateIterator(); It; ++It)
	{
		if (It.Key().IsEmpty())
		{
			UE_VLOG_UELOG(this, LogPaperGolfGame, Warning, TEXT("%s: Invalid matchType in MatchTypesToModes array"), *GetName());
			It.RemoveCurrent();
		}
		else if (It.Value().IsNull())
		{
			UE_VLOG_UELOG(this, LogPaperGolfGame, Warning, TEXT("%s: Invalid game mode in MatchTypesToModes array for MatchType=%s"), *GetName(), *It.Key());
			It.RemoveCurrent();
		}
	}

	if (!ensureMsgf(!MatchTypesToModes.IsEmpty(), TEXT("%s: No valid game mode mappings defined!"), *GetName()))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: No valid game mode mappings defined!"), *GetName());
	}
}
