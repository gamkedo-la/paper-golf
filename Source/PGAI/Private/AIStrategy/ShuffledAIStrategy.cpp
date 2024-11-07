// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "AIStrategy/ShuffledAIStrategy.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGAILogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShuffledAIStrategy)

using namespace PG;

bool UShuffledAIStrategy::Initialize(const PG::FAIPerformanceConfig& Config)
{
	return Super::Initialize(Config);
}

FShotErrorResult UShuffledAIStrategy::CalculateShotError(float PowerFraction)
{
	// TODO: Implement
	return Super::CalculateShotError(PowerFraction);
}
