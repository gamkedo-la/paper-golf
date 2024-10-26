// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "BasePlayerController.generated.h"

class UUserWidget;

/**
 * 
 */
UCLASS()
class PGPLAYER_API ABasePlayerController : public APlayerController, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_BODY()

public:
#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

	UFUNCTION(BlueprintCallable)
	void SetInputModeUI(UUserWidget* FocusWidget = nullptr);

	UFUNCTION(BlueprintCallable)
	void SetInputModeGame();

	UFUNCTION(BlueprintCallable)
	void PauseGame(UUserWidget* FocusWidget = nullptr);

	UFUNCTION(BlueprintCallable)
	void ResumeGame();

	UFUNCTION(BlueprintPure)
	bool IsGamePaused() const;

	/**
	 Cannot pause the game in multiplayer, so this checks if the pause action
	 is still active or if it was resumed. 
	*/
	UFUNCTION(BlueprintPure)
	bool IsPauseActionActive() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	virtual void OnPauseGame(UUserWidget* FocusWidget) {}
	virtual void OnResumeGame() {}

	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintPauseGame(UUserWidget* FocusWidget);

	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintResumeGame();

	UFUNCTION(BlueprintPure)
	bool CanPauseGame() const;

#if ENABLE_VISUAL_LOG
	virtual bool ShouldCaptureDebugSnapshot() const { return false; }
#endif

private:
	void SetPaused(bool bPaused);

	void InitDebugDraw();
	void CleanupDebugDraw();

	
private:

	#if ENABLE_VISUAL_LOG
		FTimerHandle VisualLoggerTimer{};
	#endif

	bool bPauseRequested{};
};

#pragma region Inline Definitions

FORCEINLINE bool ABasePlayerController::IsPauseActionActive() const
{
	return bPauseRequested;
}

#pragma endregion Inline Definitions
