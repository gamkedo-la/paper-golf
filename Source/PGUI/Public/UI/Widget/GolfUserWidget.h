// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GolfUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class PGUI_API UGolfUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Shot")
	void ResetMeter();

	UFUNCTION(BlueprintPure, BlueprintImplementableEvent, Category = "Shot")
	bool IsReadyForShot() const;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Shot")
	void ProcessClickForMeter();

	UFUNCTION(BlueprintPure, BlueprintImplementableEvent, Category = "Shot")
	float GetMeterAccuracy() const;
	
	UFUNCTION(BlueprintPure, BlueprintImplementableEvent, Category = "Shot")
	float GetMeterPower() const;
};
