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

	virtual FVector2D ModifyProjectedLocalPosition(const FGeometry& ViewportGeometry, const FVector2D& LocalPosition) override;

protected:

	virtual void BeginPlay() override;

	virtual void InitializeComponent() override;

private:
	FString GetPlayerIndicatorString(const AGolfPlayerState& Player) const;
};
