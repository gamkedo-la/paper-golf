// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "Controller/BasePlayerController.h"

#include "Logging/LoggingUtils.h"
#include "PGPlayerLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasePlayerController)

void ABasePlayerController::BeginPlay()
{
	Super::BeginPlay();

	InitDebugDraw();
}

void ABasePlayerController::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	CleanupDebugDraw();
}

void ABasePlayerController::SetInputModeUI(UUserWidget* FocusWidget)
{
	// Set to game and ui so that we can process input actions in the UI
	FInputModeGameAndUI InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputModeData);

	if (FocusWidget)
	{
		InputModeData.SetWidgetToFocus(FocusWidget->TakeWidget());
	}

	SetShowMouseCursor(true);
}

void ABasePlayerController::SetInputModeGame()
{
	FInputModeGameOnly InputModeData;
	SetInputMode(InputModeData);

	SetShowMouseCursor(false);
}

void ABasePlayerController::PauseGame(UUserWidget* FocusWidget)
{
	SetInputModeUI(FocusWidget);

	SetPaused(true);

	OnPauseGame(FocusWidget);
	BlueprintPauseGame(FocusWidget);
}

void ABasePlayerController::ResumeGame()
{
	SetPaused(false);

	SetInputModeGame();

	OnResumeGame();
	BlueprintResumeGame();
}

bool ABasePlayerController::IsGamePaused() const
{
	return UGameplayStatics::IsGamePaused(this);
}

void ABasePlayerController::SetPaused(bool bPaused)
{
	if (!CanPauseGame())
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("Cannot pause game in Net Mode: %d"), GetNetMode());
		return;
	}

	UGameplayStatics::SetGamePaused(GetWorld(), bPaused);
}


#pragma region Visual Logger

#if ENABLE_VISUAL_LOG

void ABasePlayerController::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{

}

void ABasePlayerController::InitDebugDraw()
{
	// Ensure that state logged regularly so we see the updates in the visual logger

	FTimerDelegate DebugDrawDelegate = FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (ShouldCaptureDebugSnapshot())
		{
			UE_VLOG(this, LogPGPlayer, Log, TEXT("Get Player State"));
		}
	});

	GetWorldTimerManager().SetTimer(VisualLoggerTimer, DebugDrawDelegate, 0.05f, true);
}

void ABasePlayerController::CleanupDebugDraw()
{
	GetWorldTimerManager().ClearTimer(VisualLoggerTimer);
}

bool ABasePlayerController::CanPauseGame() const
{
	// To be fair only allow pause when running standalone
	return GetNetMode() == NM_Standalone;
}

#else

void ABasePlayerController::InitDebugDraw() {}
void ABasePlayerController::CleanupDebugDraw() {}

#endif

#pragma endregion Visual Logger
