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

	UFUNCTION(BlueprintNativeEvent, Category = "UI")
	void SpectatePlayer(APaperGolfPawn* PlayerPawn);

	UFUNCTION(BlueprintNativeEvent, Category = UI)
	void BeginTurn();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnToggleHUDVisibility(bool bVisible);

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowScoresHUD(const TArray<AGolfPlayerState*>& PlayerStates);

private:
	void DisplayMessageWidgetByClass(const TSoftClassPtr<UUserWidget>& WidgetClass);

	void Init();

	void OnScoresSynced(APaperGolfGameStateBase& GameState);

	UFUNCTION()
	void OnPlayerScored(APaperGolfPawn* PaperGolfPawn);

	UFUNCTION()
	void OnStartHole(int32 HoleNumber);

	UFUNCTION()
	void OnCourseComplete();

	UFUNCTION()
	void OnHoleComplete();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	TSubclassOf<UGolfUserWidget> GolfWidgetClass{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	TSubclassOf<UUserWidget> SkipHoleFlybyWidgetClass{};

	UPROPERTY(Transient, BlueprintReadWrite, Category = "UI")
	TObjectPtr<UGolfUserWidget> GolfWidget{};

private:
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSoftClassPtr<UUserWidget> OutOfBoundsWidgetClass{};

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSoftClassPtr<UUserWidget> HoleFinishedWidgetClass{};

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSoftClassPtr<UUserWidget> TutorialWidgetClass{};

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveMessageWidget{};

	UPROPERTY(Transient)
	TSoftClassPtr<UUserWidget> ActiveMessageWidgetClass{};
};

#pragma region Inline Definitions

FORCEINLINE UGolfUserWidget* APGHUD::GetGolfWidget() const
{
	return GolfWidget;
}

#pragma endregion Inline Definitions
