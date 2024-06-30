// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GolfTurnBasedDirectorComponent.generated.h"

class AGameModeBase;
class AGolfPlayerController;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UGolfTurnBasedDirectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGolfTurnBasedDirectorComponent();

	UFUNCTION(BlueprintCallable)
	void StartHole();

	UFUNCTION(BlueprintCallable)
	void AddPlayer(AController* Player);

	UFUNCTION(BlueprintCallable)
	void RemovePlayer(AController* Player);

protected:
	virtual void BeginPlay() override;

private:
	void RegisterEventHandlers();

	UFUNCTION()
	void OnPaperGolfShotFinished(APaperGolfPawn* PaperGolfPawn);

	void ActivateNextPlayer();

	int32 DetermineClosestPlayerToHole() const;
	int32 DetermineNextPlayer() const;

private:
	UPROPERTY(Transient)
	class AGameModeBase* GameMode{};

	// FIXME: Should use an interface for this as want to also support AI players
	UPROPERTY(Transient)
	TArray<AGolfPlayerController*> Players{};

	int32 ActivePlayerIndex{};
};
