// Copyright Game Salutes. All Rights Reserved.

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
