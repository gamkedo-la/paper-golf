// Copyright Game Salutes. All Rights Reserved.


#include "GameMode/PGTurnBasedGameMode.h"

#include "GameMode/GolfTurnBasedDirectorComponent.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PaperGolfLogging.h"

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

	// FIXME: Determine better criteria for number of players before starting the match
	// Also need to handle multiple holes

	TurnBasedDirectorComponent->StartHole();
}
