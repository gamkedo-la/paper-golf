// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "PaperGolfGameStateBase.generated.h"

class AGolfPlayerState;
class APaperGolfPawn;

/**
 * 
 */
UCLASS()
class PGPAWN_API APaperGolfGameStateBase : public AGameState
{
	GENERATED_BODY()

public:

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnHoleChanged, int32 /*Current Hole Number*/);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnScoresSynced, APaperGolfGameStateBase& /*GameState*/);

	FOnHoleChanged OnHoleChanged{};
	FOnScoresSynced OnScoresSynced{};

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure)
	int32 GetCurrentHoleNumber() const { return CurrentHoleNumber; }

	void SetCurrentHoleNumber(int32 Hole);

	UFUNCTION(BlueprintPure)
	AGolfPlayerState* GetActivePlayer() const { return ActivePlayer; }

	void SetActivePlayer(AGolfPlayerState* Player);

	/** Add PlayerState to the PlayerArray - called on both clients and server */
	virtual void AddPlayerState(APlayerState* PlayerState) override;

	/** Remove PlayerState from the PlayerArray - called on both clients and server */
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	UFUNCTION(BlueprintPure)
	TArray<AGolfPlayerState*> GetSortedPlayerStatesByScore() const;

protected:
	virtual void BeginPlay() override;

	/*
	* Call after updating a field that affects score syncing.  This will handle checking, broadcasting, and resetting the state.
	*/
	void CheckScoreSyncState();

	virtual bool AllScoresSynced() const;
	virtual void ResetScoreSyncState();

private:
	UFUNCTION()
	void OnRep_CurrentHoleNumber();

	void OnTotalShotsUpdated(AGolfPlayerState& PlayerState);

	void SubscribeToGolfEvents();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnCourseComplete();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnNextHole();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnStartHole(int32 HoleNumber);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnPlayerScored(APaperGolfPawn* PlayerPawn);

private:

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHoleNumber)
	int32 CurrentHoleNumber{};

	UPROPERTY(Transient, Replicated)
	TObjectPtr<AGolfPlayerState> ActivePlayer{};

	UPROPERTY(Transient)
	TArray<AGolfPlayerState*> UpdatedPlayerStates{};
};
