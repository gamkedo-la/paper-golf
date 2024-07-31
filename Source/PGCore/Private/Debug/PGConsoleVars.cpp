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
}

#endif
