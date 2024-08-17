// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "GolfPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class PGPAWN_API AGolfPlayerState : public APlayerState, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_BODY()

public:

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTotalShotsUpdated, AGolfPlayerState& /*PlayerState*/);

	AGolfPlayerState();

	FOnTotalShotsUpdated OnTotalShotsUpdated{};
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty >& OutLifetimeProps) const override;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

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

	// TODO: Cannot implement this yet - need to track which hole the score was for rather than just an array since may not start at hole 1
	// or may drop out
	//UFUNCTION(BlueprintPure)
	//int32 GetScoreForHole(int32 HoleNumber) const;

	UFUNCTION(BlueprintPure)
	int32 GetLastCompletedHoleScore() const;

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

FORCEINLINE int32 AGolfPlayerState::GetLastCompletedHoleScore() const
{
	return !ScoreByHole.IsEmpty() ? static_cast<int32>(ScoreByHole.Last()) : -1;
}

#pragma endregion Inline Definitions
