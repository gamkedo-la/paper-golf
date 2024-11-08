// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Utils/GolfAIShotCalculationUtils.h"

#include "Utils/RandUtils.h"

#include "CoreMinimal.h"

namespace
{
	float RandDeviation(float MinDeviation, float MaxDeviation)
	{
		return !FMath::IsNearlyEqual(MinDeviation, MaxDeviation) ? FMath::FRandRange(MinDeviation, MaxDeviation) : MinDeviation;
	}

	int32 RandSign(float SignBias)
	{
		if (FMath::IsNearlyZero(SignBias))
		{
			return RandUtils::RandSign();
		}

		const auto Value = FMath::FRandRange(-1.0f, 1.0f);
		return Value <= SignBias ? 1 : -1;
	}
}

float PG::GolfAIShotCalculationUtils::GenerateAccuracy(float MinDeviation, float MaxDeviation)
{
	return RandUtils::RandSign() * RandDeviation(MinDeviation, MaxDeviation);
}

float PG::GolfAIShotCalculationUtils::GeneratePowerFraction(float InPowerFraction, float MinShotPower, float MinDeviation, float MaxDeviation, float SignBias)
{
	const auto Deviation = RandSign(SignBias) * RandDeviation(MinDeviation, MaxDeviation);
	return FMath::Clamp(InPowerFraction * (1 + Deviation), MinShotPower, 1.0f);
}
