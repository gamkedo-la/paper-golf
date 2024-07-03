// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GolfTurnBasedDirectorComponent.generated.h"

class APaperGolfGameModeBase;
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

	virtual void InitializeComponent() override;

protected:
	virtual void BeginPlay() override;

private:
	void RegisterEventHandlers();

	UFUNCTION()
	void OnPaperGolfShotFinished(APaperGolfPawn* PaperGolfPawn);

	UFUNCTION()
	void OnPaperGolfPlayerScored(APaperGolfPawn* PaperGolfPawn);

	void DoNextTurn();

	void ActivateNextPlayer();

	int32 DetermineClosestPlayerToHole() const;
	int32 DetermineNextPlayer() const;

	void ActivatePlayer(AGolfPlayerController* Player);
	// TODO: Will need a variant that takes an AI controller or better yet use an interface implemented by both

	void NextHole();

private:
	// TODO: Use APGTurnBasedGameMode if need the functionality of it.  Keeping it to the base class for now for maximum reuse
	UPROPERTY(Transient)
	APaperGolfGameModeBase* GameMode{};

	// FIXME: Should use an interface for this as want to also support AI players
	UPROPERTY(Transient)
	TArray<AGolfPlayerController*> Players{};

	int32 ActivePlayerIndex{};

	UPROPERTY(EditDefaultsOnly)
	float NextHoleDelay{ 3.0f };
};
