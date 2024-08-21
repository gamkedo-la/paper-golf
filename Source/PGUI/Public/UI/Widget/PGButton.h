// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "PGButton.generated.h"

class USoundBase;

/**
 * 
 */
UCLASS()
class PGUI_API UPGButton : public UButton
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Gamepad | Button")
	void Hover();

	UFUNCTION(BlueprintCallable, Category = "Gamepad | Button")
	void ExitHover();

	UFUNCTION(BlueprintCallable, Category = "Gamepad | Button")
	void Click();

protected:
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;

private:

	void BindEvents();

	UFUNCTION()
	void DoHover();

	void TickHover(float DeltaTime);

	void UnregisterTimer();

protected:
	UPROPERTY(EditAnywhere, Category = "Hover")
	float HoverMaxScale{ 1.0f };

	UPROPERTY(EditAnywhere, Category = "Hover")
	float HoverUpdateTime{ 1 / 30.0f };

	UPROPERTY(EditAnywhere, Category = "Hover")
	float HoverScaleTime{ 1.0f };

	UPROPERTY(EditAnywhere, Category = "Hover")
	float HoverDelayTime{ 0.1f };

	UPROPERTY(EditAnywhere, Category = "Hover")
	float HoverEaseFactor{ 2.0f };

	FDelegateHandle HoverHandle{};

	float HoverStartTime{ -1.0f };
};
