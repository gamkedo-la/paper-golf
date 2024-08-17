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
	float CalculateShotPitch() const;

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

	UPROPERTY(Transient)
	FAIShotContext ShotContext{};
};
