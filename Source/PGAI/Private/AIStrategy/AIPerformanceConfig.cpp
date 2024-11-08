// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "AIStrategy/AIPerformanceConfig.h"

#include "Utils/PGDataTableUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AIPerformanceConfig)

TArray<FAIPerformanceConfigData> AIPerformanceConfigDataTableParser::ReadAll(UDataTable* GolfAIDataTable)
{
	return PG::DataTableUtils::ReadAll<FAIPerformanceConfigData>(GolfAIDataTable, true, "AIPerformanceConfigDataTableParser::ReadAll");
}

FString FAIPerformanceConfigData::ToString() const
{
	return FString::Printf(TEXT("AccuracyDeltaMin=%.2f; AccuracyDeltaMax=%.2f; PowerDeltaMin=%.2f; PowerDeltaMax=%.2f"), AccuracyDeltaMin, AccuracyDeltaMax, PowerDeltaMin, PowerDeltaMax);
}
