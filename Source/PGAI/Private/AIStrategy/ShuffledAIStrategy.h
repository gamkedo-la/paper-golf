// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "AIStrategy/AIPerformanceStrategy.h"

#include "PaperGolfTypes.h"

#include "AIPerformanceConfig.h"

#include "ShuffledAIStrategy.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class UShuffledAIStrategy : public UAIPerformanceStrategy
{
	GENERATED_BODY()

public:
	virtual bool Initialize(const PG::FAIPerformanceConfig& Config) override;
	virtual PG::FShotErrorResult CalculateShotError(const FFlickParams& FlickParams) override;

private:
	float MinShotPower{};

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	TMap<EShotType, UDataTable*> ShotTypeStrategyTableMap{};

	TMap<EShotType, TArray<FAIPerformanceConfigData>> ShotTypeStrategyDataMap{};
	TMap<EShotType, int32> ShotTypeStrategyIndexMap{};
};
