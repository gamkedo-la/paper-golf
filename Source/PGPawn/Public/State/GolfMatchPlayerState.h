// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "State/GolfPlayerState.h"
#include "GolfMatchPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class PGPAWN_API AGolfMatchPlayerState : public AGolfPlayerState
{
	GENERATED_BODY()

public:

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnDisplayScoreUpdated, AGolfMatchPlayerState& /*PlayerState*/);

	FOnDisplayScoreUpdated OnDisplayScoreUpdated{};

	virtual int32 GetDisplayScore() const override;

	void AwardPoints(int32 InPoints);
	virtual bool CompareByScore(const AGolfPlayerState& Other) const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty >& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* PlayerState) override;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

private:
	UFUNCTION() 
	void OnRep_DisplayScore();

	int32 GetNumScoreChanges() const;

	void SetDisplayScore(int32 Points, int32 NumChanges);

private:

	UPROPERTY(ReplicatedUsing = OnRep_DisplayScore)
	uint32 DisplayScore{};
};
