// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

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

	bool IsSkippingHumanPlayers() const;

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

	void SortPlayersForNextHole();

	bool IsActivePlayer(const IGolfController* Player) const;

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

	// Need to keep this mapping in order to make sure we can mark scored if turn ends before it is detected
	// TODO: This is a workaround as we should be checking for shot finished and scored in same place to avoid these timing issues
	TMap<TWeakObjectPtr<APaperGolfPawn>, TWeakObjectPtr<UObject>> PlayerPawnToController{};
 
	/*
	* Skip human players and only spectate the AI. Used for testing.
	*/
	UPROPERTY(Category = "Config", EditDefaultsOnly)
	bool bSkipHumanPlayers{};

	int32 HolesCompleted{};
};

#pragma region Inline Definitions

FORCEINLINE bool UGolfTurnBasedDirectorComponent::IsSkippingHumanPlayers() const
{
	return bSkipHumanPlayers;
}

#pragma endregion Inline Definitions
