// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "GameMode/PGTurnBasedGameMode.h"

#include "GameMode/GolfTurnBasedDirectorComponent.h"
#include "State/PaperGolfGameStateBase.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PaperGolfLogging.h"

#include "Controller/GolfAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PGTurnBasedGameMode)


APGTurnBasedGameMode::APGTurnBasedGameMode()
{
	TurnBasedDirectorComponent = CreateDefaultSubobject<UGolfTurnBasedDirectorComponent>(TEXT("TurnBasedDirector"));
}

void APGTurnBasedGameMode::Logout(AController* Exiting)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: Logout - Exiting=%s"), *GetName(), *LoggingUtils::GetName(Exiting));

	Super::Logout(Exiting);

	check(TurnBasedDirectorComponent);

	TurnBasedDirectorComponent->RemovePlayer(Exiting);
}

void APGTurnBasedGameMode::OnPostLogin(AController* NewPlayer)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: OnPostLogin - NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(NewPlayer));

	Super::OnPostLogin(NewPlayer);

	check(TurnBasedDirectorComponent);

	TurnBasedDirectorComponent->AddPlayer(NewPlayer);
}

void APGTurnBasedGameMode::OnGameStart()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: OnGameStart"), *GetName());

	StartHole(StartHoleNumber);
}

void APGTurnBasedGameMode::StartHole(int32 HoleNumber)
{
	check(TurnBasedDirectorComponent);

	if (auto GolfState = GetGameState<APaperGolfGameStateBase>(); ensure(GolfState))
	{
		GolfState->SetCurrentHoleNumber(HoleNumber);
		TurnBasedDirectorComponent->StartHole();
	}
}

void APGTurnBasedGameMode::OnBotSpawnedIntoGame(AGolfAIController& AIController, int32 BotNumber)
{
	// Note that the Login functions are not called when AI controllers are spawned. These are only for player controllers joining the session, so add to the turn component manually here
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: OnBotSpawnedIntoGame - AIController=%s, BotNumber=%d"), *GetName(), *AIController.GetName(), BotNumber);

	check(TurnBasedDirectorComponent);

	TurnBasedDirectorComponent->AddPlayer(&AIController);
}

bool APGTurnBasedGameMode::DelayStartWithTimer() const
{
	check(TurnBasedDirectorComponent);

	return Super::DelayStartWithTimer() || TurnBasedDirectorComponent->IsSkippingHumanPlayers();
}
