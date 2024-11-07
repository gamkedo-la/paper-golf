// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Utils/GolfAIShotCalculationUtils.h"

#include "Utils/RandUtils.h"

#include "CoreMinimal.h"


float PG::GolfAIShotCalculationUtils::GenerateAccuracy(float MinDeviation, float MaxDeviation)
{
	return RandUtils::RandSign() * FMath::FRandRange(MinDeviation, MaxDeviation);
}

float PG::GolfAIShotCalculationUtils::GeneratePowerFraction(float InPowerFraction, float MinShotPower, float MinDeviation, float MaxDeviation)
{
	const auto Deviation = RandUtils::RandSign() * FMath::FRandRange(MinDeviation, MaxDeviation);
	return FMath::Clamp(InPowerFraction * (1 + Deviation), MinShotPower, 1.0f);
}
