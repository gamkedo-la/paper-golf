// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Tutorial/TutorialAction.h"

#include "Kismet/GameplayStatics.h"

#include "UI/PGHUD.h"

#include "Pawn/PaperGolfPawn.h"

#include "GameFramework/PlayerController.h"
#include "VisualLogger/VisualLogger.h"

#include "Logging/LoggingUtils.h"

#include "Utils/ArrayUtils.h"

#include "PGUILogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TutorialAction)

void UTutorialAction::Abort()
{
	UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: Aborted"), *GetName());
	
	if (!MessageTimerHandle.IsValid())
	{
		return;
	}

	if (auto World = GetWorld(); MessageTimerHandle.IsValid() && World)
	{
		World->GetTimerManager().ClearTimer(MessageTimerHandle);
	}

	// TODO: This may dismiss the wrong message if another message replaced this one
	if (auto HUD = GetHUD(); HUD)
	{
		HUD->RemoveActiveMessageWidget();
	}
}

void UTutorialAction::ShowMessages(const TArray<FText>& Messages, float MessageDuration)
{
	if(!ensure(!Messages.IsEmpty()))
	{
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Error, TEXT("%s: ShowMessages: Messages is empty"), *GetName());
		return;
	}

	auto HUD = GetHUD();
	if (!HUD)
	{
		return;
	}

	LastMessageIndex = 0;
	if (MessageDuration <= 0)
	{
		MessageDuration = MaxMessageDuration;
	}

	UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: ShowMessages: Messages=%s; MessageDuration=%fs"), *GetName(), *PG::ToString(Messages), MessageDuration);

	HUD->GetWorldTimerManager().SetTimer(MessageTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this, Messages, WeakHUD = MakeWeakObjectPtr(HUD)]()
	{
		// TODO: This may dismiss the wrong message if another message replaced this one
		// Maybe the DisplayMessageWidgetWithText should have an optional show time parameter and let the HUD automatically dismiss it
		if (auto HUD = WeakHUD.Get())
		{
			if (LastMessageIndex > 0)
			{
				HUD->RemoveActiveMessageWidget();
			}

			if (LastMessageIndex < Messages.Num())
			{
				UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: ShowMessages: %d/%d - %s"), *GetName(),
					LastMessageIndex + 1, Messages.Num(), *Messages[LastMessageIndex].ToString());

				HUD->DisplayMessageWidgetWithText(Messages[LastMessageIndex]);
				OnMessageShown(LastMessageIndex, Messages.Num());

				++LastMessageIndex;
			}
			else
			{
				UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: ShowMessages: All messages shown"), *GetName());
				HUD->GetWorldTimerManager().ClearTimer(MessageTimerHandle);

				if (ShouldMarkCompletedOnLastMessageDismissed())
				{
					MarkCompleted();
				}
			}
		}
	}), MessageDuration, true, 0.0f);
}

// TODO: This code is essentially copied from PaperGolfGameUtilities::GetLocalPlayerController.

APlayerController* UTutorialAction::GetPlayerController() const
{
	auto GameInstance = UGameplayStatics::GetGameInstance(GetWorld());
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetFirstLocalPlayerController();
}

APaperGolfPawn* UTutorialAction::GetPlayerPawn() const
{
	auto PC = GetPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	return Cast<APaperGolfPawn>(PC->GetPawn());
}

APGHUD* UTutorialAction::GetHUD() const
{
	auto PC = GetPlayerController();
	if (!ensure(PC))
	{
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Error, TEXT("%s: GetHUD - Failed to get player controller"), *GetName());
		return nullptr;
	}

	return Cast<APGHUD>(PC->GetHUD());
}

void UTutorialAction::MarkCompleted()
{
	UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: Marked as completed"), *GetName());

	bIsCompleted = true;
}