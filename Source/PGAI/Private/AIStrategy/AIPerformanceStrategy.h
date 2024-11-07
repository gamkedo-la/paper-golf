// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "PaperGolfTypes.h"

#include "AIPerformanceStrategy.generated.h"

namespace PG
{
	struct FShotErrorResult
	{
		float PowerFraction{};
		float Accuracy{};
	};

	struct FAIPerformanceConfig
	{
		float MinPower{};
		float DefaultAccuracyDeviation{};
		float DefaultPowerDeviation{};

		FString ToString() const;
	};
}

/**
 * Controls how the AI deviates from a perfect shot.
 */
UCLASS(Abstract, Blueprintable)
class UAIPerformanceStrategy : public UObject
{
	GENERATED_BODY()

public:

	virtual bool Initialize(const PG::FAIPerformanceConfig& Config);
	virtual PG::FShotErrorResult CalculateShotError(const FFlickParams& FlickParams);

protected:
	PG::FAIPerformanceConfig AIConfig{};
};
