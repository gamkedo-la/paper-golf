// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "GameMode/PaperGolfGameModeBase.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PaperGolfLogging.h"

#include "Debug/PGConsoleVars.h"

#include "PGConstants.h"

#include "Interfaces/GolfController.h"
#include "Controller/GolfAIController.h"

#include "State/GolfPlayerState.h"

#include "State/PaperGolfGameStateBase.h"

#include "Config/GameModeOptionParams.h"

#include "Pawn/PaperGolfPawn.h"

#include "GameMode/HoleTransitionComponent.h"

#include "Utils/VisualLoggerUtils.h"

#include "Subsystems/GolfEventsSubsystem.h"
#include "MultiplayerSessionsSubsystem.h"

#include "Config/PlayerConfig.h"
#include "Config/PlayerStateConfigurator.h"

#include "GameFramework/GameSession.h"

#if WITH_EDITOR
	#include "Settings/LevelEditorPlaySettings.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfGameModeBase)

namespace
{
	constexpr const auto MatchJoinFailureMessage = TEXT("Match is no longer joinable");
}

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

	// TODO: total number of players should not exceed 4 - PG::MaxPlayers
	if (!SetDesiredNumberOfPlayersFromPIESettings())
	{
		InitFromConsoleVars();
		SetNumberOfPlayersFromOptions(Options);
	}
	// This will be optimized out in non-editor builds
	else
	{
		// If playing in the editor, consider both the number of human players from editor windows and the options passed into the game mode
		const auto EditorHumanPlayers = DesiredNumberOfPlayers;

		// reset number of players and attempt to read from console vars and options
		DesiredNumberOfPlayers = 0;

		InitFromConsoleVars();
		SetNumberOfPlayersFromOptions(Options);

		// Use the editor values for number of human players if none passed from options
		if (DesiredNumberOfPlayers == 0)
		{
			DesiredNumberOfPlayers = EditorHumanPlayers;
			MaxPlayers = FMath::Max(MaxPlayers, GetTotalPlayersToStartMatch());
		}
	}

	// fallback to defaults
	CheckDefaultOptions();

	DetermineAllowBots(Options);

	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: InitNumberOfPlayers - DesiredNumberOfPlayers=%d; DesiredNumberOfBotPlayers=%d; MinNumberOfPlayers=%d; MaxPlayers=%d; bAllowBots=%s"),
		*GetName(), DesiredNumberOfPlayers, DesiredNumberOfBotPlayers, MinNumberOfPlayers, MaxPlayers, LoggingUtils::GetBoolString(bAllowBots));
}

void APaperGolfGameModeBase::CheckDefaultOptions()
{
	if (DesiredNumberOfPlayers <= 0)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: InitNumberOfPlayers - Falling back to default number of players - DefaultDesiredNumberOfPlayers=%d"),
			*GetName(), DefaultDesiredNumberOfPlayers);
		DesiredNumberOfPlayers = DefaultDesiredNumberOfPlayers;
	}

	if (DesiredNumberOfBotPlayers <= 0)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: InitNumberOfPlayers - Falling back to default number of bot players - DefaultMinDesiredNumberOfBotPlayers=%d"),
			*GetName(), DefaultMinDesiredNumberOfBotPlayers);
		DesiredNumberOfBotPlayers = DefaultMinDesiredNumberOfBotPlayers;
	}

	if (GetTotalPlayersToStartMatch() > MaxPlayers)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: InitNumberOfPlayers - Increasing MaxPlayers=%d to %d to account for current num players=%d and bots=%d requested"),
			*GetName(), MaxPlayers, GetTotalPlayersToStartMatch(), DesiredNumberOfPlayers, DesiredNumberOfBotPlayers);
		MaxPlayers = GetTotalPlayersToStartMatch();
	}

	const auto DesiredTotalPlayers = GetTotalPlayersToStartMatch();

	if (!ensureAlwaysMsgf(DesiredTotalPlayers >= MinNumberOfPlayers,
		TEXT("%s: InitNumberOfPlayers - DesiredNumberOfPlayers=%d + DesiredNumberOfBotPlayers=%d < MinNumberOfPlayers=%d"),
		*GetName(), DesiredNumberOfPlayers, DesiredNumberOfBotPlayers, MinNumberOfPlayers))
	{
		const auto AdditionalBotPlayers = MinNumberOfPlayers - DesiredTotalPlayers;
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error,
			TEXT("%s: InitNumberOfPlayers - DesiredNumberOfPlayers=%d + DesiredNumberOfBotPlayers=%d < MinNumberOfPlayers=%d - Adding %d additional bot player%s to hit the minimum"),
			*GetName(), DesiredNumberOfPlayers, DesiredNumberOfBotPlayers, MinNumberOfPlayers, AdditionalBotPlayers, LoggingUtils::Pluralize(AdditionalBotPlayers)
		);
		DesiredNumberOfBotPlayers += AdditionalBotPlayers;
	}
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

	if (FParse::Value(*Options, PG::GameModeOptions::MaxPlayers, MaxPlayers))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: SetNumberOfPlayersFromOptions - MaxPlayers=%d"), *GetName(), MaxPlayers);
	}
}

void APaperGolfGameModeBase::InitFromConsoleVars()
{
#if PG_DEBUG_ENABLED
	
	if (const auto OverrideAllowBots = PG::GameMode::CAllowBots.GetValueOnGameThread(); OverrideAllowBots >= 0)
	{
		const bool bOverrideAllowBots = OverrideAllowBots > 0;

		UE_CVLOG_UELOG(bAllowBots != bOverrideAllowBots, this, LogPaperGolfGame, Display, TEXT("%s: InitFromConsoleVars - bAllowBots= %s -> %s"),
			*GetName(), LoggingUtils::GetBoolString(bAllowBots), LoggingUtils::GetBoolString(bOverrideAllowBots));

		bAllowBots = bOverrideAllowBots;
	}

	if (const auto OverrideDesiredNumberOfPlayers = PG::GameMode::CNumDesiredPlayers.GetValueOnGameThread(); OverrideDesiredNumberOfPlayers >= 0)
	{
		UE_CVLOG_UELOG(DesiredNumberOfPlayers != OverrideDesiredNumberOfPlayers, this, LogPaperGolfGame, Display, TEXT("%s: InitFromConsoleVars - DesiredNumberOfPlayers= %d -> %d"),
			*GetName(), DesiredNumberOfPlayers, OverrideDesiredNumberOfPlayers);

		DesiredNumberOfPlayers = OverrideDesiredNumberOfPlayers;
	}

	if (const auto OverrideDesiredNumberOfBotPlayers = PG::GameMode::CNumDesiredBots.GetValueOnGameThread(); OverrideDesiredNumberOfBotPlayers >= 0)
	{
		UE_CVLOG_UELOG(DesiredNumberOfBotPlayers != OverrideDesiredNumberOfBotPlayers, this, LogPaperGolfGame, Display, TEXT("%s: InitFromConsoleVars - DesiredNumberOfBotPlayers= %d -> %d"),
			*GetName(), DesiredNumberOfBotPlayers, OverrideDesiredNumberOfBotPlayers);

		DesiredNumberOfBotPlayers = OverrideDesiredNumberOfBotPlayers;
	}

	if (const auto OverrideMinTotalPlayers = PG::GameMode::CMinTotalPlayers.GetValueOnGameThread(); OverrideMinTotalPlayers > 0)
	{
		UE_CVLOG_UELOG(MinNumberOfPlayers != OverrideMinTotalPlayers, this, LogPaperGolfGame, Display, TEXT("%s: InitFromConsoleVars - MinNumberOfPlayers= %d -> %d"),
			*GetName(), MinNumberOfPlayers, OverrideMinTotalPlayers);

		MinNumberOfPlayers = OverrideMinTotalPlayers;
	}

	if (const auto OverrideMaxTotalPlayers = PG::GameMode::CMaxTotalPlayers.GetValueOnGameThread(); OverrideMaxTotalPlayers > 0)
	{
		UE_CVLOG_UELOG(MaxPlayers != OverrideMaxTotalPlayers, this, LogPaperGolfGame, Display, TEXT("%s: InitFromConsoleVars - MaxPlayers= %d -> %d"),
			*GetName(), MaxPlayers, OverrideMaxTotalPlayers);

		MaxPlayers = OverrideMaxTotalPlayers;
	}

#endif
}

void APaperGolfGameModeBase::DetermineAllowBots(const FString& Options)
{
	if (NumBots > 0)
	{
		bAllowBots = true;
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: DetermineAllowBots - TRUE - NumBots=%d > 0"), *GetName(), NumBots);

		// Make sure not inconsistent with options
#if !UE_BUILD_SHIPPING
		bool bAllowBotsFromOptions{};
		if (FParse::Bool(*Options, PG::GameModeOptions::AllowBots, bAllowBotsFromOptions))
		{
			if (!ensureAlwaysMsgf(bAllowBotsFromOptions == bAllowBots, TEXT("%s: DetermineAllowBots - NumBots=%d > 0 but bAllowBots=%s != bAllowBots from Options=%s"),
				*GetName(), NumBots, LoggingUtils::GetBoolString(bAllowBots), LoggingUtils::GetBoolString(bAllowBotsFromOptions)))
			{
				UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: DetermineAllowBots - NumBots=%d > 0 but bAllowBots=%s != bAllowBots from Options=%s"),
					*GetName(), NumBots, LoggingUtils::GetBoolString(bAllowBots), LoggingUtils::GetBoolString(bAllowBotsFromOptions));
			}
		}

#endif
	}
	else if (FParse::Bool(*Options, PG::GameModeOptions::AllowBots, bAllowBots))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: DetermineAllowBots - %s - Determined from Options"), *GetName(), LoggingUtils::GetBoolString(bAllowBots));
	}
	else
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: DetermineAllowBots - %s - Defaulting to game mode default value"), *GetName(), LoggingUtils::GetBoolString(bAllowBots));
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

void APaperGolfGameModeBase::HandleSeamlessTravelPlayer(AController*& C)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: HandleSeamlessTravelPlayer - C=%s"), *GetName(), *LoggingUtils::GetName(C));

	// Make sure we can join the match
	if (!ValidateJoinMatch(C))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: HandleSeamlessTravelPlayer - Validation failured - destroying C=%s"), *GetName(), *LoggingUtils::GetName(C));

		if (C)
		{
			C->Destroy();
			C = nullptr;
		}
		return;
	}

	Super::HandleSeamlessTravelPlayer(C);
}

void APaperGolfGameModeBase::KickPlayer(AController* Player, const TCHAR* Reason)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: KickPlayer - Player=%s; Reason=%s"), *GetName(), *LoggingUtils::GetName(Player), Reason);

	if (Player && Player->IsLocalPlayerController())
	{
		// Return server to main menu host which will end the game
		ReturnToMainMenuHost();
		return;
	}

	// Return client to main menu host
	if (auto PC = Cast<APlayerController>(Player); PC)
	{
		PC->ClientReturnToMainMenuWithTextReason(FText::FromString(Reason));
		PC->Destroy();
	}
}

bool APaperGolfGameModeBase::CanCourseBeStarted() const
{
	auto PaperGolfGameState = GetGameState<APaperGolfGameStateBase>();

	if (!ensureMsgf(PaperGolfGameState, TEXT("%s: PaperGolfGameState=%s is not APaperGolfGameStateBase"), *GetName(), *LoggingUtils::GetName(GameState)))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: PaperGolfGameState=%s is not APaperGolfGameStateBase"),
			*GetName(), *LoggingUtils::GetName(GameState));
		return false;
	}

	// TODO: May want to make this configurable
	// return PaperGolfGameState->HasCourseStarted();
	return PaperGolfGameState->GetNumCompletedHoles() == 0;
}

void APaperGolfGameModeBase::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: PreLogin - Options=%s; Address=%s; UniqueId=%s"), *GetName(), *Options, *Address, *UniqueId.ToString());

	if (!ValidateJoinMatch(nullptr))
	{
		ErrorMessage = MatchJoinFailureMessage;
		FGameModeEvents::GameModePreLoginEvent.Broadcast(this, UniqueId, ErrorMessage);

		return;
	}

	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

bool APaperGolfGameModeBase::ValidateJoinMatch(AController* Controller)
{
	if (MatchIsJoinable())
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: ValidateJoinMatch - TRUE - Controller=%s"), *GetName(), *LoggingUtils::GetName(Controller));
		return true;
	}

	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: ValidateJoinMatch - FALSE - Controller=%s"), *GetName(), *LoggingUtils::GetName(Controller));

	KickPlayer(Controller, MatchJoinFailureMessage);

	return false;
}

void APaperGolfGameModeBase::GenericPlayerInitialization(AController* NewPlayer)
{
	// This function is called immediately after OnPostLogin and InitSeamlessTravelPlayer and is designed to handle initialization code for both scenarios
	// Handle our player set up here
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: GenericPlayerInitialization - NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(NewPlayer));

	Super::GenericPlayerInitialization(NewPlayer);

	HandlePlayerJoining(NewPlayer);
}

void APaperGolfGameModeBase::OnPlayerJoined(AController* NewPlayer)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: OnPlayerJoined - NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(NewPlayer));

	if (NewPlayer)
	{
		ConfigureJoinedPlayerState(*NewPlayer);
	}
}

void APaperGolfGameModeBase::ConfigureJoinedPlayerState(AController& Player)
{
	// See https://forums.unrealengine.com/t/pass-playerstate-to-the-server-when-the-client-join-a-session/719511
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: ConfigureJoinedPlayerState - Player=%s"), *GetName(), *Player.GetName());


	if (!Player.IsPlayerController())
	{
		return;
	}

	auto PlayerState = Player.GetPlayerState<AGolfPlayerState>();
	if(!ensureMsgf(PlayerState, TEXT("%s: Player %s does not have AGolfPlayerState; PlayerState=%s"), *GetName(), *Player.GetName(), *LoggingUtils::GetName(Player.GetPlayerState<APlayerState>())))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: Player %s does not have AGolfPlayerState; PlayerState=%s"),
			*GetName(), *Player.GetName(), *LoggingUtils::GetName<APlayerState>(Player.GetPlayerState<APlayerState>()));
		return;
	}

	SetHumanPlayerName(Player, *PlayerState);

	if (PlayerStateConfigurator)
	{
		PlayerStateConfigurator->AssignToPlayer(*PlayerState);
	}
}

void APaperGolfGameModeBase::SetHumanPlayerName(AController& PlayerController, APlayerState& PlayerState)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: SetHumanPlayerName - PlayerController=%s; PlayerState=%s"), 
		*GetName(), *LoggingUtils::GetName(&PlayerController), *LoggingUtils::GetName<APlayerState>(&PlayerState));

	// TODO: Need option for player to set name when joining session if it's not an online session

	// Try to set from multiplayer sessions and if failed then use default index strategy
	if (!TrySetHumanPlayerNameFromMultiplayerSessions(PlayerController, PlayerState))
	{
		++HumanPlayerDefaultNameIndex;
		PlayerState.SetPlayerName(FString::Printf(TEXT("Player %d"), HumanPlayerDefaultNameIndex));
	}
}

bool APaperGolfGameModeBase::TrySetHumanPlayerNameFromMultiplayerSessions(AController& PlayerController, APlayerState& PlayerState)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!ensure(GameInstance))
	{
		return false;
	}

	auto Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	if (!ensure(Subsystem))
	{
		return false;
	}

	FString OnlineName;
	if (Subsystem->GetOnlineUserName(&PlayerController, OnlineName))
	{
		PlayerState.SetPlayerName(OnlineName);
		return true;
	}

	return false;
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
		bReady = GetTotalNumberOfPlayers() == GetTotalPlayersToStartMatch();
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
		// Must still add tick delay to avoid timing issues
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::StartGame));
	}
}

bool APaperGolfGameModeBase::DelayStartWithTimer() const
{
	return GetNetMode() != NM_Standalone;
}

void APaperGolfGameModeBase::StartGameWithDelay()
{
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &APaperGolfGameModeBase::StartGame, MatchStartDelayTime, false);
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
	EndMatch();

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


void APaperGolfGameModeBase::Logout(AController* Exiting)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: Logout - Exiting=%s"), *GetName(), *LoggingUtils::GetName(Exiting));

	// Logouts gets called for AI controllers as well so need to check if it's a player controller as AI only logs out if evicted and already handled that
	if (Exiting && Exiting->IsPlayerController())
	{
		HandlePlayerLeaving(Exiting);
	}

	Super::Logout(Exiting);

	// If fall below minimum number of players for a game in progress, return to the main menu
	if (MatchShouldBeAbandoned())
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log,
			TEXT("%s: Logout - Returning to main menu as number of players=%d is less than minimum=%d"),
			*GetName(), GetNumberOfActivePlayers(), MinNumberOfPlayers
		);
		ReturnToMainMenuHost();
	}
}

bool APaperGolfGameModeBase::MatchIsActive() const
{
	// When waiting to start return true as match is being set up and need to allow players to login
	if (!IsMatchInProgress() && MatchState != MatchState::WaitingToStart && MatchState != MatchState::EnteringMap)
	{
		return false;
	}

	// make sure world is not shutting down
	return !IsWorldBeingDestroyed();
}

bool APaperGolfGameModeBase::IsWorldBeingDestroyed() const
{
	auto World = GetWorld();
	if (!IsValid(World))
	{
		return true;
	}

	return World->bIsTearingDown;
}

bool APaperGolfGameModeBase::MatchShouldBeAbandoned() const
{
	// TODO: Fix mismatch between NumPlayers and GetNumberOfActivePlayers that will also consider bots in derived classes
	return MatchIsActive() && GetNumberOfActivePlayers() < MinNumberOfPlayers;
}

bool APaperGolfGameModeBase::MatchIsJoinable() const
{
	// Can join the match and kick a bot if the number of active human players is less than number of total desired players (Players + Bots)
	const bool bResult = MatchIsActive() && NumPlayers < GetMaximumNumberOfPlayers();
	UE_VLOG_UELOG(this, LogPaperGolfGame, Verbose, TEXT("%s: MatchIsJoinable - %s - MatchIsActive=%s; NumPlayers()=%d; DesiredNumberOfPlayers=%d; DesiredNumberOfBotPlayers=%d; MaxPlayers=%d"),
		*GetName(), LoggingUtils::GetBoolString(bResult), LoggingUtils::GetBoolString(MatchIsActive()), NumPlayers, DesiredNumberOfPlayers, DesiredNumberOfBotPlayers, MaxPlayers);
	return bResult;
}

void APaperGolfGameModeBase::HandleMatchIsWaitingToStart()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: HandleMatchIsWaitingToStart"), *GetName());

	Super::HandleMatchIsWaitingToStart();

	// Create the initial bots configured in the game options as soon as the game is starting
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

	// Copying from AGameMode::RestartGame
	if (GameSession->CanRestartGame())
	{
		if (GetMatchState() == MatchState::LeavingMap)
		{
			return;
		}

		FString MapName = GetWorld()->GetMapName();
		MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix); // Remove any prefix if necessary

		const FString& GameModeName = GetClass()->GetClassPathName().ToString();

		// Pass along updated options from current game state
		GetWorld()->ServerTravel(FString::Printf(TEXT("%s?game=%s?%s%d?%s%d?%s%d"),
			*MapName,
			*GameModeName,
			PG::GameModeOptions::NumPlayers, NumPlayers,
			PG::GameModeOptions::NumBots, NumBots,
			PG::GameModeOptions::AllowBots, bAllowBots),
		GetTravelType());
	}
}

void APaperGolfGameModeBase::AbortMatch()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: AbortMatch"), *GetName());

	Super::AbortMatch();
}

AGolfAIController* APaperGolfGameModeBase::AddBot()
{
	// TODO: This needs to be revisited with player names
	const auto CreatedBot = CreateBot(NumBots + 1);

	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, 
		TEXT("%s: AddBot - %s - NumPlayers=%d; DesiredNumberOfPlayers=%d"),
		*GetName(), *LoggingUtils::GetName(CreatedBot), GetNumberOfActivePlayers() + (CreatedBot ? 1 : 0), DesiredNumberOfPlayers
	);

	return CreatedBot;
}

AGolfAIController* APaperGolfGameModeBase::ReplaceLeavingPlayerWithBot(AController* Player)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: ReplaceLeavingPlayerWithBot - Player=%s"), *GetName(), *LoggingUtils::GetName(Player));

	const auto CreatedBot = AddBot();
	if (!CreatedBot)
	{
		return nullptr;
	}

	if (Player && Player->PlayerState)
	{
		auto BotPlayerState = CreatedBot->PlayerState;
		if (BotPlayerState)
		{
			// Bot takes over player name
			BotPlayerState->SetPlayerName(Player->PlayerState->GetPlayerName());
		}
	}

	// Rest of state properties will be copied later
	
	return CreatedBot;
}

AController* APaperGolfGameModeBase::ChooseBotToEvict() const
{
	auto PaperGolfGameState = GetGameState<APaperGolfGameStateBase>();

	if (!ensureMsgf(PaperGolfGameState, TEXT("%s: PaperGolfGameState=%s is not APaperGolfGameStateBase"), *GetName(), *LoggingUtils::GetName(GameState)))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: PaperGolfGameState=%s is not APaperGolfGameStateBase"),
			*GetName(), *LoggingUtils::GetName(GameState));
		return nullptr;
	}

	AGolfPlayerState* PlayerStateToEvict{};

	for (auto PlayerState : PaperGolfGameState->PlayerArray)
	{
		auto GolfPlayerState = Cast<AGolfPlayerState>(PlayerState);
		if (!GolfPlayerState || !GolfPlayerState->IsABot())
		{
			continue;
		}

		// If GolfPlayerState CompareByScore returns true with PlayerStateToEvict, then GolfPlayerState is better and should be preferred to be evicted
		if (!PlayerStateToEvict || GolfPlayerState->CompareByScore(*PlayerStateToEvict))
		{
			PlayerStateToEvict = GolfPlayerState;
		}
	}

	return PlayerStateToEvict ? PlayerStateToEvict->GetOwningController() : nullptr;
}

void APaperGolfGameModeBase::HandlePlayerLeaving(AController* LeavingPlayer)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: HandlePlayerLeaving - LeavingPlayer=%s"), *GetName(), *LoggingUtils::GetName(LeavingPlayer));

	if (!bAllowBots || !MatchIsActive())
	{
		OnPlayerLeft(LeavingPlayer);
	}
	else
	{
#if PG_ALLOW_BOT_REPLACE_PLAYER
		if (AController* NewPlayer = ReplaceLeavingPlayerWithBot(LeavingPlayer); NewPlayer)
		{
			ReplacePlayer(LeavingPlayer, NewPlayer);
		}
		else
		{
			OnPlayerLeft(LeavingPlayer);
		}

#else
		OnPlayerLeft(LeavingPlayer);
#endif

	}

	// Be sure to destroy the leaving player's pawn
	// TODO: Just like with the bot, it would be better to just swap this out
	if (auto GolfController = Cast<IGolfController>(LeavingPlayer); GolfController && GolfController->HasPaperGolfPawn())
	{
		if (auto Pawn = GolfController->GetPaperGolfPawn(); Pawn)
		{
			Pawn->Destroy();
		}
	}
}

void APaperGolfGameModeBase::HandlePlayerJoining(AController* NewPlayer)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: HandlePlayerJoining - NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(NewPlayer));

	if (!IsValid(NewPlayer))
	{
		return;
	}

	// Note that NumPlayers already incremented so we would be full if >
	const bool bNotFull = GetTotalNumberOfPlayers() <= GetMaximumNumberOfPlayers();

	// TODO: We may want to make it configurable to be able to join a course in progress and not force player to be spectator
	const bool bCourseCanBeStarted = CanCourseBeStarted();

	if (bNotFull && bCourseCanBeStarted)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: HandlePlayerJoining - Not full - NumPlayers=%d; DesiredNumberOfPlayers=%d"),
			*GetName(), GetTotalNumberOfPlayers(), GetMaximumNumberOfPlayers());

		OnPlayerJoined(NewPlayer);
		return;
	}

#if PG_ALLOW_BOT_REPLACE_PLAYER
	// If we are full or course has already started we need to see if we can remove a bot
	AController* const BotToEvict = ChooseBotToEvict();
#else
	AController* const BotToEvict = nullptr;
#endif

	ConfigureJoinedPlayerState(*NewPlayer);

	if (BotToEvict)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: HandlePlayerJoining - Full - Replacing BotToEvict=%s with NewPlayer=%s"),
			*GetName(), *LoggingUtils::GetName(BotToEvict), *LoggingUtils::GetName(NewPlayer));

		ReplacePlayer(BotToEvict, NewPlayer);
		DestroyBot(BotToEvict);
	}
	else if (auto GolfPlayerState = NewPlayer->GetPlayerState<AGolfPlayerState>(); ensure(GolfPlayerState))
	{
		GolfPlayerState->SetSpectatorOnly();
		UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: HandlePlayerJoining - Full - No BotToEvict - NewPlayer=%s joining as spectator only"), *GetName(), *LoggingUtils::GetName(NewPlayer));
		OnPlayerJoined(NewPlayer);
	}
	else
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: HandlePlayerJoining - Full - No BotToEvict - NewPlayer=%s - Invalid player state"),
			*GetName(), *LoggingUtils::GetName(NewPlayer->GetPlayerState<APlayerState>()));

		KickPlayer(NewPlayer, MatchJoinFailureMessage);
	}
}

void APaperGolfGameModeBase::DestroyBot(AController* BotToEvict)
{
	if (!BotToEvict)
	{
		return;
	}

	if (auto BotToEvictGolfController = Cast<IGolfController>(BotToEvict); ensure(BotToEvictGolfController) && BotToEvictGolfController->HasPaperGolfPawn())
	{
		if (auto Pawn = BotToEvictGolfController->GetPaperGolfPawn(); Pawn)
		{
			Pawn->Destroy();
		}
	}
	BotToEvict->Destroy();
}

void APaperGolfGameModeBase::ReplacePlayer(AController* LeavingPlayer, AController* NewPlayer)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: ReplacePlayer - LeavingPlayer=%s; NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(LeavingPlayer), *LoggingUtils::GetName(NewPlayer));

	if (NewPlayer && LeavingPlayer)
	{
		// Copy player state data from leaving player to new player
		const auto NewPlayerState = NewPlayer->GetPlayerState<AGolfPlayerState>();
		const auto LeavingPlayerState = LeavingPlayer->GetPlayerState<AGolfPlayerState>();
		if (NewPlayerState && LeavingPlayerState)
		{
			// Check for shot in progress and if so, undo the stroke since the replacing player will start back
			// from the beginning of the shot
			if (auto LeavingPlayerGolfController = Cast<IGolfController>(LeavingPlayer); 
				ensure(LeavingPlayerGolfController))
			{
				if (LeavingPlayerGolfController->IsActiveShotInProgress())
				{
					UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s:  LeavingPlayer=%s - Undoing shot as replacing player when shot was in progress for NewPlayer=%s"),
						*GetName(), *LoggingUtils::GetName(LeavingPlayer), *LoggingUtils::GetName(NewPlayer));
					LeavingPlayerState->UndoShot();
				}
			}
			else
			{
				UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s:  LeavingPlayer=%s - Cannot undo check for undo stroke as cannot cast to IGolfController"),
					*GetName(), *LoggingUtils::GetName(LeavingPlayer));
			}

			NewPlayerState->CopyGameStateProperties(LeavingPlayerState);
		}
		else
		{
			UE_VLOG_UELOG(this, LogPaperGolfGame, Warning, TEXT("%s:  LeavingPlayer=%s; NewPlayer=%s - Cannot copy player state from %s to %s"),
				*GetName(),
				*LeavingPlayer->GetName(), *NewPlayer->GetName(),
				*LoggingUtils::GetName(LeavingPlayer->PlayerState), *LoggingUtils::GetName(NewPlayer->PlayerState)
			);
		}
	}

	OnPlayerReplaced(LeavingPlayer, NewPlayer);
}

void APaperGolfGameModeBase::OnPlayerReplaced(AController* LeavingPlayer, AController* NewPlayer)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: OnPlayerReplaced - LeavingPlayer=%s; NewPlayer=%s"),
		*GetName(), *LoggingUtils::GetName(LeavingPlayer), *LoggingUtils::GetName(NewPlayer));

	OnPlayerLeft(LeavingPlayer);
	OnPlayerJoined(NewPlayer);
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
		if (auto Controller = CreateBot(i + 1); Controller)
		{
			OnPlayerJoined(Controller);
		}
	}
}

AGolfAIController* APaperGolfGameModeBase::CreateBot(int32 BotNumber)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: CreateBot - BotNumber=%d"), *GetName(), BotNumber);

	if(!ensureMsgf(AIControllerClass, TEXT("AIControllerClass is not set!")))
	{
		return nullptr;
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
		return nullptr;
	}

	InitBot(*AIC, BotNumber);

	return AIC;
}

void APaperGolfGameModeBase::InitBot(AGolfAIController& AIController, int32 BotNumber)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitBot - BotNumber=%d"), *GetName(), BotNumber);

	if(auto PlayerState = AIController.GetGolfPlayerState(); ensureMsgf(PlayerState, TEXT("%s: No player state for %s - BotNumber=%d"), *GetName(), *AIController.GetName(), BotNumber))
	{
		// TODO: Draw from a random list of 90s names
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
