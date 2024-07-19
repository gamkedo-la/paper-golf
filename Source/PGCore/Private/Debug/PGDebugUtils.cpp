// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "Debug/PGDebugUtils.h"

#if PG_DEBUG_ENABLED

#include "Debug/PGConsoleVars.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"

using namespace PG;

namespace PG::DebugUtils
{
	void DrawCenterOfMass(const UPrimitiveComponent* Component)
	{
		if (!Component || !CShowForces.GetValueOnGameThread())
		{
			return;
		}
		auto BodyInstance = Component->GetBodyInstance();
		if (!BodyInstance)
		{
			return;
		}

		DrawDebugSphere(Component->GetWorld(), BodyInstance->GetCOMPosition(), 50, 8, FColor::Yellow, false, 0, 100);
	}

	void DrawForceAtLocation(const UPrimitiveComponent* Component, const FVector& Force, const FVector& Location, const FColor& Color, float Scale)
	{
		if (!Component || !CShowForces.GetValueOnGameThread())
		{
			return;
		}

		const auto ForceDirection = Force.GetSafeNormal();

		DrawDebugDirectionalArrow(
			Component->GetWorld(),
			Location,
			Location + Force.Size() / Scale * ForceDirection,
			20,
			Color,
			false,
			0,
			100
		);
	}
}
#endif
