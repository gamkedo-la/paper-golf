// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Subsystems/TutorialTrackingSubsystem.h"

#include "VisualLogger/VisualLogger.h"

#include "Logging/LoggingUtils.h"

#include "PGUILogging.h"

#include "GameFramework/PlayerController.h"

#include "Tutorial/TutorialConfigDataAsset.h"
#include "Tutorial/ShotTutorialAction.h"
#include "Tutorial/AimTutorialAction.h"
#include "Tutorial/ShotPreviewTutorialAction.h"
#include "Tutorial/ShotSpinTutorialAction.h"

#include "Tutorial/TutorialSaveGame.h"

#include "Utils/SaveGameUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TutorialTrackingSubsystem)

UTutorialTrackingSubsystem::UTutorialTrackingSubsystem()
{
	HoleFlybySeen.SetNum(MaxHoles);
}

void UTutorialTrackingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: Initialize"), *GetName());
	Super::Initialize(Collection);

	TutorialSaveGame = SaveGameUtils::GetSavedGame<UTutorialSaveGame>();
	if (TutorialSaveGame)
	{
		UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: Initialize: Restoring saved TutorialSaveGame state"), *GetName());
		TutorialSaveGame->RestoreState(this);
	}
	else
	{
		UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: Initialize: Creating new TutorialSaveGame"), *GetName());
		TutorialSaveGame = SaveGameUtils::CreateSaveGameInstance<UTutorialSaveGame>();
	}
}

void UTutorialTrackingSubsystem::MarkAllHoleFlybysSeen(bool bSeen)
{
	for (auto& seen : HoleFlybySeen)
	{
		seen = bSeen;
	}
}

void UTutorialTrackingSubsystem::SaveTutorialState()
{
	if (TutorialSaveGame)
	{
		TutorialSaveGame->Save(this);
	}
}

void UTutorialTrackingSubsystem::InitializeTutorialActions(UTutorialConfigDataAsset* TutorialConfig, APlayerController* PlayerController)
{
	UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: InitializeTutorialActions: TutorialConfig=%s; PlayerController=%s")
		, *GetName(), *LoggingUtils::GetName(TutorialConfig), *LoggingUtils::GetName(PlayerController));

	if (!ensure(PlayerController))
	{
		UE_VLOG_UELOG(this, LogPGUI, Error, TEXT("%s: InitializeTutorialActions: PlayerController is nullptr"), *GetName());
		return;
	}

	CurrentTutorialAction = nullptr;
	TutorialActions.Reset();

	RegisterTutorialAction<UShotTutorialAction>(TutorialConfig, PlayerController);
	RegisterTutorialAction<UAimTutorialAction>(TutorialConfig, PlayerController);
	RegisterTutorialAction<UShotPreviewTutorialAction>(TutorialConfig, PlayerController);
	RegisterTutorialAction<UShotSpinTutorialAction>(TutorialConfig, PlayerController);
}

template<std::derived_from<UTutorialAction> T>
void UTutorialTrackingSubsystem::RegisterTutorialAction(UTutorialConfigDataAsset* TutorialConfig, APlayerController* PlayerController)
{
	if (auto Tutorial = NewObject<T>(PlayerController); ensure(Tutorial))
	{
		if (!TutorialSaveGame || !TutorialSaveGame->IsTutorialCompleted(*Tutorial))
		{
			Tutorial->Initialize(TutorialConfig);
			TutorialActions.Add(Tutorial);
		}
		else
		{
			UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: RegisterTutorialAction: Tutorial %s is already completed"),
				*GetName(), *LoggingUtils::GetName(Tutorial));
		}
	}
}

void UTutorialTrackingSubsystem::MarkTutorialSeen()
{
	bTutorialHoleSeen = true;

	SaveTutorialState();
}

void UTutorialTrackingSubsystem::DisplayNextTutorial(APlayerController* PlayerController)
{
	UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: DisplayNextTutorial: PlayerController=%s"), *GetName(), *LoggingUtils::GetName(PlayerController));

	SaveTutorialState();

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

	SaveTutorialState();

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
