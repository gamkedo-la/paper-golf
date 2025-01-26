// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "GameMode/LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"
#include "MultiplayerSessionsSubsystem.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PaperGolfLogging.h"
#include "Utils/VisualLoggerUtils.h"

#include "Utils/ObjectUtils.h"

#include "Config/GameModeOptionParams.h"

#include "GameMode/PaperGolfGameModeBase.h"

#include "State/PaperGolfLobbyGameState.h"

#include "Library/PaperGolfGameUtilities.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LobbyGameMode)

void ALobbyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	PG::VisualLoggerUtils::StartAutomaticRecording(this);

	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitGame - %s"), *GetName(), *MapName);

	Super::InitGame(MapName, Options, ErrorMessage);

	bMatchStarted = false;

	if (!ensure(IsValid(GameSessionConfig)))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: InitGame - GameSessionConfig is NULL"), *GetName());
		ErrorMessage = "GameSessionConfig is NULL";
		return;
	}

	GameSessionConfig->Validate();
}

void ALobbyGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: EndPlay - EndPlayReason=%s"), *GetName(), *UEnum::GetValueAsString(EndPlayReason));

	Super::EndPlay(EndPlayReason);

	PG::VisualLoggerUtils::StopAutomaticRecording(this);
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

	if (!GameSessionConfig)
	{
		// Already asserted in InitGame
		return;
	}

	LobbyGameState = GetGameState<APaperGolfLobbyGameState>();
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

	bool bMatchTypeValid{};
	const auto& FoundMatchTypeInfo = GameSessionConfig->GetGameModeInfo(MatchType, bMatchTypeValid);
	if (!bMatchTypeValid)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: MatchType=%s not found in MatchTypesToMode"), *GetName(), *MatchType);
		return;
	}

	LobbyGameState->Initialize(
		FoundMatchTypeInfo.Name,
		Subsystem->GetDesiredMap(),
		FoundMatchTypeInfo.MinPlayers,
		Subsystem->GetDesiredNumPublicConnections(),
		Subsystem->AllowBots()
	);
}

bool ALobbyGameMode::CanStartMatch() const
{
	return !bMatchStarted && LobbyGameState && LobbyGameState->IsGameEligibleToStart();
}

bool ALobbyGameMode::HostStartMatch()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: HostStartMatch"), *GetName());

	if (!GameSessionConfig)
	{
		return false;
	}

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

	bool bMatchTypeValid{};
	const auto& FoundMatchTypeInfo = GameSessionConfig->GetGameModeInfo(MatchType, bMatchTypeValid);

	if (!bMatchTypeValid)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: HostStartMatch - MatchType=%s not found in MatchTypesToMode"), *GetName(), *MatchType);
		return false;
	}

	const FString GameMode = UGameSessionConfig::GetGameModePath(FoundMatchTypeInfo.GameMode);
	const FString MapPath = GameSessionConfig->GetPathForMapName(Subsystem->GetDesiredMap());

	const FString MatchUrl = UPaperGolfGameUtilities::CreateCourseOptionsUrl(
		MapPath, GameMode,
		GetNumPlayers(), GetNumBots(),
		Subsystem->AllowBots(),
		Subsystem->GetDesiredNumPublicConnections()
	);

	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: ServerTravel to Map=%s with GameMode=%s - %s"),
		*GetName(), *MapPath, *GameMode, *MatchUrl);

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
		UE_VLOG_UELOG(this, LogPaperGolfGame, Warning, TEXT("%s: GetNumBots - 0 - Subsystem is null"), *GetName());
		return 0;
	}

	if (!Subsystem->AllowBots())
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: GetNumBots - 0 - Bots are not allowed"), *GetName());
		return 0;
	}

	// Number of bots is max players - current players
	// GetNumPlayers() from AGameModeBase does not follow const-correctness
	const auto CurrentNumberOfPlayers = const_cast<ALobbyGameMode*>(this)->GetNumPlayers();

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

	if (!IsValid(GameSessionConfig) || !GameSessionConfig->IsValid())
	{
		return;
	}

	const auto NumberOfPlayers = GetNumPlayers();

	if (!LobbyGameState)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: GameState=%s is not APaperGolfLobbyGameState"), *GetName(), *LoggingUtils::GetName(GameState));
		return;
	}


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
