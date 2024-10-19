// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Subsystems/TutorialTrackingSubsystem.h"

#include "VisualLogger/VisualLogger.h"

#include "Logging/LoggingUtils.h"

#include "PGUILogging.h"

#include "GameFramework/PlayerController.h"

#include "Tutorial/ShotTutorialAction.h"
#include "Tutorial/AimTutorialAction.h"
#include "Tutorial/ShotPreviewTutorialAction.h"
#include "Tutorial/ShotSpinTutorialAction.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TutorialTrackingSubsystem)

UTutorialTrackingSubsystem::UTutorialTrackingSubsystem()
{
	HoleFlybySeen.SetNum(MaxHoles);
}

void UTutorialTrackingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: Initialize"), *GetName());
	Super::Initialize(Collection);
}

void UTutorialTrackingSubsystem::MarkAllHoleFlybysSeen(bool bSeen)
{
	for (auto& seen : HoleFlybySeen)
	{
		seen = bSeen;
	}
}

void UTutorialTrackingSubsystem::InitializeTutorialActions(APlayerController* PlayerController)
{
	UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: InitializeTutorialActions: PlayerController=%s"), *GetName(), *LoggingUtils::GetName(PlayerController));

	if (!ensure(PlayerController))
	{
		UE_VLOG_UELOG(this, LogPGUI, Error, TEXT("%s: InitializeTutorialActions: PlayerController is nullptr"), *GetName());
		return;
	}

	CurrentTutorialAction = nullptr;
	TutorialActions.Reset();

	TutorialActions.Add(NewObject<UShotTutorialAction>(PlayerController));
	TutorialActions.Add(NewObject<UAimTutorialAction>(PlayerController));
	TutorialActions.Add(NewObject<UShotPreviewTutorialAction>(PlayerController));
	TutorialActions.Add(NewObject<UShotSpinTutorialAction>(PlayerController));
}

void UTutorialTrackingSubsystem::DisplayNextTutorial(APlayerController* PlayerController)
{
	UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: DisplayNextTutorial: PlayerController=%s"), *GetName(), *LoggingUtils::GetName(PlayerController));

	for (auto TutorialAction : TutorialActions)
	{
		if (IsValid(TutorialAction) && TutorialAction->IsRelevant())
		{
			CurrentTutorialAction = TutorialAction;
			TutorialAction->Execute();
			break;
		}
	}
}

void UTutorialTrackingSubsystem::HideActiveTutorial()
{
	UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: HideActiveTutorial"), *GetName());

	if (IsValid(CurrentTutorialAction) && CurrentTutorialAction->IsActive())
	{
		CurrentTutorialAction->Abort();
	}

	CurrentTutorialAction = nullptr;
}

void UTutorialTrackingSubsystem::DestroyTutorialActions()
{
	UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: DestroyTutorialActions"), *GetName());

	HideActiveTutorial();
	TutorialActions.Reset();
}
