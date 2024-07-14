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

	void ResetRotation();

	void ResetShot();

	bool IsReadyForShot() const;

	bool HandleOutOfBounds();

	bool HasScored() const;

	void MarkScored();

	void SnapToGround();

	void AddPaperGolfPawnRelativeRotation(const FRotator& DeltaRotation);

	void SetShotType(EShotType InShotType);

	void ToggleShotType();

	void DestroyPawn();

	void DetermineShotType(const AActor& GolfHole);

	void AddToShotHistory(APaperGolfPawn* PaperGolfPawn);

	TOptional<FShotHistory> GetLastShot() const;

	bool IsFlickedAtRest() const;

	void ResetShotAfterOutOfBounds();

	void RegisterShotFinishedTimer();
	void UnregisterShotFinishedTimer();

	void RegisterGolfSubsystemEvents();

	void HandleFallThroughFloor();

	bool IsReadyForNextShot() const;
	void SetupNextShot(bool bSetCanFlick);

	void SetPositionTo(const FVector& Position, const TOptional<FRotator>& OptionalRotation = {});

	void CheckForNextShot();

	bool CanFlick() const;

	// TODO: Visual Logger grab debug snapshot function to be called from controllers


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	void Init();
	void DeferredInit();

	UFUNCTION()
	void OnFellThroughFloor(APaperGolfPawn* InPaperGolfPawn);

private:
	TArray<FShotHistory> ShotHistory{};

	UPROPERTY(Transient)
	TScriptInterface<IGolfController> GolfController{};

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	FRotator RotationMax{ 75.0, 180.0, 90.0 };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	FRotator TotalRotation{ EForceInit::ForceInitToZero };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float RotationRate{ 100.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Timer")
	float RestCheckTickRate{ 0.5f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Type")
	float CloseShotThreshold{ 1000.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Type")
	float MediumShotThreshold{ 3000.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Correction")
	float FallThroughFloorCorrectionTestZ{ 1000.0f };

	EShotType ShotType{ EShotType::Default };

	FTimerHandle NextShotTimerHandle{};

	bool bCanFlick{ };
	bool bTurnActivated{};
	bool bScored{};
	bool bOutOfBounds{};

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