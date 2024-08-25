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
		float ShotPitch{};
		float ShotYaw{};
	};

	UGolfAIShotComponent();

	FAIShotSetupResult SetupShot(const FAIShotContext& ShotContext);

protected:
	virtual void BeginPlay() override;

private:

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
	};

	TOptional<FAIShotSetupResult> CalculateShotParams();
	FAIShotSetupResult CalculateDefaultShotParams() const;

	FShotCalibrationResult CalibrateShot(const FVector& FlickLocation, float PowerFraction) const;
	FShotErrorResult CalculateShotError(float PowerFraction);

	float GenerateAccuracy(float Deviation) const;
	float GeneratePowerFraction(float InPowerFraction, float Deviation) const;

	float CalculateZOffset() const;

	FShotCalculationResult CalculateShotFactors(const FVector& FlickLocation, float PreferredShotAngle, float FlickMaxSpeed, float PowerFraction) const;

	TTuple<bool, float> CalculateShotPitch(const FVector& FlickLocation, const FVector& FlickDirection, float FlickSpeed) const;

	float GetMaxProjectileHeight(float FlickPitchAngle, float FlickSpeed) const;
	bool TraceShotAngle(const APaperGolfPawn& PlayerPawn, const FVector& TraceStart, const FVector& FlickDirection, float FlickSpeed, float FlickAngleDegrees) const;

	FVector GetFocusActorLocation(const FVector& FlickLocation) const;

	bool ValidateAndLoadConfig();
	bool ValidateCurveTable(UCurveTable* CurveTable) const;

	const FGolfAIConfigData* SelectAIConfigEntry() const;

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
	FAIShotContext ShotContext{};

	int32 ShotsSinceLastError{};
};
