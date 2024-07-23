// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HoleTransitionComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UHoleTransitionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHoleTransitionComponent();

	AActor* ChoosePlayerStart(AController* Player);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnNextHole();

private:
	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float NextHoleDelay{ 3.0f };
};
