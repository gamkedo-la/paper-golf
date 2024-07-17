// Copyright Game Salutes. All Rights Reserved.


#include "GameMode/PaperGolfGameModeBase.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PaperGolfLogging.h"

#include "Controller/GolfAIController.h"

#include "State/GolfPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfGameModeBase)

APaperGolfGameModeBase::APaperGolfGameModeBase()
{
	// We will start the game when the desired number of players have joined or other time out happens which should trigger an AI bot to be spawned in their place
	// If a player drop, then they should be replaced with a bot until another player joins
	bDelayedStart = true;
	bStartPlayersAsSpectators = true;
	
	AIControllerClass = AGolfAIController::StaticClass();
}

void APaperGolfGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitGame - MapName=%s, Options=%s; DefaultDesiredNumberOfPlayers=%d"), *GetName(), *MapName, *Options, DefaultDesiredNumberOfPlayers);

	Super::InitGame(MapName, Options, ErrorMessage);

	if (!SetDesiredNumberOfPlayersFromPIESettings() && DesiredNumberOfPlayers == 0)
	{
		DesiredNumberOfPlayers = DefaultDesiredNumberOfPlayers;
	}

	if(DesiredNumberOfBotPlayers == 0)
	{
		DesiredNumberOfBotPlayers = DefaultMinDesiredNumberOfBotPlayers;
	}

	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: InitGame - DesiredNumberOfPlayers=%d; DesiredNumberOfBotPlayers=%d"), *GetName(), DesiredNumberOfPlayers, DesiredNumberOfBotPlayers);
}

void APaperGolfGameModeBase::InitGameState()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitGameState"), *GetName());

	Super::InitGameState();
}

void APaperGolfGameModeBase::OnPostLogin(AController* NewPlayer)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: OnPostLogin - NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(NewPlayer));

	Super::OnPostLogin(NewPlayer);

	if (NewPlayer)
	{
		SetDefaultPlayerName(*NewPlayer);
	}
}

void APaperGolfGameModeBase::SetDefaultPlayerName(AController& Player)
{
	// See https://forums.unrealengine.com/t/pass-playerstate-to-the-server-when-the-client-join-a-session/719511
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: SetDefaultPlayerName - Player=%s"), *GetName(), *Player.GetName());

	// TODO: Need option for player to set name when joining session

	if (!Player.IsPlayerController())
	{
		return;
	}

	auto PlayerState = Player.GetPlayerState<AGolfPlayerState>();
	if(!ensureMsgf(PlayerState, TEXT("%s: Player %s does not have AGolfPlayerState; PlayerState=%s"), *GetName(), *Player.GetName(), *LoggingUtils::GetName(Player.GetPlayerState<APlayerState>())))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: Player %s does not have AGolfPlayerState; PlayerState=%s"),
			*GetName(), *Player.GetName(), *LoggingUtils::GetName(Player.GetPlayerState<APlayerState>()));
		return;
	}

	++HumanPlayerDefaultNameIndex;
	PlayerState->SetPlayerName(FString::Printf(TEXT("Player %d"), HumanPlayerDefaultNameIndex));
}

void APaperGolfGameModeBase::StartGame()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: StartGame"), *GetName());

	OnGameStart();
}

bool APaperGolfGameModeBase::ReadyToStartMatch_Implementation()
{
	bool bReady{};
	if (GetMatchState() == MatchState::WaitingToStart)
	{
		bReady = GetTotalNumberOfPlayers() == DesiredNumberOfPlayers + DesiredNumberOfBotPlayers;
	}

	// TODO: We may want to manually start the match after a countdown timer or something - this is checked on Tick in the AGameMode base class and will immediately start the match
	// (Calling StartMatch) when this returns true
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log,
		TEXT("%s: ReadyToStartMatch_Implementation: %s : NumPlayers=%d; NumBots=%d; DesiredNumberOfPlayers=%d; DesiredNumberOfBotPlayers=%d"),
		*GetName(), LoggingUtils::GetBoolString(bReady), NumPlayers, NumBots, DesiredNumberOfPlayers, DesiredNumberOfBotPlayers);

	return bReady;
}

UClass* APaperGolfGameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: GetDefaultPawnClassForController_Implementation - InController=%s"), *GetName(), *LoggingUtils::GetName(InController));

	return Super::GetDefaultPawnClassForController_Implementation(InController);

	// TODO: This is another spot where we can customize AI or which pawn to use for a given player - AI or human
}

void APaperGolfGameModeBase::OnMatchStateSet()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: OnMatchStateSet: %s"), *GetName(), *MatchState.ToString());

	Super::OnMatchStateSet();

	// TODO: If we start players as spectators, we can use RestartPlayer(AController*) to spawn them in one at a time - or maybe call it when starting the hole on each player
}

void APaperGolfGameModeBase::HandleMatchHasStarted()
{
	// This is called when match state transitions to InProgress.
	// Equivalent to writing logic in OnMatchStateSet checking MatchState == MatchState::InProgress
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: HandleMatchHasStarted"), *GetName());

	Super::HandleMatchHasStarted();

	// If playing standalone then don't have a timer
	if (DelayStartWithTimer())
	{
		StartGameWithDelay();
	}
	else
	{
		StartGame();
	}
}

bool APaperGolfGameModeBase::DelayStartWithTimer() const
{
	return GetNetMode() != NM_Standalone;
}

void APaperGolfGameModeBase::StartGameWithDelay()
{
	auto World = GetWorld();
	check(World);

	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(TimerHandle, this, &APaperGolfGameModeBase::StartGame, MatchStartDelayTime, false);
}

void APaperGolfGameModeBase::BeginPlay()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();
}

void APaperGolfGameModeBase::NotifyHoleAboutToStart()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: NotifyHoleAboutToStart"), *GetName());

	bAllowPlayerSpawn = true;
}

bool APaperGolfGameModeBase::PlayerCanRestart_Implementation(APlayerController* Player)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: PlayerCanRestart_Implementation: Player=%s -> %s"), 
		*GetName(), *LoggingUtils::GetName(Player), LoggingUtils::GetBoolString(bAllowPlayerSpawn));

	return bAllowPlayerSpawn;
}

APawn* APaperGolfGameModeBase::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	const auto Pawn = Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);

	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: SpawnDefaultPawnAtTransform_Implementation - NewPlayer=%s; Pawn=%s"),
		*GetName(), *LoggingUtils::GetName(NewPlayer), *LoggingUtils::GetName(Pawn));
	
	return Pawn;
}

AActor* APaperGolfGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	// Unless we implement difficulties with different player starts, we can just use the single player start location
	// If we wanted to do the latter, we may not even need to override this as can just use tag names like "Amateur, Pro, Expert" and then use
	// FindPlayerStart("Amateur") which returns an AActor* that we can pass to RestartPlayer(AController*, AActor*) so it uses that player start
	const auto Actor = Super::ChoosePlayerStart_Implementation(Player);

	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: ChoosePlayerStart_Implementation - Player=%s; PlayerStart=%s"),
		*GetName(), *LoggingUtils::GetName(Player), *LoggingUtils::GetName(Actor));

	return Actor;
}

bool APaperGolfGameModeBase::ShouldSpawnAtStartSpot(AController* Player)
{
	const bool bResult = Super::ShouldSpawnAtStartSpot(Player);

	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: ShouldSpawnAtStartSpot - Player=%s; ShouldSpawn=%s"),
		*GetName(), *LoggingUtils::GetName(Player), LoggingUtils::GetBoolString(bResult));

	return bResult;
}

void APaperGolfGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: HandleStartingNewPlayer_Implementation - NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(NewPlayer));

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

bool APaperGolfGameModeBase::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
	const auto bResult = Super::UpdatePlayerStartSpot(Player, Portal, OutErrorMessage);

	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: UpdatePlayerStartSpot - Player=%s, Portal=%s; OutErrorMessage=%s; Result=%s"),
		*GetName(), *LoggingUtils::GetName(Player), *Portal, *OutErrorMessage, LoggingUtils::GetBoolString(bResult));

	return bResult;
}

void APaperGolfGameModeBase::GenericPlayerInitialization(AController* NewPlayer)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: GenericPlayerInitialization - NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(NewPlayer));

	Super::GenericPlayerInitialization(NewPlayer);

	// TODO: Maybe we can do something with AI here?
}

void APaperGolfGameModeBase::Logout(AController* Exiting)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: Logout - Exiting=%s"), *GetName(), *LoggingUtils::GetName(Exiting));

	Super::Logout(Exiting);
}

void APaperGolfGameModeBase::HandleMatchIsWaitingToStart()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: HandleMatchIsWaitingToStart"), *GetName());

	Super::HandleMatchIsWaitingToStart();

	// This is where ShooterGameMode creates bots does it in ShooterGame\Private\Online\ShooterGameMode.cpp
	// TODO: However, this is called immediately so will need to create additional bots on a timer if the total number of players is less than the desired number
	// This could be checked inReadyToStartMatch_Implementation as that is called continuously after this function is called
	CreateBots();
}

void APaperGolfGameModeBase::StartMatch()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: StartMatch"), *GetName());

	Super::StartMatch();
}

void APaperGolfGameModeBase::EndMatch()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: EndMatch"), *GetName());

	Super::EndMatch();
}

void APaperGolfGameModeBase::RestartGame()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: RestartGame"), *GetName());

	Super::RestartGame();

}

void APaperGolfGameModeBase::AbortMatch()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: AbortMatch"), *GetName());

	Super::AbortMatch();
}

bool APaperGolfGameModeBase::SetDesiredNumberOfPlayersFromPIESettings()
{
#if WITH_EDITOR

	if (!GIsEditor)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: SetDesiredNumberOfPlayersFromPIESettings: Not running in editor - returning false"), *GetName());

		return false;
	}
	// Get the default Play settings
	ULevelEditorPlaySettings* PlayInSettings = Cast<ULevelEditorPlaySettings>(ULevelEditorPlaySettings::StaticClass()->GetDefaultObject());

	// Retrieve the number of clients (players) from the settings
	PlayInSettings->GetPlayNumberOfClients(DesiredNumberOfPlayers);

	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: SetDesiredNumberOfPlayersFromPIESettings: DesiredNumberOfPlayers=%d"), *GetName(),
		DesiredNumberOfPlayers);

	return true;

#else
	return false;
#endif
}

void APaperGolfGameModeBase::CreateBots()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: CreateBots - DesiredNumberOfBotPlayers=%d"), *GetName(), DesiredNumberOfBotPlayers);

	for (int32 i = 0; i < DesiredNumberOfBotPlayers; ++i)
	{
		CreateBot(i + 1);
	}
}

void APaperGolfGameModeBase::CreateBot(int32 BotNumber)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: CreateBot - BotNumber=%d"), *GetName(), BotNumber);

	if(!ensureMsgf(AIControllerClass, TEXT("AIControllerClass is not set!")))
	{
		return;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = nullptr;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = nullptr;

	UWorld* World = GetWorld();
	AGolfAIController* AIC = World->SpawnActor<AGolfAIController>(AIControllerClass, SpawnInfo);

	if (!AIC)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: CreateBot - BotNumber=%d - Failed to spawn AIController"), *GetName(), BotNumber);
		return;
	}

	InitBot(*AIC, BotNumber);

	OnBotSpawnedIntoGame(*AIC, BotNumber);
}

void APaperGolfGameModeBase::InitBot(AGolfAIController& AIController, int32 BotNumber)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitBot - BotNumber=%d"), *GetName(), BotNumber);

	if(auto PlayerState = AIController.GetGolfPlayerState(); ensureMsgf(PlayerState, TEXT("%s: No player state for %s - BotNumber=%d"), *GetName(), *AIController.GetName(), BotNumber))
	{
		// TODO: Draw from a random list of funny golfer names
		PlayerState->SetPlayerName(FString::Printf(TEXT("Bot %d"), BotNumber));
	}
	else
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: InitBot - BotNumber=%d - Failed to get player state"), *GetName(), BotNumber);
	}

	// NumPlayers is incremented automatically but NumBots is not
	++NumBots;
}
