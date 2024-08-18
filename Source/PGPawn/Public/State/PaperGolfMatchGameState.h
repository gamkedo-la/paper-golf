// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "State/PaperGolfGameStateBase.h"

#include "PGConstants.h"

#include "PaperGolfMatchGameState.generated.h"

class AGolfMatchPlayerState;

USTRUCT()
struct PGPAWN_API FGolfMatchScoring
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, meta=(ClampMin = "2"))
	int32 NumPlayers{ 2 };

	UPROPERTY(EditDefaultsOnly)
	TArray<int32> Points;
};

/**
 * 
 */
UCLASS()
class PGPAWN_API APaperGolfMatchGameState : public APaperGolfGameStateBase
{
	GENERATED_BODY()
	
public:
	virtual bool IsHoleComplete() const;

	/** Add PlayerState to the PlayerArray - called on both clients and server */
	virtual void AddPlayerState(APlayerState* PlayerState) override;

	/** Remove PlayerState from the PlayerArray - called on both clients and server */
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

protected:

	virtual bool AllScoresSynced() const;
	virtual void ResetScoreSyncState();
	virtual void DoAdditionalHoleComplete();

private:
	void OnDisplayScoreUpdated(AGolfMatchPlayerState& PlayerState);

	using FGolfMatchPlayerStateArray = TArray<AGolfMatchPlayerState*, TInlineAllocator<PG::MaxPlayers>>;

	FGolfMatchPlayerStateArray GetGolfMatchStatePlayerArray() const;

	const FGolfMatchScoring& GetScoreConfig(int32 NumPlayers) const;

private:

	static const FGolfMatchScoring DefaultScoring;

	UPROPERTY(EditDefaultsOnly, meta = (TitleProperty = "NumPlayers"), Category = "Scoring")
	TArray<FGolfMatchScoring> ScoringConfig;

	UPROPERTY(Transient)
	TArray<AGolfMatchPlayerState*> UpdatedMatchPlayerStates{};

	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	bool bAlwaysAllowFinish{};
};
