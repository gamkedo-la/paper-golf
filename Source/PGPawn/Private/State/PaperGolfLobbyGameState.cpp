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
	DOREPLIFETIME(APaperGolfLobbyGameState, MinPlayers);
	DOREPLIFETIME(APaperGolfLobbyGameState, MaxPlayers);
}

void APaperGolfLobbyGameState::Initialize(const FString& InGameModeName, int32 InMinPlayers, int32 InMaxPlayers)
{
	UE_VLOG_UELOG(this, LogPGPawn, Display, TEXT("%s: Initialize - GameModeName=%s; MinPlayers=%d; MaxPlayers=%d"), *GetName(), *InGameModeName, InMinPlayers, InMaxPlayers);

	GameModeName = InGameModeName;
	MinPlayers = InMinPlayers;
	MaxPlayers = InMaxPlayers;

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
	return PlayerArray.Num() >= MinPlayers;
}
