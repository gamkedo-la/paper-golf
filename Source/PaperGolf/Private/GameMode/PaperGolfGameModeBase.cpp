// Copyright Game Salutes. All Rights Reserved.


#include "GameMode/PaperGolfGameModeBase.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PaperGolfLogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfGameModeBase)

APaperGolfGameModeBase::APaperGolfGameModeBase()
{
	// We will start the game when the desired number of players have joined or other time out happens which should trigger an AI bot to be spawned in their place
	// If a player drop, then they should be replaced with a bot until another player joins
	bDelayedStart = true;
	bStartPlayersAsSpectators = true;
}

void APaperGolfGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitGame - MapName=%s, Options=%s; DefaultDesiredNumberOfPlayers=%d"), *GetName(), *MapName, *Options, DefaultDesiredNumberOfPlayers);

	Super::InitGame(MapName, Options, ErrorMessage);

	// TODO: Override desired number of players
	DesiredNumberOfPlayers = DefaultDesiredNumberOfPlayers;
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
}

bool APaperGolfGameModeBase::ReadyToStartMatch_Implementation()
{
	bool bReady{};
	if (GetMatchState() == MatchState::WaitingToStart)
	{
		bReady = GetTotalNumberOfPlayers() == DesiredNumberOfPlayers;
	}

	// TODO: We may want to manually start the match after a countdown timer or something - this is checked on Tick in the AGameMode base class and will immediately start the match
	// (Calling StartMatch) when this returns true
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: ReadyToStartMatch_Implementation: %s"), *GetName(), LoggingUtils::GetBoolString(bReady));

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

void APaperGolfGameModeBase::BeginPlay()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();
}

APawn* APaperGolfGameModeBase::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: SpawnDefaultPawnAtTransform_Implementation - NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(NewPlayer));

	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);
}

AActor* APaperGolfGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: ChoosePlayerStart_Implementation - Player=%s"), *GetName(), *LoggingUtils::GetName(Player));

	// Unless we implement difficulties with different player starts, we can just use the single player start location
	// If we wanted to do the latter, we may not even need to override this as can just use tag names like "Amateur, Pro, Expert" and then use
	// FindPlayerStart("Amateur") which returns an AActor* that we can pass to RestartPlayer(AController*, AActor*) so it uses that player start
	return Super::ChoosePlayerStart_Implementation(Player);
}

bool APaperGolfGameModeBase::ShouldSpawnAtStartSpot(AController* Player)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: ShouldSpawnAtStartSpot - Player=%s"), *GetName(), *LoggingUtils::GetName(Player));

	return Super::ShouldSpawnAtStartSpot(Player);
}

void APaperGolfGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: HandleStartingNewPlayer_Implementation - NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(NewPlayer));

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

bool APaperGolfGameModeBase::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: UpdatePlayerStartSpot - Player=%s, Portal=%s"), *GetName(), *LoggingUtils::GetName(Player), *Portal);

	return Super::UpdatePlayerStartSpot(Player, Portal, OutErrorMessage);
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

	// TODO: This may be best place to spawn AI bots. That's how ShooterGameMode does it in ShooterGame\Private\Online\ShooterGameMode.cpp
	// However, this is called immediately 
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
