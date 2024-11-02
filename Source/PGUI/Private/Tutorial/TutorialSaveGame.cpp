// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Tutorial/TutorialSaveGame.h"
#include "Tutorial/TutorialAction.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PGUILogging.h"

#include "Utils/SaveGameUtils.h"

#include "Kismet/GameplayStatics.h"

#include "Subsystems/TutorialTrackingSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TutorialSaveGame)

const FString UTutorialSaveGame::SlotName = TEXT("TutorialSaveGame");

void UTutorialSaveGame::Save(const UObject* WorldContextObject)
{
	UE_VLOG_UELOG(this, LogPGUI, Log, TEXT("%s: Save"), *GetName());

	// Setting this in field or constructor overwrites the saved value
	Version = CurrentVersion;

	auto TutorialTrackingSubsystem = GetTutorialTrackingSubsystem(WorldContextObject);
	if (!TutorialTrackingSubsystem)
	{
		return;
	}

	bTutorialHoleSeen = TutorialTrackingSubsystem->IsTutorialSeen();

	// Assume additive so just add to the set
	// Delete the saved state to reset it
	for (auto Tutorial : TutorialTrackingSubsystem->TutorialActions)
	{
		if (Tutorial->IsCompleted())
		{
			CompletedTutorials.Add(Tutorial->GetActionName());
		}
	}

	SaveGameUtils::SaveGame(this);
}

void UTutorialSaveGame::RestoreState(const UObject* WorldContextObject) const
{
	// Change bool on subsystem and filter out completed tutorials
	// We will have to assume this is additive
	auto TutorialTrackingSubsystem = GetTutorialTrackingSubsystem(WorldContextObject);
	if (!TutorialTrackingSubsystem)
	{
		return;
	}

	TutorialTrackingSubsystem->bTutorialHoleSeen = bTutorialHoleSeen;
}

bool UTutorialSaveGame::IsTutorialCompleted(const UTutorialAction& Tutorial) const
{
	return CompletedTutorials.Contains(Tutorial.GetActionName());
}

UTutorialTrackingSubsystem* UTutorialSaveGame::GetTutorialTrackingSubsystem(const UObject* WorldContextObject) const
{
	auto GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);

	if (!ensure(GameInstance))
	{
		return nullptr;
	}

	auto TutorialTrackingSubsystem = GameInstance->GetSubsystem<UTutorialTrackingSubsystem>();
	ensure(TutorialTrackingSubsystem);

	return TutorialTrackingSubsystem;
}
