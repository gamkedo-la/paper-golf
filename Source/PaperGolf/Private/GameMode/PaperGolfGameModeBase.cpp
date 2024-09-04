// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "GameMode/PaperGolfGameModeBase.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PaperGolfLogging.h"

#include "Debug/PGConsoleVars.h"

#include "Interfaces/GolfController.h"
#include "Controller/GolfAIController.h"

#include "State/GolfPlayerState.h"

#include "State/PaperGolfGameStateBase.h"

#include "Config/GameModeOptionParams.h"

#include "Pawn/PaperGolfPawn.h"

#include "GameMode/HoleTransitionComponent.h"

#include "Utils/VisualLoggerUtils.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include "Config/PlayerConfig.h"
#include "Config/PlayerStateConfigurator.h"

#if WITH_EDITOR
	#include "Settings/LevelEditorPlaySettings.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfGameModeBase)

APaperGolfGameModeBase::APaperGolfGameModeBase()
{
	// We will start the game when the desired number of players have joined or other time out happens which should trigger an AI bot to be spawned in their place
	// If a player drop, then they should be replaced with a bot until another player joins
	bDelayedStart = true;
	bStartPlayersAsSpectators = true;
	
	AIControllerClass = AGolfAIController::StaticClass();

	HoleTransitionComponent = CreateDefaultSubobject<UHoleTransitionComponent>(TEXT("HoleTransition"));
}

void APaperGolfGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	PG::VisualLoggerUtils::StartAutomaticRecording(this);

	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitGame - MapName=%s, Options=%s; DefaultDesiredNumberOfPlayers=%d"), *GetName(), *MapName, *Options, DefaultDesiredNumberOfPlayers);

	Super::InitGame(MapName, Options, ErrorMessage);

#if PG_DEBUG_ENABLED

	if (const auto StartHoleOverride = PG::CStartHoleOverride.GetValueOnGameThread(); StartHoleOverride > 0)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: InitGame - Overriding default StartHole=%d to %d"), *GetName(), StartHoleNumber, StartHoleOverride);
		StartHoleNumber = StartHoleOverride;
	}

#endif

	InitNumberOfPlayers(Options);

	InitPlayerStateDefaults();
}

void APaperGolfGameModeBase::InitNumberOfPlayers(const FString& Options)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitNumberOfPlayers - Options=%s"), *GetName(), *Options);

	if (!SetDesiredNumberOfPlayersFromPIESettings())
	{
		SetNumberOfPlayersFromOptions(Options);
	}
	// This will be optimized out in non-editor builds
	else
	{
		// If playing in the editor, consider both the number of human players from editor windows and the options passed into the game mode
		const auto EditorHumanPlayers = DesiredNumberOfPlayers;

		// reset number of players and attempt to read from options
		DesiredNumberOfPlayers = 0;
		SetNumberOfPlayersFromOptions(Options);

		// Use the editor values for number of human players if none passed from options
		DesiredNumberOfPlayers = FMath::Max(DesiredNumberOfPlayers, EditorHumanPlayers);
	}

	// fallback to defaults
	if (DesiredNumberOfPlayers <= 0)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: InitNumberOfPlayers - Falling back to default number of players - DefaultDesiredNumberOfPlayers=%d"), *GetName(), DefaultDesiredNumberOfPlayers);
		DesiredNumberOfPlayers = DefaultDesiredNumberOfPlayers;
	}

	if (DesiredNumberOfBotPlayers <= 0)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: InitNumberOfPlayers - Falling back to default number of bot players - DefaultMinDesiredNumberOfBotPlayers=%d"), *GetName(), DefaultMinDesiredNumberOfBotPlayers);
		DesiredNumberOfBotPlayers = DefaultMinDesiredNumberOfBotPlayers;
	}

	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: InitNumberOfPlayers - DesiredNumberOfPlayers=%d; DesiredNumberOfBotPlayers=%d"), *GetName(), DesiredNumberOfPlayers, DesiredNumberOfBotPlayers);
}

void APaperGolfGameModeBase::SetNumberOfPlayersFromOptions(const FString& Options)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: SetNumberOfPlayersFromOptions - Options=%s"), *GetName(), *Options);

	if(FParse::Value(*Options, PG::GameModeOptions::NumPlayers, DesiredNumberOfPlayers))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: SetNumberOfPlayersFromOptions - DesiredNumberOfPlayers=%d"), *GetName(), DesiredNumberOfPlayers);
	}

	if(FParse::Value(*Options, PG::GameModeOptions::NumBots, DesiredNumberOfBotPlayers))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: SetNumberOfPlayersFromOptions - DesiredNumberOfBotPlayers=%d"), *GetName(), DesiredNumberOfBotPlayers);
	}
}

void APaperGolfGameModeBase::InitPlayerStateDefaults()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitPlayerStateDefaults"), *GetName());

	if (!ensureMsgf(PlayerConfigData, TEXT("%s: InitPlayerStateDefaults - PlayerConfigData is not set"), *GetName()))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: InitPlayerStateDefaults - PlayerConfigData is not set"), *GetName());
		return;
	}

	PlayerStateConfigurator = MakeUnique<PG::FPlayerStateConfigurator>(PlayerConfigData);
}

void APaperGolfGameModeBase::InitGameState()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitGameState"), *GetName());

	Super::InitGameState();

	// Set start hole
	auto PaperGolfGameState = GetGameState<APaperGolfGameStateBase>();

	if (!ensureMsgf(PaperGolfGameState, TEXT("%s: PaperGolfGameState=%s is not APaperGolfGameStateBase"), *GetName(), *LoggingUtils::GetName(GameState)))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: PaperGolfGameState=%s is not APaperGolfGameStateBase"),
			*GetName(), *LoggingUtils::GetName(GameState));
		return;
	}

	// If the game is not being reset for the next hole then the current hole number needs to be set
	// in InitGameState as ChoosePlayerStart will get called before the first hole starts and we don't want it to start at the default value
	if (!bWasReset)
	{
		PaperGolfGameState->SetCurrentHoleNumber(StartHoleNumber);
	}
}

void APaperGolfGameModeBase::Reset()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: Reset"), *GetName());
	bWasReset = true;

	Super::Reset();
}

void APaperGolfGameModeBase::OnPostLogin(AController* NewPlayer)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: OnPostLogin - NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(NewPlayer));

	Super::OnPostLogin(NewPlayer);

	OnPlayerJoined(NewPlayer);
}

void APaperGolfGameModeBase::InitSeamlessTravelPlayer(AController* NewController)
{
	// Seamless travel is done for hosting player when doing a seamless server travel. PostLogin is not called in this case so standardizing with OnPlayerJoined

	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitSeamlessTravelPlayer - NewController=%s"), *GetName(), *LoggingUtils::GetName(NewController));

	Super::InitSeamlessTravelPlayer(NewController);

	OnPlayerJoined(NewController);
}

void APaperGolfGameModeBase::OnPlayerJoined(AController* NewPlayer)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: OnPlayerJoined - NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(NewPlayer));

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

	if (PlayerStateConfigurator)
	{
		PlayerStateConfigurator->AssignToPlayer(*PlayerState);
	}
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

void APaperGolfGameModeBase::OnStartHole(int32 HoleNumber)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: OnStartHole - HoleNumber=%d"), *GetName(), HoleNumber);

	bWasReset = false;

	StartHole(HoleNumber);
}

void APaperGolfGameModeBase::OnHoleComplete()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: OnHoleComplete"), *GetName());

	DoAdditionalHoleComplete();
}

void APaperGolfGameModeBase::OnCourseComplete()
{	
	UE_VLOG_UELOG(this, LogPaperGolfGame, Verbose, TEXT("%s: OnCourseComplete - Enter"), *GetName());

	const auto ActionDelayTime = DoAdditionalCourseComplete();
	UE_VLOG_UELOG(this, LogPaperGolfGame, Verbose, TEXT("%s: OnCourseComplete - ActionDelayTime=%f"), *GetName(), ActionDelayTime);

	if (ActionDelayTime > 0)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::DoCourseComplete, ActionDelayTime);
	}
	else
	{
		DoCourseComplete();
	}
}

void APaperGolfGameModeBase::DoCourseComplete()
{
	if (bRestartGameOnCourseComplete)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: DoCourseComplete - bRestartGameOnCourseComplete = TRUE - Restarting the map"), *GetName());

		RestartGame();
	}
	else
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: DoCourseComplete - Returning to Main Menu Host"), *GetName());

		ReturnToMainMenuHost();
	}
}

void APaperGolfGameModeBase::BeginPlay()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	if (auto GolfEventSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventSubsystem))
	{
		GolfEventSubsystem->OnPaperGolfStartHole.AddDynamic(this, &ThisClass::OnStartHole);
		GolfEventSubsystem->OnPaperGolfCourseComplete.AddDynamic(this, &ThisClass::OnCourseComplete);
		GolfEventSubsystem->OnPaperGolfNextHole.AddDynamic(this, &ThisClass::OnHoleComplete);
	}
}

void APaperGolfGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: EndPlay - EndPlayReason=%s"), *GetName(), *UEnum::GetValueAsString(EndPlayReason));

	Super::EndPlay(EndPlayReason);

	PG::VisualLoggerUtils::StopAutomaticRecording(this);
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
	
	if (auto PaperGolfPawn = Cast<APaperGolfPawn>(Pawn); PaperGolfPawn)
	{
		auto PlayerState = NewPlayer->GetPlayerState<AGolfPlayerState>();
		if (ensureMsgf(PlayerState, TEXT("%s: No player state for NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(NewPlayer)))
		{
			PaperGolfPawn->SetPawnColor(PlayerState->GetPlayerColor());
		}
		else
		{
			UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: No player state for NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(NewPlayer));
		}
	}

	return Pawn;
}

AActor* APaperGolfGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	// Unless we implement difficulties with different player starts, we can just use the single player start location
	// If we wanted to do the latter, we may not even need to override this as can just use tag names like "Amateur, Pro, Expert" and then use
	// FindPlayerStart("Amateur") which returns an AActor* that we can pass to RestartPlayer(AController*, AActor*) so it uses that player start

	check(HoleTransitionComponent);
	auto Actor = HoleTransitionComponent->ChoosePlayerStart(Player);

	if (!Actor)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Warning, TEXT("%s: ChoosePlayerStart_Implementation - Player=%s - Using default implementation"),
			*GetName(), *LoggingUtils::GetName(Player), *LoggingUtils::GetName(Actor));
		Actor = Super::ChoosePlayerStart_Implementation(Player);
	}

	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: ChoosePlayerStart_Implementation - Player=%s; PlayerStart=%s"),
		*GetName(), *LoggingUtils::GetName(Player), *LoggingUtils::GetName(Actor));

	if (auto GolfController = Cast<IGolfController>(Player); GolfController)
	{
		GolfController->ReceivePlayerStart(Actor);
	}

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

		if (PlayerStateConfigurator)
		{
			PlayerStateConfigurator->AssignToPlayer(*PlayerState);
		}
	}
	else
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: InitBot - BotNumber=%d - Failed to get player state"), *GetName(), BotNumber);
	}

	// NumPlayers is incremented automatically but NumBots is not
	++NumBots;
}
