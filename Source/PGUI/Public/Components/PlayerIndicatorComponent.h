// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "PlayerIndicatorComponent.generated.h"

class AGolfPlayerState;
class UUserWidget;

/**
 * 
 */
UCLASS()
class PGUI_API UPlayerIndicatorComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:

	UPlayerIndicatorComponent();

	UFUNCTION(BlueprintCallable)
	void SetVisibleForPlayer(AGolfPlayerState* Player);

	UFUNCTION(BlueprintCallable)
	void Hide();

protected:
	virtual void InitializeComponent() override;
};
