// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "GameMode/PGTutorialGameMode.h"

#include "State/PaperGolfGameStateBase.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PaperGolfLogging.h"

#include "Library/PaperGolfGameUtilities.h"
#include "Subsystems/TutorialTrackingSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PGTutorialGameMode)

void APGTutorialGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	NextCourseOptions = UPaperGolfGameUtilities::DecodeNextCourseOptions(Options);

	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: InitGame: NextCourseOptions=%s"), *GetName(), *NextCourseOptions);
}

void APGTutorialGameMode::InitGameState()
{
	Super::InitGameState();

	auto PaperGolfGameState = GetGameState<APaperGolfGameStateBase>();

	if (!ensureMsgf(PaperGolfGameState, TEXT("%s: PaperGolfGameState=%s is not APaperGolfGameStateBase"), *GetName(), *LoggingUtils::GetName(GameState)))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: PaperGolfGameState=%s is not APaperGolfGameStateBase"),
			*GetName(), *LoggingUtils::GetName(GameState));
		return;
	}

	// Don't show scores in a tutorial - it is a low-stakes practice session
	PaperGolfGameState->SetShowScoresHUD(false);
}

void APGTutorialGameMode::CourseCompletionNextAction()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: CourseCompletionNextAction: NextCourseOptions=%s"), *GetName(), *NextCourseOptions);

	MarkTutorialComplete();

	if (NextCourseOptions.IsEmpty())
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Warning, TEXT("%s: CourseCompletionNextAction: NextCourseOptions is empty - defaulting to base class behavior"),
			*GetName());
		Super::CourseCompletionNextAction();
		return;
	}

	if (auto World = GetWorld(); ensure(World))
	{
		// TODO: This causes a crash in non editor builds - no players are registers when the course starts
		// Assertion failed: ActivePlayerIndex >= 0 && ActivePlayerIndex < Players.Num() [Source\PaperGolf\Private\GameMode\GolfTurnBasedDirectorComponent.cpp] [Line: 485]
		bUseSeamlessTravel = false; // !WITH_EDITOR;

		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: CourseCompletionNextAction: Loading next course with options=%s"),
			*GetName(), *NextCourseOptions);
		World->ServerTravel(NextCourseOptions);
	}
}

void APGTutorialGameMode::MarkTutorialComplete()
{
	auto GameInstance = GetGameInstance();
	if (!ensure(GameInstance))
	{
		return;
	}

	auto TutorialTrackingSubsystem = GameInstance->GetSubsystem<UTutorialTrackingSubsystem>();
	
	if (ensure(TutorialTrackingSubsystem))
	{
		TutorialTrackingSubsystem->MarkTutorialSeen();
	}
}
