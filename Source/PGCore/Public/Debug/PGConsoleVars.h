// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "PGConstants.h"

#if PG_DEBUG_ENABLED

namespace PG
{
	extern PGCORE_API TAutoConsoleVariable<bool> CShowForces;
	extern PGCORE_API TAutoConsoleVariable<bool> CAutomaticVisualLoggerRecording;
	extern PGCORE_API TAutoConsoleVariable<int32> CStartHoleOverride;

	extern PGCORE_API TAutoConsoleVariable<float> CPlayerAccuracyExponent;
	extern PGCORE_API TAutoConsoleVariable<float> CPlayerMaxAccuracy;
	extern PGCORE_API TAutoConsoleVariable<float> CGlobalPowerAccuracyDampenExponent;
	extern PGCORE_API TAutoConsoleVariable<float> CGlobalMinPowerMultiplier;
	extern PGCORE_API TAutoConsoleVariable<float> CGlobalPowerAccuracyExponent;
}
#endif
