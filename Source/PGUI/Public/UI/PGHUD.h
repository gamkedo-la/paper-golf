// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "PGHUD.generated.h"

class UUserWidget;
class UGolfUserWidget;
class APaperGolfGameStateBase;
class APaperGolfPawn;
class AGolfPlayerState;
class ITextDisplayingWidget;
class USoundBase;
class ULevelSequence;

UENUM(BlueprintType)
enum class EMessageWidgetType : uint8
{
	OutOfBounds,
	HoleFinished,
	Tutorial
};

/**
 * 
 */
UCLASS()
class PGUI_API APGHUD : public AHUD
{
	GENERATED_BODY()

public:

	virtual void ShowHUD() override;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetHUDVisible(bool bVisible);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnGameOver();

	UFUNCTION(BlueprintPure)
	UGolfUserWidget* GetGolfWidget() const;

	UFUNCTION(BlueprintCallable)
	void DisplayMessageWidget(EMessageWidgetType MessageType);

	UFUNCTION(BlueprintCallable)
	void RemoveActiveMessageWidget();

	UFUNCTION(BlueprintNativeEvent, Category = UI)
	void BeginTurn();

	UFUNCTION(BlueprintNativeEvent, Category = UI)
	void BeginShot();

	/* Plays the hole flyby. To be notified when it is completed or canceled bind to the UTutorialTrackingSystem::OnHoleFlybyComplete delegate */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Cinematics")
	void PlayHoleFlybySequence(ULevelSequence* LevelSequence, bool bFadeCamera = false);

	void SpectatePlayer(int32 PlayerStateId);
	void BeginSpectatorShot(int32 PlayerStateId);

protected:

	UFUNCTION(BlueprintNativeEvent, Category = "UI")
	void SpectatePlayer(const AGolfPlayerState* InPlayerState);

	UFUNCTION(BlueprintNativeEvent, Category = UI)
	void BeginSpectatorShot(const AGolfPlayerState* InPlayerState);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnToggleHUDVisibility(bool bVisible);

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowScoresHUD(const TArray<AGolfPlayerState*>& PlayerStates);

	UFUNCTION(BlueprintImplementableEvent)
	void HideScoresHUD();

	UFUNCTION(BlueprintImplementableEvent)
	void ShowFinalResultsHUD(const TArray<AGolfPlayerState*>& PlayerStates);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowCurrentHoleScoresHUD(const TArray<AGolfPlayerState*>& PlayerStates);

	UFUNCTION(BlueprintImplementableEvent)
	void HideCurrentHoleScoresHUD();

private:

	enum class EDeferredPlayerState : uint8
	{
		None,
		SpectatingShotSetup,
		SpectatingShot
	};

	struct FActivePlayer
	{
		TWeakObjectPtr<const AGolfPlayerState> PlayerState{};
		int32 Id{};
		EDeferredPlayerState LastDeferredState{ EDeferredPlayerState::None };


		bool HasAnyDeferredState() const { return LastDeferredState != EDeferredPlayerState::None; }
		bool MatchesDeferredState(int32 PlayerId) const { return Id == PlayerId && HasAnyDeferredState(); }

		FActivePlayer(const AGolfPlayerState& InPlayerState);
		FActivePlayer(int32 Id, EDeferredPlayerState RequestedState) : Id(Id), LastDeferredState(RequestedState) {}
	};

	bool TryFindPlayerById(int32 PlayerStateId, AGolfPlayerState*& OutPlayerState) const;

	void CheckExecuteDeferredSpectatorAction(const AGolfPlayerState& AddedPlayerState);

	void DisplayMessageWidgetByClass(const TSoftClassPtr<UUserWidget>& WidgetClass);

	void Init();

	void OnScoresSynced(APaperGolfGameStateBase& GameState);
	void OnCurrentHoleScoreUpdate(APaperGolfGameStateBase& GameState, const AGolfPlayerState& PlayerState);
	void OnPlayersChanged(APaperGolfGameStateBase& GameState, const AGolfPlayerState& PlayerState, bool bPlayerAdded);

	UFUNCTION()
	void OnPlayerScored(APaperGolfPawn* PaperGolfPawn);

	UFUNCTION()
	void OnStartHole(int32 HoleNumber);

	UFUNCTION()
	void OnCourseComplete();

	UFUNCTION()
	void OnHoleComplete();

	void HideActiveTurnWidget();

	bool ShouldShowActiveTurnWidgets() const;

	void LoadWidgetAsync(const TSoftClassPtr<UUserWidget>& WidgetClass, TFunction<void(UUserWidget&)> OnWidgetReady);

	using WidgetMemberPtr = TObjectPtr<UUserWidget> ThisClass::*;

	void DisplayTurnWidget(const TSoftClassPtr<UUserWidget>& WidgetClass, WidgetMemberPtr WidgetToDisplay, const FText& Message);
	
	void DisplayHoleFinishedMessage();
	void RegisterHoleFinishedMessageRemoval();
	void RemoveHoleFinishedMessage();
	void UnregisterHoleFinishedMessageRemoval();

	void PlaySound2D(const TSoftObjectPtr<USoundBase>& Sound);

	void PlayWinSoundIfApplicable();

	void CheckNotifyHoleShotsUpdate(const APaperGolfGameStateBase& GameState);

	void CheckShowScoresOrFinalResults(const APaperGolfGameStateBase& GameState);
	bool CheckShowFinalResults(const APaperGolfGameStateBase* GameState);

	APaperGolfGameStateBase* GetGameState() const;

	bool FinalResultsAreDetermined() const;
	
	void CheckForInitialDeferredState(const APaperGolfGameStateBase& GameState);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	TSubclassOf<UGolfUserWidget> GolfWidgetClass{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	TSubclassOf<UUserWidget> SkipHoleFlybyWidgetClass{};

	UPROPERTY(Transient, BlueprintReadWrite, Category = "UI")
	TObjectPtr<UGolfUserWidget> GolfWidget{};

	// Use this instead of the game state since there may be a replication delay
	TOptional<FActivePlayer> ActivePlayer{};

	/* Hide the score of the active player in the top-left scores HUD at the start of the turn. */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	bool bHideActivePlayerHoleScore{};

private:
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSoftClassPtr<UUserWidget> OutOfBoundsWidgetClass{};

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSoftClassPtr<UUserWidget> HoleFinishedWidgetClass{};

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float HoleFinishedMessageDuration{ 2.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSoftClassPtr<UUserWidget> TutorialWidgetClass{};

	UPROPERTY(EditDefaultsOnly, Category = "Config", meta= (MustImplement = "/Script/PGUI.TextDisplayingWidget"))
	TSoftClassPtr<UUserWidget> ActiveTurnWidgetClass{};

	UPROPERTY(EditDefaultsOnly, Category = "Config", meta = (MustImplement = "/Script/PGUI.TextDisplayingWidget"))
	TSoftClassPtr<UUserWidget> SpectatingWidgetClass{};

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveMessageWidget{};

	UPROPERTY(Transient)
	TSoftClassPtr<UUserWidget> ActiveMessageWidgetClass{};

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveTurnWidget{};

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> SpectatingWidget{};

	UPROPERTY(Transient)
	TSoftClassPtr<UUserWidget> ActivePlayerTurnWidgetClass{};

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActivePlayerTurnWidget{};

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TSoftObjectPtr<USoundBase> ScoredSfx{};

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TSoftObjectPtr<USoundBase> WinSfx{};

	FTimerHandle HoleFinishedMessageTimerHandle{};

	bool bScoresSynced{};
	bool bScoresEverSynced{};
	bool bCourseComplete{};
	bool bShotUpdatesReceived{};
};

#pragma region Inline Definitions

FORCEINLINE UGolfUserWidget* APGHUD::GetGolfWidget() const
{
	return GolfWidget;
}

#pragma endregion Inline Definitions
