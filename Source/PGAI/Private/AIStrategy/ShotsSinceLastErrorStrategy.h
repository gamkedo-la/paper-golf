// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "AIStrategy/AIPerformanceStrategy.h"

#include "Data/GolfAIConfigData.h"

#include "ShotsSinceLastErrorStrategy.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class UShotsSinceLastErrorStrategy : public UAIPerformanceStrategy
{
	GENERATED_BODY()

public:
	virtual bool Initialize(const PG::FAIPerformanceConfig& Config) override;

	virtual PG::FShotErrorResult CalculateShotError(float PowerFraction) override;

private:
	const FGolfAIConfigData* SelectAIConfigEntry() const;

private:

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	TObjectPtr<UDataTable> AIErrorsDataTable{};

	TArray<FGolfAIConfigData> AIErrorsData{};

	int32 ShotsSinceLastError{};
};
