// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GolfPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class PGPAWN_API AGolfPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTotalShotsUpdated, AGolfPlayerState& /*PlayerState*/);

	AGolfPlayerState();

	FOnTotalShotsUpdated OnTotalShotsUpdated{};
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty >& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void AddShot() { ++Shots;  }

	UFUNCTION(BlueprintPure)
	int32 GetShots() const { return Shots; }

	UFUNCTION(BlueprintPure)
	int32 GetTotalShots() const;

	UFUNCTION(BlueprintCallable)
	void SetReadyForShot(bool bReady) { bReadyForShot = bReady; }

	UFUNCTION(BlueprintPure)
	bool IsReadyForShot() const { return bReadyForShot; }

	UFUNCTION(BlueprintCallable)
	virtual void FinishHole();

	UFUNCTION(BlueprintCallable)
	void StartHole() { Shots = 0; }

	virtual void CopyProperties(APlayerState* PlayerState) override;

	void SetSpectatorOnly() { bSpectatorOnly = true; }
	bool IsSpectatorOnly() const { return bSpectatorOnly; }

	virtual bool CompareByScore(const AGolfPlayerState& Other) const;
		
private:
	UFUNCTION()
	void OnRep_ScoreByHole();

private:

	UPROPERTY(ReplicatedUsing = OnRep_ScoreByHole)
	TArray<uint8> ScoreByHole{};

	UPROPERTY(Replicated)
	uint8 Shots{};

	UPROPERTY(Replicated)
	bool bReadyForShot{};

	UPROPERTY(Replicated)
	bool bSpectatorOnly{};
};
