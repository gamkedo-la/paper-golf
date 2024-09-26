// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "GameMode/PGTurnBasedGameMode.h"

#include "GameMode/GolfTurnBasedDirectorComponent.h"
#include "State/PaperGolfGameStateBase.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PaperGolfLogging.h"

#include "Controller/GolfAIController.h"

#include "Interfaces/GolfController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PGTurnBasedGameMode)


APGTurnBasedGameMode::APGTurnBasedGameMode()
{
	TurnBasedDirectorComponent = CreateDefaultSubobject<UGolfTurnBasedDirectorComponent>(TEXT("TurnBasedDirector"));
}

void APGTurnBasedGameMode::OnPlayerLeft(AController* Exiting)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: OnPlayerLeft - Exiting=%s"), *GetName(), *LoggingUtils::GetName(Exiting));

	Super::OnPlayerLeft(Exiting);

	// Only remove players that implement GolfController - this is mostly for PIE simulated players but could be any spectators that join as well
	if (auto GolfController = Cast<IGolfController>(Exiting); GolfController)
	{
		check(TurnBasedDirectorComponent);

		TurnBasedDirectorComponent->RemovePlayer(Exiting);
	}
}

void APGTurnBasedGameMode::OnPlayerReplaced(AController* LeavingPlayer, AController* NewPlayer)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: OnPlayerReplaced - LeavingPlayer=%s, NewPlayer=%s"), *GetName(), *LoggingUtils::GetName(LeavingPlayer), *LoggingUtils::GetName(NewPlayer));

	auto LeavingGolfController = Cast<IGolfController>(LeavingPlayer);
	auto NewGolfController = Cast<IGolfController>(NewPlayer);

	if (LeavingGolfController && NewGolfController)
	{
		check(TurnBasedDirectorComponent);
		TurnBasedDirectorComponent->ReplacePlayer(LeavingPlayer, NewPlayer);
	}
	else
	{
		// This will call OnPlayerLeft and OnPlayerJoined for applicable players
		Super::OnPlayerReplaced(LeavingPlayer, NewPlayer);
	}
}

void APGTurnBasedGameMode::OnPlayerJoined(AController* NewPlayer)
{
	Super::OnPlayerJoined(NewPlayer);

	// Only add players that implement GolfController - this is mostly for PIE simulated players but could be any specatators that join as well
	if (auto GolfController = Cast<IGolfController>(NewPlayer); GolfController)
	{
		check(TurnBasedDirectorComponent);

		TurnBasedDirectorComponent->AddPlayer(NewPlayer);
	}
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

bool APGTurnBasedGameMode::DelayStartWithTimer() const
{
	check(TurnBasedDirectorComponent);

	return Super::DelayStartWithTimer() || TurnBasedDirectorComponent->IsSkippingHumanPlayers();
}

int32 APGTurnBasedGameMode::GetNumberOfActivePlayers() const
{
	check(TurnBasedDirectorComponent);

	return TurnBasedDirectorComponent->GetNumberOfActivePlayers();
}
