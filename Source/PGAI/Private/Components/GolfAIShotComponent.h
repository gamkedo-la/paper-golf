// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "PGAITypes.h"

#include "Data/GolfAIConfigData.h"
#include "GolfAIShotComponent.generated.h"

class APaperGolfPawn;
class UCurveTable;
class UAIPerformanceStrategy;

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

	TOptional<FAIShotSetupResult> CalculateShotParams();
	TOptional<FAIShotSetupResult> CalculateShotParamsForCurrentFocusActor();

	FAIShotSetupResult CalculateDefaultShotParams() const;

	TOptional<FShotPowerCalculationResult> CalculateInitialShotParams() const;
	FShotCalibrationResult CalibrateShot(const FVector& FlickLocation, float PowerFraction) const;

	float CalculateDefaultZOffset() const;

	FShotCalculationResult CalculateAvoidanceShotFactors(const FVector& FlickLocation, float PreferredShotAngle, float FlickMaxSpeed, float PowerFraction) const;

	TTuple<bool, float> CalculateShotPitch(const FVector& FlickLocation, const FVector& FlickDirection, float FlickSpeed) const;

	FVector GetFocusActorLocation(const FVector& FlickLocation) const;

	bool ValidateAndLoadConfig();
	bool ValidateCurveTable(UCurveTable* CurveTable) const;
	bool ValidateAndLoadAIPerformanceStrategy();

	void LoadWorldData();

	bool ShotWillEndUpInHazard(const FAIShotSetupResult& ShotSetupResult) const;

	float GetHitRestitution(const FHitResult& HitResult) const;

	double CalculateDistanceSum(double HorizontalDistance, double VerticalDistance) const;

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

	UPROPERTY(Category = "Shot Arc Prediction", EditDefaultsOnly)
	float MinTraceDistance{ 1000.0f };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	TObjectPtr<UCurveTable> AIConfigCurveTable{};

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	TSubclassOf<UAIPerformanceStrategy> AIPerformanceStrategyClass{};

	UPROPERTY(Transient)
	TObjectPtr<UAIPerformanceStrategy> AIPerformanceStrategy{};

	TArray<FGolfAIConfigData> AIErrorsData{};

	UPROPERTY(Transient)
	TArray<AActor*> HazardActors{};

	UPROPERTY(Transient)
	FAIShotContext ShotContext{};

	float InitialFocusYaw{};

	UPROPERTY(Transient)
	AActor* FocusActor{};
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

#pragma endregion Inline Definitions
