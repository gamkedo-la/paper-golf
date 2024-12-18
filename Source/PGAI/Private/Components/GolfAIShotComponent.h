// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "PGAITypes.h"

#include "Subsystems/GolfEvents.h"

#include "GolfAIShotComponent.generated.h"

class APaperGolfPawn;
class UCurveTable;
class UAIPerformanceStrategy;
class IPenaltyHazard;
struct FPredictProjectilePathResult;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UGolfAIShotComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UGolfAIShotComponent();

	/**
	* Calculate the shot set up for AI opponents given the input context and return the parameters
	  needed to set the AI heading and flick parameters to hit the shot. 
	*/
	FAIShotSetupResult SetupShot(FAIShotContext&& ShotContext);

	void StartHole();

	void Reset();

	void OnHazard(EHazardType HazardType);
	void OnShotFinished(const APaperGolfPawn& Pawn);

protected:
	virtual void BeginPlay() override;

private:

	struct FShotPowerCalculationResult
	{
		FVector FlickLocation{};
		float FlickMaxSpeed{};
		float PowerFraction{};
		EShotType ShotType{};

		FFlickParams ToFlickParams() const;
	};

	struct FShotCalculationResult
	{
		float Pitch{};
		float Yaw{};
		float PowerMultiplier{ 1.0f };
	};

	struct FShotCalibrationResult
	{
		float PowerFraction{};
		float Pitch{};
		float LocalZOffset{};
	};

	struct FShotSetupParams
	{
		FVector FlickLocation{};
		float InitialPowerFraction{};
		FAIShotSetupResult ShotSetupResult;

		FString ToString() const;

		float GetPowerDeviation() const;
	};

	struct FShotResult
	{
		FShotSetupParams ShotSetupParams{};
		FVector EndPosition{ EForceInit::ForceInitToZero };
		TOptional<EHazardType> HitHazard{};

		float GetShotDistanceSquared() const;
	};

	TOptional<FShotSetupParams> CalculateShotParams();
	TOptional<FShotSetupParams> CalculateShotParamsForCurrentFocusActor();

	/**
	* Checks if the current focus actor hasn't resulted in many recent failures
	* Returns true if the actor is viable, false otherwise.
	* If true is returned OutNumFailures will contain the number of recent failures.
	**/
	bool IsFocusActorViableBasedOnShotHistory(const AActor* FocusActor, int32 MaxFailures, int32* OutNumFailures = nullptr, bool* bOutCurrentFocusActorLandedInHazard = nullptr) const;

	FShotSetupParams CalculateDefaultShotParams() const;

	TOptional<FShotPowerCalculationResult> CalculateInitialShotParams() const;
	FShotCalibrationResult CalibrateShot(const FVector& FlickLocation, float PowerFraction) const;

	float CalculateDefaultZOffset() const;

	FShotCalculationResult CalculateAvoidanceShotFactors(const FVector& FlickLocation, float PreferredShotAngle, float FlickMaxSpeed, float PowerFraction) const;

	TTuple<bool, float> CalculateShotPitch(const FVector& FlickLocation, const FVector& FlickDirection, float FlickSpeed) const;

	FVector GetFocusActorLocation(const FVector& FlickLocation) const;

	bool ValidateAndLoadConfig();
	bool ValidateCurveTable(UCurveTable* CurveTable) const;
	bool ValidateAndLoadAIPerformanceStrategy();

	bool ShotWillEndUpInHazard(const FAIShotSetupResult& ShotSetupResult) const;
	TOptional<const IPenaltyHazard*> TraceToHazard(const FVector& Location) const;
	FVector GetBounceLocation(const FPredictProjectilePathResult& PathResult) const;
	float GetHitRestitution(const FHitResult& HitResult) const;

	double CalculateDistanceSum(double HorizontalDistance, double VerticalDistance) const;

	void ResetHoleData();

	float GetTraceDistanceToCurrentFocusActor() const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float BounceOverhitCorrectionFactor{ 0.1f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float DefaultAccuracyDeviation{ 0.3f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float DefaultPowerDeviation{ 0.1f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float MinShotPower{ 0.1f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float MinZOffset{ -50.f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float MaxZOffset{ 50.f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float HazardPredictBounceTime{ 3.0f };

	/*
	* Delta to focus actor orientation to offset the shot yaw.  Try the hole direction first.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float YawRetryDelta{ 45.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float MinRetryShotPowerReductionFactor { 0.5f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float MinHeightDistanceReductionFactor{ 0.25f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float MinHeightAdjustmentTreshold{ 100.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float HeightToDistanceRatioStartExp{ 1.0 };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float DefaultHitRestitution{ 0.25f };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float HazardTraceDistance{ 100.0f };

	UPROPERTY(Category = "Shot Arc Prediction", EditDefaultsOnly)
	float MinTraceDistance{ 1000.0f };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	TObjectPtr<UCurveTable> AIConfigCurveTable{};

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	TSubclassOf<UAIPerformanceStrategy> AIPerformanceStrategyClass{};

	UPROPERTY(Category = "Config | Retry", EditDefaultsOnly)
	int32 ConsecutiveFailureCurrentFocusLimit{ 2 };

	UPROPERTY(Category = "Config | Retry", EditDefaultsOnly)
	float MaxPowerDeviationRetry{ 0.2f };

	UPROPERTY(Category = "Config | Retry", EditDefaultsOnly)
	float MaxAccuracyDeviationRetry{ 0.3f };

	/*
	* Minimum fraction [0,1] of distance to the targeted focus actor to consider the shot a success.
	*/
	UPROPERTY(Category = "Config | Retry", EditDefaultsOnly)
	float MinDistanceFractionSuccess{ 0.5f };

	/*
	* Minimum distance to the hole to even consider whether the previous shot was a success.
	* When close to the hole there shouldn't be any blocking obstacles.
	*/
	UPROPERTY(Category = "Config | Retry", EditDefaultsOnly)
	float MinDistanceSuccessTest{ 40 * 1000 };

	/*
	 * How close to the previous shot to consider it as relevant for the current shot.
	*/
	UPROPERTY(Category = "Config | Retry", EditDefaultsOnly)
	float ShotHistoryRadiusRelevance{ 15 * 1000 };

	UPROPERTY(Transient)
	TObjectPtr<UAIPerformanceStrategy> AIPerformanceStrategy{};

	UPROPERTY(Transient)
	FAIShotContext ShotContext{};

	float InitialFocusYaw{};

	UPROPERTY(Transient)
	AActor* FocusActor{};

	TArray<FShotResult> HoleShotResults{};
	mutable float DistanceToHole{};
	int32 CurrentFocusActorFailures{};
	bool bCurrentFocusActorLandedInHazard{};
};

#pragma region Inline Definitions

FORCEINLINE FFlickParams UGolfAIShotComponent::FShotPowerCalculationResult::ToFlickParams() const
{
	return FFlickParams
	{
		.ShotType = ShotType,
		.PowerFraction = PowerFraction
	};
}

FORCEINLINE float UGolfAIShotComponent::FShotSetupParams::GetPowerDeviation() const
{
	return ShotSetupResult.FlickParams.PowerFraction - InitialPowerFraction;
}

#pragma endregion Inline Definitions
