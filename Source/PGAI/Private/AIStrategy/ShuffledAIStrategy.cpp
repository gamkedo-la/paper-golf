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

FShotErrorResult UShuffledAIStrategy::CalculateShotError(const FFlickParams& FlickParams)
{
	// TODO: Select the correct strategy based on the shot type
	// Choose the next outcome in the array and if reach the end then reshuffle
	// 
	// TODO: Implement
	return Super::CalculateShotError(FlickParams);
}
