// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HoleTransitionComponent.generated.h"

class APaperGolfGameStateBase;
class AGolfPlayerStart;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UHoleTransitionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHoleTransitionComponent();

	AActor* ChoosePlayerStart(AController* Player);

protected:
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;

private:

	void Init();
	void InitCachedData();
	void RegisterEventHandlers();

	UFUNCTION()
	void OnNextHole();

#if WITH_EDITOR
	APlayerStart* FindPlayFromHereStart(AController* Player);
#endif

private:
	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float NextHoleDelay{ 3.0f };

	UPROPERTY(Transient)
	TObjectPtr<AGameModeBase> GameMode{};

	UPROPERTY(Transient)
	TObjectPtr<APaperGolfGameStateBase> GameState{};

	UPROPERTY(Transient)
	TArray<AGolfPlayerStart*> RelevantPlayerStarts{};
};
