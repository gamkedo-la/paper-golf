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
	};

	UGolfAIShotComponent();

	FAIShotSetupResult SetupShot(const FAIShotContext& ShotContext);

protected:
	virtual void BeginPlay() override;

private:

	TOptional<FFlickParams> CalculateFlickParams() const;
	FFlickParams CalculateDefaultFlickParams() const;

	float GenerateAccuracy() const;
	float GeneratePowerFraction(float InPowerFraction) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float BounceOverhitCorrectionFactor{ 0.075f };

	// TODO: Use curve table
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float AccuracyDeviation{ 0.3f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float PowerDeviation{ 0.1f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float MinShotPower{ 0.1f };

	UPROPERTY(Transient)
	FAIShotContext ShotContext{};
};
