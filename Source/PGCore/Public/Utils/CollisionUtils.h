// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"


namespace PG::CollisionUtils
{
	PGCORE_API FBox GetAABB(const AActor& Actor);

	PGCORE_API FBox GetAABB(const USceneComponent& Component);

	struct PGCORE_API FGroundData
	{
		FVector Location;
		FVector Normal;
	};

	PGCORE_API float GetActorHalfHeight(const AActor& Actor);

	PGCORE_API TOptional<FGroundData> GetGroundData(const AActor& Actor, const TOptional<FVector>& PositionOverride = {});

	PGCORE_API void ResetActorToGround(const FGroundData& GroundData, AActor& Actor, float AdditionalZOffset = 0.0f);
}

namespace PG::CollisionChannel
{
	inline constexpr ECollisionChannel FlickTraceType = ECollisionChannel::ECC_GameTraceChannel1;
	inline constexpr ECollisionChannel StaticObstacleTrace = ECollisionChannel::ECC_GameTraceChannel2;
}


namespace PG::CollisionObjectType
{
	inline constexpr ECollisionChannel Hazard = ECollisionChannel::ECC_GameTraceChannel3;
}

namespace PG::CollisionProfile
{
	inline constexpr const TCHAR* NoCollision = TEXT("NoCollision");
	inline constexpr const TCHAR* OverlapOnlyPawn = TEXT("OverlapPawnIgnoreElse");
	inline constexpr const TCHAR* Hazard = TEXT("Hazard");
}
