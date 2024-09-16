// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "GameMode/LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"
#include "MultiplayerSessionsSubsystem.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PaperGolfLogging.h"

#include "Utils/ObjectUtils.h"

#include "Config/GameModeOptionParams.h"

#include "GameMode/PaperGolfGameModeBase.h"

#include "State/PaperGolfLobbyGameState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LobbyGameMode)


void ALobbyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitGame - %s"), *GetName(), *MapName);

	Super::InitGame(MapName, Options, ErrorMessage);

	bMatchStarted = false;

#if !UE_BUILD_SHIPPING
	ValidateMaps();
	ValidateGameModes();
#endif

}

void ALobbyGameMode::InitGameState()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitGameState"), *GetName());

	Super::InitGameState();

#if WITH_EDITOR
	if (auto World = GetWorld(); World && !World->IsGameWorld())
	{
		// Skip initialization in editor world as its not needed and sometimes the assertion for the world subsystem fails
		return;
	}
#endif 
	// LobbyGameMode is created and server hosts the game so the max desired players would be known here

	auto LobbyGameState = GetGameState<APaperGolfLobbyGameState>();
	if (!ensureMsgf(LobbyGameState, TEXT("%s: GameState=%s is not APaperGolfLobbyGameState"), *GetName(), *LoggingUtils::GetName(GameState)))
	{
		return;
	}

	auto Subsystem = GetMultiplayerSessionsSubsystem();
	if (!Subsystem)
	{
		return;
	}

	const auto& MatchType = Subsystem->GetDesiredMatchType();
	const auto& FoundMatchTypeInfo = MatchTypesToModes.Find(MatchType);
	if (!FoundMatchTypeInfo)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: MatchType=%s not found in MatchTypesToMode"), *GetName(), *MatchType);
		return;
	}

	check(!FoundMatchTypeInfo->GameMode.IsNull());

	LobbyGameState->Initialize(
		FoundMatchTypeInfo->Name,
		FoundMatchTypeInfo->MinPlayers,
		Subsystem->GetDesiredNumPublicConnections()
	);
}

bool ALobbyGameMode::HostStartMatch()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: HostStartMatch"), *GetName());
	if (!CanStartMatch())
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: HostStartMatch - FALSE - conditions not met to start match"), *GetName());

		return false;
	}

	auto Subsystem = GetMultiplayerSessionsSubsystem();
	if (!Subsystem)
	{
		return false;
	}

	auto World = GetWorld();
	if (!ensure(World))
	{
		return false;
	}

	bUseSeamlessTravel = !WITH_EDITOR;

	const auto& MatchType = Subsystem->GetDesiredMatchType();
	const auto& FoundMatchTypeInfo = MatchTypesToModes.Find(MatchType);
	if (!FoundMatchTypeInfo)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: HostStartMatch - MatchType=%s not found in MatchTypesToMode"), *GetName(), *MatchType);
		return false;
	}

	check(!FoundMatchTypeInfo->GameMode.IsNull());

	const FString GameModeName = FoundMatchTypeInfo->GameMode.ToSoftObjectPath().ToString();
	const FString MapPath = GetPathForMap(GetRandomMap());

	const FString MatchUrl = FString::Printf(TEXT("%s?game=%s?%s%d?%s%d"),
		*MapPath, *GameModeName, PG::GameModeOptions::NumPlayers, GetNumPlayers(), PG::GameModeOptions::NumBots, GetNumBots());

	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: ServerTravel to Map=%s with GameMode=%s - %s"),
		*GetName(), *MapPath, *GameModeName, *MatchUrl);

	// Make sure don't start twice from a menu, especially since doing seamless travel
	bMatchStarted = true;

	World->ServerTravel(MatchUrl);

	return true;
}

int32 ALobbyGameMode::GetNumBots() const
{
	auto Subsystem = GetMultiplayerSessionsSubsystem();
	if (!Subsystem)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: GetNumBots - 0 - Subsystem is null"), *GetName());
		return 0;
	}

	if (!Subsystem->AllowBots())
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: GetNumBots - 0 - Bots are not allowed"), *GetName());
		return 0;
	}

	// Number of bots is max players - current players
	// GetNumPlayers() does not follow const-correctness
	const auto CurrentNumberOfPlayers = const_cast<ALobbyGameMode*>(this)->GetNumPlayers();

	const auto LobbyGameState = GetGameState<APaperGolfLobbyGameState>();
	if (!LobbyGameState)
	{
		// Already logged this as error elsewhere
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: GGetNumBots - 0 - GameState=%s is not APaperGolfLobbyGameState"), *GetName(), *LoggingUtils::GetName(GameState));
		return 0;
	}

	return FMath::Max(0, LobbyGameState->GetMaxPlayers() - CurrentNumberOfPlayers);
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

	const auto NumberOfPlayers = GetNumPlayers();

	auto LobbyGameState = GetGameState<APaperGolfLobbyGameState>();
	if (!LobbyGameState)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: GameState=%s is not APaperGolfLobbyGameState"), *GetName(), *LoggingUtils::GetName(GameState));
		return;
	}

	// TODO: When adding bot options to the multiplayer menu, this could be updated to take that into account
	// May want to allow the game to start with only bots and then maybe players join later or maybe criteria is the same with MinPlayers configured from the game mode properties
	// but when starting the game with HostStartGame with fewer than max players then ?numBots is passed to the server travel url
	// The easiest option is to add a checkbox to the Multiplayer menu on whether allow bots or nots. If bots are allowed then the number of bots to use is just 
	// MaxPlayers - NumberOfPlayers.  The bot option can be read on the "Options" parameter on InitGame
	bCanStartMatch = LobbyGameState->IsGameEligibleToStart();

#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		FString OnlineName;
		const bool hasOnlineName = [&]()
		{
			if (auto Subsystem = GetMultiplayerSessionsSubsystem(); Subsystem)
			{
				return Subsystem->GetOnlineUserName(NewPlayer, OnlineName);
			}
			return false;
		}();

		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Green,
			FString::Printf(TEXT("User: %s has joined"),
				hasOnlineName ? *OnlineName : *FString::Printf(TEXT("Player %d"), NumberOfPlayers))
		);
	}
#endif

	if (NumberOfPlayers == LobbyGameState->GetMaxPlayers() && CanStartMatch())
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: PostLogin - Max of %d players reached - host is auto-starting the match"), *GetName(), NumberOfPlayers);
		HostStartMatch();
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
		else if (It.Value().GameMode.IsNull())
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

UMultiplayerSessionsSubsystem* ALobbyGameMode::GetMultiplayerSessionsSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!ensure(GameInstance))
	{
		return nullptr;
	}

	auto Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	ensure(Subsystem);

	return Subsystem;
}
