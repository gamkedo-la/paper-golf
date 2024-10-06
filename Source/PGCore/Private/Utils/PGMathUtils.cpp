// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "Utils/PGMathUtils.h"

float PG::MathUtils::GetMaxProjectileHeight(const UObject* WorldContextObject, float FlickPitchAngle, float FlickSpeed)
{
	if (!ensure(WorldContextObject))
	{
		return 0.0f;
	}

	// H = v^2 * sin^2(theta) / 2g
	auto World = WorldContextObject->GetWorld();
	check(World);

	return FMath::Square(FlickSpeed) * FMath::Square(FMath::Sin(FMath::DegreesToRadians(FlickPitchAngle))) / (2 * FMath::Abs(World->GetGravityZ()));
}
