// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include <concepts>

namespace PG::MathUtils
{
	template<std::floating_point T>
	T ClampDeltaYaw(T YawDelta);

	template<std::floating_point... TArgs>
	auto ClampDeltaYaw(TArgs... Args)
	{
		return ClampDeltaYaw((Args + ...));
	}

	PGCORE_API float GetMaxProjectileHeight(const UObject* WorldContextObject, float FlickPitchAngle, float FlickSpeed);
}

#pragma region Template Definitions

template<std::floating_point T>
T PG::MathUtils::ClampDeltaYaw(T YawDelta)
{
	auto ClampedAngle = FMath::Fmod(YawDelta + 180.0, 360.0);
	if (ClampedAngle < 0.0)
	{
		ClampedAngle += 360.0;
	}

	ClampedAngle -= 180.0;

	return ClampedAngle;
}

#pragma endregion Template Definitions
