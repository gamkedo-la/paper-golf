// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "Interfaces/GolfController.h"

#include "PaperGolfTypes.h"
#include "PGAITypes.h"

#include "Pawn/PaperGolfPawn.h"

#include "GolfAIController.generated.h"

class UGolfControllerCommonComponent;
class UGolfAIShotComponent;

class APaperGolfPawn;
class AGolfPlayerState;

/**
 * 
 */
UCLASS()
class PGAI_API AGolfAIController : public AAIController, public IGolfController
{
	GENERATED_BODY()
	
public:

	AGolfAIController();

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

	virtual void ReceivePlayerStart(AActor* PlayerStart) override;

	// Inherited via IGolfController
	using IGolfController::GetPaperGolfPawn;
	virtual APaperGolfPawn* GetPaperGolfPawn() override;

	virtual bool HasPaperGolfPawn() const override;

	virtual void MarkScored() override;

	virtual bool HasScored() const override;

	virtual bool IsActivePlayer() const override;

	virtual bool IsReadyForNextShot() const override;

	virtual void StartHole(EHoleStartType InHoleStartType) override;

	virtual void ActivateTurn() override;

	virtual void Spectate(APaperGolfPawn* InPawn, AGolfPlayerState* InPlayerState) override;

	virtual bool HandleOutOfBounds() override;

	virtual EShotType GetShotType() const override;

	virtual void SetPawn(APawn* InPawn) override;

	virtual void Reset() override;

	virtual void ResetShot() override;

protected:
	virtual AController* AsController() override { return this; }

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	struct FShotAnimationState
	{
		FTimerHandle TimerHandle{};
		double CurrentValue{};
		double TargetValue{};
		float StartTimeSeconds{};
		float EndTimeSeconds{};
	};

	void OnScored();

	bool SetupShot();
	void InterpolateShotSetup(float ShootDeltaTime);

	void ExecuteTurn();

	void DestroyPawn();
	void SetupNextShot(bool bSetCanFlick);
	void DetermineShotType();
	void ResetShotAfterOutOfBounds();
	void SetPositionTo(const FVector& Position, const TOptional<FRotator>& OptionalRotation = {});

	UFUNCTION()
	void OnFellThroughFloor(APaperGolfPawn* InPaperGolfPawn);

	// Inherited via IGolfController
	virtual UGolfControllerCommonComponent* GetGolfControllerCommonComponent() override;
	virtual void DoAdditionalOnShotFinished() override;
	virtual void DoAdditionalFallThroughFloor() override;

	void InitDebugDraw();
	void CleanupDebugDraw();

	void ClearTimers();
	void ClearShotAnimationTimers();

	bool InterpolateShotAnimationState(FShotAnimationState& AnimationState, double& DeltaValue) const;
	void InterpolateAnimation(FShotAnimationState& AnimationState, TFunction<FRotator(double)> RotatorCreator) const;
	void InterpolateYawAnimation();
	void InterpolatePitchAnimation();

#if ENABLE_VISUAL_LOG
	bool ShouldCaptureDebugSnapshot() const;
#endif

private:
	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UGolfControllerCommonComponent> GolfControllerCommonComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UGolfAIShotComponent> GolfAIShotComponent{};

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float MinFlickReactionTime{ 1.5f };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float MaxFlickReactionTime{ 3.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float OutOfBoundsDelayTime{ 3.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot Animation")
	float ShotAnimationMinTime{ 0.5f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot Animation")
	float ShotAnimationInterpSpeed{ 1.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot Animation")
	float ShotAnimationFinishDeltaTime{ 0.25f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot Animation")
	float ShotAnimationDeltaTime{ 1 / 60.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot Animation")
	float ShotAnimationEaseFactor{ 2.0f };

	UPROPERTY(Transient)
	TObjectPtr<APaperGolfPawn> PlayerPawn{};

	UPROPERTY(Transient)
	TOptional<FAIShotSetupResult> ShotSetupParams{};

	FTimerHandle TurnTimerHandle{};

#if ENABLE_VISUAL_LOG
	FTimerHandle VisualLoggerTimer{};
#endif

	EShotType ShotType{ EShotType::Default };

	FShotAnimationState YawAnimationState{};
	FShotAnimationState PitchAnimationState{};


	bool bCanFlick{ };
	bool bScored{};
	bool bTurnActivated{};
	bool bOutOfBounds{};
};

#pragma region Inline Definitions

FORCEINLINE EShotType AGolfAIController::GetShotType() const
{
	return ShotType;
}

FORCEINLINE bool AGolfAIController::HasScored() const
{
	return bScored;
}

FORCEINLINE bool AGolfAIController::IsActivePlayer() const
{
	return bTurnActivated;
}

FORCEINLINE UGolfControllerCommonComponent* AGolfAIController::GetGolfControllerCommonComponent()
{
	return GolfControllerCommonComponent;
}

FORCEINLINE bool AGolfAIController::HasPaperGolfPawn() const
{
	return PlayerPawn != nullptr;
}

#pragma endregion Inline Definitions
