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
	void AddShot() 
	{
		++Shots;
		ForceNetUpdate();
	}

	UFUNCTION(BlueprintPure)
	int32 GetShots() const { return Shots; }

	UFUNCTION(BlueprintPure)
	int32 GetNumCompletedHoles() const { return ScoreByHole.Num(); }

	UFUNCTION(BlueprintPure)	
	int32 GetTotalShots() const;

	// TODO: Cannot implement this yet
	//UFUNCTION(BlueprintPure)
	//bool GetScoreForHole(int32 HoleNumber) const;

	UFUNCTION(BlueprintPure)
	virtual int32 GetDisplayScore() const;

	UFUNCTION(BlueprintCallable)
	void SetReadyForShot(bool bReady)
	{ 
		bReadyForShot = bReady;
		ForceNetUpdate(); 
	}

	UFUNCTION(BlueprintPure)
	bool IsReadyForShot() const { return bReadyForShot; }

	UFUNCTION(BlueprintCallable)
	virtual void FinishHole();

	UFUNCTION(BlueprintCallable)
	void StartHole();

	virtual void CopyProperties(APlayerState* PlayerState) override;

	void SetSpectatorOnly() 
	{ 
		bSpectatorOnly = true; 
		ForceNetUpdate();
	}

	bool IsSpectatorOnly() const { return bSpectatorOnly; }

	void SetHasScored(bool bInScored) 
	{ 
		bScored = bInScored;
		ForceNetUpdate();
	}

	bool HasScored() const { return bScored; }

	virtual bool CompareByScore(const AGolfPlayerState& Other) const;

	UFUNCTION(BlueprintPure)
	FLinearColor GetPlayerColor() const;

	void SetPlayerColor(const FLinearColor& Color) 
	{ 
		PlayerColor = Color; 
		ForceNetUpdate();
	}
		
private:
	UFUNCTION()
	void OnRep_ScoreByHole();

protected:

	// TODO: Cannot implement GetScoreByHole since this is assuming always start at hole 1 at index 0 and that might not be case
	// So would need an array of structs here that track which hole the score is for
	UPROPERTY(ReplicatedUsing = OnRep_ScoreByHole)
	TArray<uint8> ScoreByHole{};

private:

	UPROPERTY(Replicated)
	uint8 Shots{};

	UPROPERTY(Replicated)
	bool bReadyForShot{};

	UPROPERTY(Replicated)
	bool bSpectatorOnly{};

	UPROPERTY(Replicated)
	bool bScored{};

	UPROPERTY(Replicated)
	FLinearColor PlayerColor{FLinearColor::White};
};

#pragma region Inline Definitions

FORCEINLINE int32 AGolfPlayerState::GetDisplayScore() const
{
	return GetTotalShots();
}

FORCEINLINE FLinearColor AGolfPlayerState::GetPlayerColor() const
{
	return PlayerColor;
}

#pragma endregion Inline Definitions
