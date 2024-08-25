// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "PaperGolfTypes.h"

#include "Pawn/PaperGolfPawn.h"

#include "GolfAIShotComponent.generated.h"

class APaperGolfPawn;
class AGolfPlayerState;

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

	TOptional<FAIShotSetupResult> CalculateShotParams() const;
	FAIShotSetupResult CalculateDefaultShotParams() const;

	float GenerateAccuracy() const;
	float GeneratePowerFraction(float InPowerFraction) const;

	float CalculateZOffset() const;

	struct FShotCalculationResult
	{
		float Pitch{};
		float Yaw{};
		float PowerMultiplier{ 1.0f };
	};

	FShotCalculationResult CalculateShotFactors(const FVector& FlickLocation, float FlickMaxSpeed, float PowerFraction) const;

	TTuple<bool, float> CalculateShotPitch(const FVector& FlickLocation, const FVector& FlickDirection, float FlickMaxSpeed, float PowerFraction) const;

	float GetMaxProjectileHeight(float FlickPitchAngle, float FlickSpeed) const;
	bool TraceShotAngle(const APaperGolfPawn& PlayerPawn, const FVector& TraceStart, const FVector& FlickDirection, float FlickSpeed, float FlickAngleDegrees) const;

	FVector GetFocusActorLocation(const FVector& FlickLocation) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float BounceOverhitCorrectionFactor{ 0.1f };

	// TODO: Use curve table
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float AccuracyDeviation{ 0.3f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float PowerDeviation{ 0.1f };

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

	UPROPERTY(Transient)
	FAIShotContext ShotContext{};
};
