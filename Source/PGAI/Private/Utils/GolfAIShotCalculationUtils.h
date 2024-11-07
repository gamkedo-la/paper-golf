// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

namespace PG::GolfAIShotCalculationUtils
{
	float GenerateAccuracy(float MinDeviation, float MaxDeviation);
	float GeneratePowerFraction(float InPowerFraction, float MinShotPower, float MinDeviation, float MaxDeviation);
}
