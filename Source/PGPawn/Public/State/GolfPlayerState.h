// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "GolfPlayerState.generated.h"

class APaperGolfPawn;

/**
 * 
 */
UCLASS()
class PGPAWN_API AGolfPlayerState : public APlayerState, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_BODY()

public:

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTotalShotsUpdated, AGolfPlayerState& /*PlayerState*/);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHoleShotsUpdated, AGolfPlayerState& /*PlayerState*/, int32 PreviousValue);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnReadyForShotUpdated, AGolfPlayerState& /*PlayerState*/);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnScoredUpdated, AGolfPlayerState& /*PlayerState*/);

	AGolfPlayerState();

	FOnTotalShotsUpdated OnTotalShotsUpdated{};
	FOnHoleShotsUpdated OnHoleShotsUpdated{};
	FOnReadyForShotUpdated OnReadyForShotUpdated{};
	FOnScoredUpdated OnScoredUpdated{};

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty >& OutLifetimeProps) const override;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override { DoGrabDebugSnapshot(Snapshot, nullptr); }
	void GrabDebugSnapshotAsChildCategory(FVisualLogEntry* Snapshot, FVisualLogStatusCategory& ParentCategory) const { DoGrabDebugSnapshot(Snapshot, &ParentCategory); }
#endif

	UFUNCTION(BlueprintCallable, meta = (BlueprintAuthorityOnly) )
	void AddShot();

	/*
	* Called if player drops out in middle of shot after flicking and turn needs to be reset so that new player is not penalized.
	*/
	void UndoShot();

	UFUNCTION(BlueprintPure)
	int32 GetShots() const { return Shots; }

	UFUNCTION(BlueprintPure)
	int32 GetShotsIncludingCurrent() const { return bReadyForShot ? Shots + 1 : Shots; }

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

	UFUNCTION(BlueprintCallable, meta = (BlueprintAuthorityOnly))
	void SetReadyForShot(bool bReady);

	UFUNCTION(BlueprintPure)
	bool IsReadyForShot() const { return bReadyForShot; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintAuthorityOnly))
	virtual void FinishHole();

	UFUNCTION(BlueprintCallable, meta = (BlueprintAuthorityOnly))
	void StartHole();

	virtual void CopyProperties(APlayerState* PlayerState) override final;

	/*
	* Copies game state properties from the input player state.
	* This is different from CopyProperties that is designed to copy everything, including network ids from the input player state.
	* Does not copy the player name.
	*/
	void CopyGameStateProperties(const AGolfPlayerState* InPlayerState);

	void SetSpectatorOnly() 
	{ 
		bSpectatorOnly = true; 
		ForceNetUpdate();
	}

	UFUNCTION(BlueprintPure)
	bool IsSpectatorOnly() const { return bSpectatorOnly; }

	void SetHasScored(bool bInScored);
		
	UFUNCTION(BlueprintPure)
	bool HasScored() const { return bScored; }

	virtual bool CompareByScore(const AGolfPlayerState& Other) const;

	bool CompareByCurrentHoleShots(const AGolfPlayerState& Other, bool bIncludeTurnActivation = true) const;

	UFUNCTION(BlueprintPure)
	FLinearColor GetPlayerColor() const;

	void SetPlayerColor(const FLinearColor& Color) 
	{ 
		PlayerColor = Color; 
		ForceNetUpdate();
	}

	void SetPawnLocationAndRotation(APaperGolfPawn& PlayerPawn) const;
	void SetLocationAndRotationFromPawn(const APaperGolfPawn& PlayerPawn);

	bool HasPawnTransformSet() const;
	float GetPawnSquaredHorizontalDistanceTo(const AActor& Actor) const;

protected:
	virtual void DoCopyProperties(const AGolfPlayerState* InPlayerState);
		
private:
	UFUNCTION()
	void OnRep_ScoreByHole();

	UFUNCTION()
	void OnRep_Shots(uint8 PreviousShots);

	UFUNCTION()
	void OnRep_ReadyForShot();

	UFUNCTION()
	void OnRep_Scored();

	void UpdateShotCount(int32 DeltaCount);

#if ENABLE_VISUAL_LOG
	void DoGrabDebugSnapshot(FVisualLogEntry* Snapshot, FVisualLogStatusCategory* ParentCategory) const;
#endif

protected:

	// TODO: Cannot implement GetScoreByHole since this is assuming always start at hole 1 at index 0 and that might not be case
	// So would need an array of structs here that track which hole the score is for
	UPROPERTY(Transient, ReplicatedUsing = OnRep_ScoreByHole)
	TArray<uint8> ScoreByHole{};

private:

	UPROPERTY(Transient, ReplicatedUsing = OnRep_Shots)
	uint8 Shots{};

	UPROPERTY(ReplicatedUsing = OnRep_ReadyForShot)
	bool bReadyForShot{};

	UPROPERTY(Transient, Replicated)
	bool bSpectatorOnly{};

	UPROPERTY(Transient, ReplicatedUsing = OnRep_Scored)
	bool bScored{};

	bool bPositionAndRotationSet{};

	UPROPERTY(Transient, Replicated)
	FLinearColor PlayerColor{FLinearColor::White};

	// Not Replicated - used by server to set initial position during late join
	FVector Position{ EForceInit::ForceInitToZero };
	FRotator Rotation{ EForceInit::ForceInitToZero };
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

FORCEINLINE float AGolfPlayerState::GetPawnSquaredHorizontalDistanceTo(const AActor& Actor) const
{
	return bPositionAndRotationSet ? FVector::DistSquared2D(Position, Actor.GetActorLocation()) : 0.0f;
}

FORCEINLINE  bool AGolfPlayerState::HasPawnTransformSet() const
{
	return bPositionAndRotationSet;
}

FORCEINLINE void AGolfPlayerState::AddShot()
{
	UpdateShotCount(1);
}

FORCEINLINE void AGolfPlayerState::UndoShot()
{
	UpdateShotCount(-1);
}

#pragma endregion Inline Definitions
