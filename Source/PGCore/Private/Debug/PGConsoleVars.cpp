// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "Debug/PGConsoleVars.h"

#if PG_DEBUG_ENABLED

namespace PG
{
	TAutoConsoleVariable<bool> CShowForces(
		TEXT("pg.showForces"),
		false,
		TEXT("Toggle on/off"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	TAutoConsoleVariable<bool> CAutomaticVisualLoggerRecording(
		TEXT("pg.vislog.autorecord"),
		true,
		TEXT("Toggle on/off"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CStartHoleOverride(
		TEXT("pg.startHole"),
		-1,
		TEXT("Override the start hole number"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CPlayerAccuracyExponent(
		TEXT("pg.diff.pAccExp"),
		-1.0f,
		TEXT("Adjust client player shot accuracy exponent >= 1"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CPlayerMaxAccuracy(
		TEXT("pg.diff.pAccMax"),
		-1.0f,
		TEXT("Adjust client player worst case shot accuracy [0,1]. Higher is harder"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CGlobalPowerAccuracyExponent(
		TEXT("pg.diff.gAccExp"),
		-1.0f,
		TEXT("Adjust all players' shot accuracy exponent >= 1"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CGlobalPowerAccuracyDampenExponent(
		TEXT("pg.diff.gPowExp"),
		-1.0f,
		TEXT("Adjust all players' accuracy effect on shot power reduction >= 0. Larger is more difficult"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CGlobalMinPowerMultiplier(
		TEXT("pg.diff.gPowMin"),
		-1.0f,
		TEXT("Adjust all players' min power reduction from accuracy [0,1]"),
		ECVF_Scalability | ECVF_RenderThreadSafe);
}

#endif
