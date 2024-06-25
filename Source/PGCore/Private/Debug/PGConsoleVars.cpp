// Copyright Game Salutes. All Rights Reserved.

#include "Debug/PGConsoleVars.h"

#if PG_DEBUG_ENABLED

namespace PG
{
	TAutoConsoleVariable<bool> CShowForces(
		TEXT("pg.showForces"),
		false,
		TEXT("Toggle on/off"),
		ECVF_Scalability | ECVF_RenderThreadSafe);
}

#endif
