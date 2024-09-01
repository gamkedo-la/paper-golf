// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "PaperGolfTypes.h"

#include "Data/GolfAIConfigData.h"

#include "Pawn/PaperGolfPawn.h"

#include "GolfAIShotComponent.generated.h"

class APaperGolfPawn;
class AGolfPlayerState;

class UCurveTable;

USTRUCT()
struct FAIShotContext
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<APaperGolfPawn> PlayerPawn{};

	UPROPERTY(Transient)
	TObjectPtr<AGolfPlayerState> PlayerState{};

	UPROPERTY(Transient)
	AActor* GolfHole{};

	UPROPERTY(Transient)
	TArray<FShotFocusScores> FocusActorScores{};

	// TODO: May want to pass in the full array of focus actors to select another target

	EShotType ShotType{ EShotType::Default };

	FString ToString() const;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UGolfAIShotComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	struct FAIShotSetupResult
	{
		FFlickParams FlickParams{};
		AActor* FocusActor{};
		float ShotPitch{};
		float ShotYaw{};

		FString ToString() const;
	};

	UGolfAIShotComponent();

	FAIShotSetupResult SetupShot(FAIShotContext&& ShotContext);

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

	struct FShotErrorResult
	{
		float PowerFraction{};
		float Accuracy{};
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
	FShotErrorResult CalculateShotError(float PowerFraction);

	float GenerateAccuracy(float MinDeviation, float MaxDeviation) const;
	float GeneratePowerFraction(float InPowerFraction, float MinDeviation, float MaxDeviation) const;

	float CalculateDefaultZOffset() const;

	FShotCalculationResult CalculateAvoidanceShotFactors(const FVector& FlickLocation, float PreferredShotAngle, float FlickMaxSpeed, float PowerFraction) const;

	TTuple<bool, float> CalculateShotPitch(const FVector& FlickLocation, const FVector& FlickDirection, float FlickSpeed) const;

	float GetMaxProjectileHeight(float FlickPitchAngle, float FlickSpeed) const;
	bool TraceShotAngle(const APaperGolfPawn& PlayerPawn, const FVector& TraceStart, const FVector& FlickDirection, float FlickSpeed, float FlickAngleDegrees) const;

	FVector GetFocusActorLocation(const FVector& FlickLocation) const;

	bool ValidateAndLoadConfig();
	bool ValidateCurveTable(UCurveTable* CurveTable) const;

	const FGolfAIConfigData* SelectAIConfigEntry() const;

	void LoadWorldData();

	bool ShotWillEndUpInHazard(const FAIShotSetupResult& ShotSetupResult) const;

	float GetHitRestitution(const FHitResult& HitResult) const;

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

	UPROPERTY(Category = "Shot Arc Prediction", EditDefaultsOnly)
	float MinTraceDistance{ 1000.0f };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	TObjectPtr<UCurveTable> AIConfigCurveTable{};

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	TObjectPtr<UDataTable> AIErrorsDataTable{};

	TArray<FGolfAIConfigData> AIErrorsData{};

	UPROPERTY(Transient)
	TArray<AActor*> HazardActors{};

	UPROPERTY(Transient)
	FAIShotContext ShotContext{};

	float InitialFocusYaw{};

	UPROPERTY(Transient)
	AActor* FocusActor{};

	int32 ShotsSinceLastError{};
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
