// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PGConstants.h"

#if PG_DEBUG_ENABLED

namespace PG
{
	extern PGCORE_API TAutoConsoleVariable<bool> CShowForces;
	extern PGCORE_API TAutoConsoleVariable<bool> CAutomaticVisualLoggerRecording;
}
#endif
