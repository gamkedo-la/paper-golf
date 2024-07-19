// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PGHUD.generated.h"

class UUserWidget;
class UGolfUserWidget;

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

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnToggleHUDVisibility(bool bVisible);

private:
	void DisplayMessageWidgetByClass(const TSoftClassPtr<UUserWidget>& WidgetClass);

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
