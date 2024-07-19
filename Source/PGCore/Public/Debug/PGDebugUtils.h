// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "PGConstants.h"

class UPrimitiveComponent;

namespace PG::DebugUtils
{
	#if PG_DEBUG_ENABLED
		PGCORE_API void DrawCenterOfMass(const UPrimitiveComponent* Component);
		PGCORE_API void DrawForceAtLocation(const UPrimitiveComponent* Component, const FVector& Force, const FVector& Location, const FColor& Color = FColor::Red, float Scale = 1e4);
	#else
		inline void DrawCenterOfMass(const UPrimitiveComponent* component) {}
		inline void DrawForceAtLocation(const UPrimitiveComponent* Component, const FVector& Force, const FVector& Location, const FColor& Color = FColor::Red, float Scale = 1e4) {}
#endif
}
