// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "AIStrategy/AIPerformanceStrategy.h"

#include "Utils/GolfAIShotCalculationUtils.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGAILogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AIPerformanceStrategy)

using namespace PG;

bool UAIPerformanceStrategy::Initialize(const PG::FAIPerformanceConfig& Config)
{
	UE_VLOG_UELOG(GetOuter(), LogPGAI, Log, TEXT("%s-%s: Initialize - Config=%s"),
		*LoggingUtils::GetName(GetOuter()), *GetName(), *Config.ToString());

	AIConfig = Config;

	return true;
}

FShotErrorResult UAIPerformanceStrategy::CalculateShotError(const FFlickParams& FlickParams)
{
	const auto PowerFraction = FlickParams.PowerFraction;

	return FShotErrorResult
	{
		.PowerFraction = GolfAIShotCalculationUtils::GeneratePowerFraction(PowerFraction, AIConfig.MinPower, -AIConfig.DefaultPowerDeviation, AIConfig.DefaultPowerDeviation),
		.Accuracy = GolfAIShotCalculationUtils::GenerateAccuracy(-AIConfig.DefaultAccuracyDeviation, AIConfig.DefaultAccuracyDeviation),
	};
}

FString PG::FAIPerformanceConfig::ToString() const
{
	return FString::Printf(TEXT("MinPower=%.2f; DefaultAccuracyDeviation=%.2f; DefaultPowerDeviation=%.2f"), MinPower, DefaultAccuracyDeviation, DefaultPowerDeviation);
}
