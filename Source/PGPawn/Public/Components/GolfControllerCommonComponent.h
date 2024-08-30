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

	void SetPaperGolfPawnAimFocus();

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

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Focus")
	float FocusTraceEndOffset{ 2000.0f };

	int32 LastHoleNumber{};
	float LastFlickTime{};
	float FirstRestCheckPassTime{ -1.0f };

	TWeakObjectPtr<APaperGolfPawn> WeakPaperGolfPawn{};
	FTimerHandle NextShotTimerHandle{};
	FDelegateHandle OnFlickHandle{};

	DECLARE_DELEGATE(FOnControllerShotFinished);

	FOnControllerShotFinished OnControllerShotFinished{};

	// TODO: Move focus actors here for AI reuse?
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
