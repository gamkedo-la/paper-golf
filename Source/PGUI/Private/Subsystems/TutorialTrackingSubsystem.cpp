// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Subsystems/TutorialTrackingSubsystem.h"

#include "VisualLogger/VisualLogger.h"

#include "PGConstants.h"

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

	RegisterResetTutorialConsoleCommand();
	RestoreTutorialState();
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

void UTutorialTrackingSubsystem::RestoreTutorialState()
{
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

void UTutorialTrackingSubsystem::InitializeTutorialActions(UTutorialConfigDataAsset* InTutorialConfig, APlayerController* InPlayerController)
{
	UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: InitializeTutorialActions: InTutorialConfig=%s; InPlayerController=%s")
		, *GetName(), *LoggingUtils::GetName(InTutorialConfig), *LoggingUtils::GetName(InPlayerController));

	if (!ensure(InPlayerController))
	{
		UE_VLOG_UELOG(this, LogPGUI, Error, TEXT("%s: InitializeTutorialActions: InPlayerController is nullptr"), *GetName());
		return;
	}

	LastPlayerController = InPlayerController;
	TutorialConfig = InTutorialConfig;

	CurrentTutorialAction = nullptr;
	TutorialActions.Reset();

	RegisterTutorialAction<UShotTutorialAction>();
	RegisterTutorialAction<UAimTutorialAction>();
	RegisterTutorialAction<UShotPreviewTutorialAction>();
	RegisterTutorialAction<UShotSpinTutorialAction>();
}

template<std::derived_from<UTutorialAction> T>
void UTutorialTrackingSubsystem::RegisterTutorialAction()
{
	if (!TutorialConfig)
	{
		UE_VLOG_UELOG(this, LogPGUI, Warning, TEXT("%s: RegisterTutorialAction: SkipAll - TutorialConfig is NULL"), *GetName());
		return;
	}

	const auto PlayerController = LastPlayerController.Get();
	if (!PlayerController)
	{
		UE_VLOG_UELOG(this, LogPGUI, Warning, TEXT("%s: RegisterTutorialAction: SkipAll - PlayerController is NULL"), *GetName());
		return;
	}
	
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

void UTutorialTrackingSubsystem::ResetTutorialState()
{
	if (SaveGameUtils::DeleteSavedGame<UTutorialSaveGame>())
	{
		UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: DeleteSavedGame - Tutorial save deleted"), *GetName());
	}
	else
	{
		UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: DeleteSavedGame - Tutorial save not found"), *GetName());
	}

	bTutorialHoleSeen = false;

	RestoreTutorialState();

	// Reinitialize tutorial actions
	const auto PlayerController = LastPlayerController.Get();
	if (IsValid(TutorialConfig) && PlayerController)
	{
		InitializeTutorialActions(TutorialConfig, PlayerController);
	}

	UE_VLOG_UELOG(this, LogPGUI, Display, TEXT("%s: ResetTutorialState"), *GetName());
}

#if PG_DEBUG_ENABLED

namespace
{
	constexpr const auto ResetTutorialConsoleName = TEXT("pg.tutorial.reset");
}

void UTutorialTrackingSubsystem::RegisterResetTutorialConsoleCommand()
{
	auto& ConsoleManager = IConsoleManager::Get();

	// Must remove existing registration to avoid warnings
	if (auto ExistingCommand = ConsoleManager.FindConsoleObject(ResetTutorialConsoleName); ExistingCommand)
	{
		UE_LOG(LogPGUI, Log, TEXT("%s: Unregistering existing console command for %s"), *GetName(), ResetTutorialConsoleName);

		ConsoleManager.UnregisterConsoleObject(ExistingCommand);
	}

	UE_LOG(LogPGUI, Log, TEXT("%s: Registering console command for %s"), *GetName(), ResetTutorialConsoleName);

	ConsoleManager.RegisterConsoleCommand(
		ResetTutorialConsoleName,
		TEXT("Deletes saved state of tutorial so it can be replayed"),
		FConsoleCommandDelegate::CreateUObject(this, &UTutorialTrackingSubsystem::ResetTutorialState),
		ECVF_Default
	);
}

#else

void UTutorialTrackingSubsystem::RegisterResetTutorialConsoleCommand(){}

#endif
