// Copyright Game Salutes. All Rights Reserved.

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

	// TODO: Visual Logger grab debug snapshot function to be called from controllers

protected:
	virtual void BeginPlay() override;

private:

	AActor* GetShotFocusActor(EShotFocusType ShotFocusType) const;

	void RegisterShotFinishedTimer();
	void UnregisterShotFinishedTimer();

	void CheckForNextShot();

	void InitFocusableActors();

	bool HasLOSToFocus(const FVector& Position, const AActor* FocusActor) const;

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

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Type")
	float CloseShotThreshold{ 1000.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Type")
	float MediumShotThreshold{ 3000.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Correction")
	float FallThroughFloorCorrectionTestZ{ 1000.0f };

	FTimerHandle NextShotTimerHandle{};

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

#pragma endregion Inline Definitions