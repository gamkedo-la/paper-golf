// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "AIStrategy/AIPerformanceStrategy.h"

#include "PaperGolfTypes.h"

#include "AIPerformanceConfig.h"

#include "ShuffledAIStrategy.generated.h"

USTRUCT()
struct FShuffleAIStrategyShotTypeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, meta=(ClampMin = "10"))
	int32 NumOutcomes{ 10 };

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RoundUpFraction{ 0.9 };

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UDataTable> DataTable{};
};

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
	bool InitializeShotTypeStrategyData();
	TArray<FAIPerformanceConfigData> CreateShotTypeEntries(EShotType ShotType, const FShuffleAIStrategyShotTypeConfig& ConfigEntry) const;
	const FAIPerformanceConfigData* GetNextConfigEntry(EShotType ShotType);

	void ShuffleEntries(TArray<FAIPerformanceConfigData>& Entries) const;

private:

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	TMap<EShotType, FShuffleAIStrategyShotTypeConfig> ShotTypeStrategyTableMap{};

	TMap<EShotType, TArray<FAIPerformanceConfigData>> ShotTypeStrategyDataMap{};
	TMap<EShotType, int32> ShotTypeStrategyIndexMap{};
};
