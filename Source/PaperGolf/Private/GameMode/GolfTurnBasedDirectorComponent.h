// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GolfTurnBasedDirectorComponent.generated.h"

class APaperGolfGameModeBase;
class APaperGolfGameStateBase;
class IGolfController;
class AGolfHole;

enum class EHazardType : uint8;

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

	UFUNCTION()
	void OnPaperGolfEnteredHazard(APaperGolfPawn* PaperGolfPawn, EHazardType HazardType);

	void DoNextTurn();

	void ActivateNextPlayer();

	int32 DetermineNextPlayer() const;

	void ActivatePlayer(IGolfController* Player);
	// TODO: Will need a variant that takes an AI controller or better yet use an interface implemented by both

	void NextHole();

	void InitializePlayersForHole();
	void MarkPlayersFinishedHole();

private:
	// TODO: Use APGTurnBasedGameMode if need the functionality of it.  Keeping it to the base class for now for maximum reuse
	UPROPERTY(Transient)
	TObjectPtr<APaperGolfGameModeBase> GameMode{};

	UPROPERTY(Transient)
	TObjectPtr<APaperGolfGameStateBase> GameState{};

	UPROPERTY(Transient)
	TArray<TScriptInterface<IGolfController>> Players{};

	UPROPERTY(Transient)
	TObjectPtr<AGolfHole> CurrentHole{};

	int32 ActivePlayerIndex{};

	/*
	* Skip human players and only spectate the AI. Used for testing.
	*/
	UPROPERTY(Category = "Config", EditDefaultsOnly)
	bool bSkipHumanPlayers{};

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float NextHoleDelay{ 3.0f };
};
