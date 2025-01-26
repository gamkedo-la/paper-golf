// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "State/PaperGolfLobbyGameState.h"

#include "Net/UnrealNetwork.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "Utils/ArrayUtils.h"
#include "PGPawnLogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfLobbyGameState)

void APaperGolfLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APaperGolfLobbyGameState, GameModeName);
	DOREPLIFETIME(APaperGolfLobbyGameState, MapName);
	DOREPLIFETIME(APaperGolfLobbyGameState, MinPlayers);
	DOREPLIFETIME(APaperGolfLobbyGameState, MaxPlayers);
	DOREPLIFETIME(APaperGolfLobbyGameState, bAllowBots);
}

void APaperGolfLobbyGameState::Initialize(const FString& InGameModeName, const FString& InMapName, int32 InMinPlayers, int32 InMaxPlayers, bool bInAllowBots)
{
	UE_VLOG_UELOG(this, LogPGPawn, Display, TEXT("%s: Initialize - GameModeName=%s; InMapName=%s; MinPlayers=%d; MaxPlayers=%d; bAllowBots=%s"),
		*GetName(), *InGameModeName, *InMapName, InMinPlayers, InMaxPlayers, LoggingUtils::GetBoolString(bInAllowBots));

	GameModeName = InGameModeName;
	MapName = InMapName;
	MinPlayers = InMinPlayers;
	MaxPlayers = InMaxPlayers;
	bAllowBots = bInAllowBots;

	bInitialized = true;
}

void APaperGolfLobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: AddPlayerState - PlayerState=%s"), *GetName(), *LoggingUtils::GetName<APlayerState>(PlayerState));

	Super::AddPlayerState(PlayerState);

	OnPlayerStatesUpdated.Broadcast(this, PlayerArray.Num());
}

void APaperGolfLobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: RemovePlayerState - PlayerState=%s"), *GetName(), *LoggingUtils::GetName<APlayerState>(PlayerState));

	Super::RemovePlayerState(PlayerState);

	OnPlayerStatesUpdated.Broadcast(this, PlayerArray.Num());
}

bool APaperGolfLobbyGameState::IsGameEligibleToStart() const
{
	const auto NumPlayers = PlayerArray.Num();

	if (NumPlayers == 0)
	{
		return false;
	}

	// Human player condition reached - return true
	if (NumPlayers >= MinPlayers)
	{
		return true;
	}

	// If there are fewer than the minimum number of players but at least 1
	// and bots are allowed then the game is also eligble to start after configured elasped time

	if (!bAllowBots)
	{
		return false;
	}

	auto World = GetWorld();
	if (!ensure(World))
	{
		return false;
	}

	return World->GetTimeSeconds() >= MinElapsedTimeStartWithBots;
}
