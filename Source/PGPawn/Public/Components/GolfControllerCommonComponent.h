// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "PaperGolfTypes.h"

#include "GolfControllerCommonComponent.generated.h"

class APaperGolfPawn;
class IGolfController;

USTRUCT(BlueprintType)
struct PGPAWN_API FShotHistory
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FVector Position{ EForceInit::ForceInitToZero };
};

bool operator== (const FShotHistory& First, const FShotHistory& Second);

struct PGPAWN_API FFocusActorScoreParams
{
	bool bIncludeMisaligned{};
};

/*
* Holds common logic shared by AI and player golf controllers.
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PGPAWN_API UGolfControllerCommonComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UGolfControllerCommonComponent();

	void Initialize(const FSimpleDelegate& InOnControllerShotFinished);

	void DestroyPawn();

	EShotType DetermineShotType(EShotFocusType FocusType = EShotFocusType::Hole);

	void AddToShotHistory(APaperGolfPawn* PaperGolfPawn);

	// PositionOverride used by clients to avoid replication delay issues
	void SetPaperGolfPawnAimFocus(const TOptional<FVector>& PositionOverride = {});

	/*
	* Gets the best focus actor based on the current pawn position. Optionally, returns all the scorings for the focus actors
	* ordered by best score (lowest value) to worst if OutFocusScores is non-null.
	* 
	* @param OutFocusScores Optional output parameter to return the focus actors and their scores.
	* @return The best focus actor based on the current pawn position.
	*/
	AActor* GetBestFocusActor(const TOptional<FVector>& PositionOverride = {}, TArray<FShotFocusScores>* OutFocusScores = nullptr, const FFocusActorScoreParams& FocusActorScoreParams = {}) const;

	TOptional<FShotHistory> GetLastShot() const;

	bool HandleFallThroughFloor();

	void ResetShot();

	bool SetupNextShot(bool bSetCanFlick);

	void SetPositionTo(const FVector& Position, const TOptional<FRotator>& OptionalRotation = {});

	void BeginTurn();

	void EndTurn();

	void Reset();

	void OnScored();

	// TODO: Visual Logger grab debug snapshot function to be called from controllers

	AActor* GetCurrentGolfHole() const;
	const TArray<AActor*>& GetFocusableActors() const;

	void SyncHoleChanged(const FSimpleDelegate& InHoleSyncedDelegate);

protected:
	virtual void BeginPlay() override;

private:

	AActor* GetShotFocusActor(EShotFocusType ShotFocusType) const;

	void RegisterShotFinishedTimer();
	void UnregisterShotFinishedTimer();

	void CheckForNextShot();

	void InitFocusableActors();

	bool HasLOSToFocus(const FVector& Position, const AActor* FocusActor) const;

	void OnHoleChanged(int32 HoleNumber);

private:
	TArray<FShotHistory> ShotHistory{};

	UPROPERTY(Transient)
	TArray<AActor*> FocusableActors{};

	UPROPERTY(Transient)
	TObjectPtr<AActor> GolfHole{};

	UPROPERTY(Transient)
	TScriptInterface<IGolfController> GolfController{};

	UPROPERTY(EditDefaultsOnly, Category = "Timer")
	float RestCheckTickRate{ 0.5f };

	UPROPERTY(EditDefaultsOnly, Category = "Timer")
	float RestCheckTriggerDelay{ 0.2f };

	UPROPERTY(EditDefaultsOnly, Category = "Timer")
	float MinFlickElapsedTimeForShotFinished{ 1.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Type")
	float CloseShotThreshold{ 1000.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Type")
	float MediumShotThreshold{ 3000.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Correction")
	float FallThroughFloorCorrectionTestZ{ 1000.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Focus")
	float FocusTraceStartOffset{ 100.0f };

	/**
	 * Minimum offset to trace to relative to the focus actor location.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Shot | Focus")
	float FocusTraceEndOffset{ 2000.0f };

	/**
	 * Maximum offset to trace to relative to the focus actor location. Helps avoiding hitting the ceiling
	 * when offsetting due to elevated start position.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Shot | Focus")
	float FocusTraceMaxEndOffset{ 3000.0f };

	/**
	 * When getting focus scores with misalignment included, this factor is multiplied on the misalignment.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Shot | Focus", meta=(ClampMin="1.0"))
	float FocusMisalignmentPenaltyScoreFactor{ 10.0f };

	int32 LastHoleNumber{};
	float LastFlickTime{};
	float FirstRestCheckPassTime{ -1.0f };
	bool bOnHoleChangedTriggered{};

	TWeakObjectPtr<APaperGolfPawn> WeakPaperGolfPawn{};
	FTimerHandle NextShotTimerHandle{};
	FDelegateHandle OnFlickHandle{};

	FSimpleDelegate OnControllerShotFinished{};
	FSimpleDelegate OnHoleSyncedDelegate{};
};

#pragma region Inline Definitions

FORCEINLINE bool operator== (const FShotHistory& First, const FShotHistory& Second)
{
	return First.Position.Equals(Second.Position, 1.0);
}

FORCEINLINE TOptional<FShotHistory> UGolfControllerCommonComponent::GetLastShot() const
{
	return !ShotHistory.IsEmpty() ? ShotHistory.Last() : TOptional<FShotHistory>{};
}

FORCEINLINE AActor* UGolfControllerCommonComponent::GetCurrentGolfHole() const
{
	return GolfHole;
}

FORCEINLINE const TArray<AActor*>& UGolfControllerCommonComponent::GetFocusableActors() const
{
	return FocusableActors;
}

#pragma endregion Inline Definitions
