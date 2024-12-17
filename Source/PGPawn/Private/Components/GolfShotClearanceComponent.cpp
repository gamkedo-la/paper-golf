// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/GolfShotClearanceComponent.h"


#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"

#include "Utils/VisualLoggerUtils.h"

#include "Utils/CollisionUtils.h"

#include "Components/StaticMeshComponent.h"

#include "PGPawnLogging.h"

#include "PGConstants.h"
#include "Debug/PGConsoleVars.h"

#include <array>


#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfShotClearanceComponent)

namespace
{
	const FVector DefaultPosition{ 1e9, 1e9, 1e9 };
	constexpr float MinUpdateDeltaTime = 0.1f;
}

UGolfShotClearanceComponent::UGolfShotClearanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool UGolfShotClearanceComponent::AdjustPositionForClearance()
{
	if (!bEnabled)
	{
		return false;
	}

	auto World = GetWorld();
	if (!ensureAlways(World))
	{
		return false;
	}

	// Return early if called multiple times in same frame
	if (const auto TimeSeconds = World->GetTimeSeconds(); FMath::IsNearlyEqual(LastPositionUpdateWorldTime, TimeSeconds, MinUpdateDeltaTime))
	{
		return false;
	}
	else
	{
		LastPositionUpdateWorldTime = TimeSeconds;
	}

	if (!ShouldAdjustPosition())
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - FALSE - ShouldAdjustPosition returned FALSE"),
			*LoggingUtils::GetName(GetOwner()), *GetName());

		bFirstShot = false;
		if (auto MyOwner = GetOwner(); MyOwner)
		{
			LastPosition = MyOwner->GetActorLocation();
		}

		return false;
	}

	FHitResultData ClearanceHitResult;
	if (!IsClearanceNeeded(ClearanceHitResult))
	{
		return false;
	}

	FVector NewPosition;

	if (!CalculateClearanceLocation(ClearanceHitResult, NewPosition))
	{
		return false;
	}

	auto MyOwner = GetOwner();
	check(MyOwner);

	MyOwner->SetActorLocation(NewPosition);
	LastPosition = NewPosition;

	return true;
}

void UGolfShotClearanceComponent::BeginPlay()
{
	Super::BeginPlay();

	LastPosition = TargetPosition = DefaultPosition;
	LastPositionUpdateWorldTime = -1.0f;
	bFirstShot = true;

	HitNormalAlignmentAngleCos = FMath::Cos(FMath::DegreesToRadians(HitNormalAlignmentAngle));
}

bool UGolfShotClearanceComponent::ShouldAdjustPosition() const
{
	const auto MyOwner = GetOwner();
	if (!ensureAlways(MyOwner))
	{
		return false;
	}

	if (PositionAdjustmentNotAllowed())
	{
		UE_VLOG_UELOG(MyOwner, LogPGPawn, Verbose, TEXT("%s-%s: ShouldAdjustPosition - FALSE - Not Allowed"),
			*LoggingUtils::GetName(MyOwner), *GetName());
		return false;
	}

	const auto& CurrentPosition = MyOwner->GetActorLocation();

	// First check if current position is the same as the last position so no need to recalculate again
	const auto DistSqLastPosition = FVector::DistSquared(LastPosition, CurrentPosition);
	const auto LastPositionThresholdSq = FMath::Square(RecalculationMinDistance);

	if (DistSqLastPosition <= LastPositionThresholdSq)
	{
		UE_VLOG_UELOG(MyOwner, LogPGPawn, Verbose, TEXT("%s-%s: ShouldAdjustPosition - FALSE - LastPosition=%s close to CurrentPosition=%s; Distance=%fm"),
			*LoggingUtils::GetName(MyOwner), *GetName(), *LastPosition.ToString(), *CurrentPosition.ToString(), FMath::Sqrt(DistSqLastPosition) / 100);
		return false;
	}

	// If target not set then return true
	if (TargetPosition.Equals(DefaultPosition))
	{
		return true;
	}

	// Check distance to the target position
	const auto DistSqTarget = FVector::DistSquared(CurrentPosition, TargetPosition);
	const auto TargetMinDistanceSq = FMath::Square(TargetMinDistance);

	if (DistSqTarget <= TargetMinDistanceSq)
	{
		UE_VLOG_UELOG(MyOwner, LogPGPawn, Verbose, TEXT("%s-%s: ShouldAdjustPosition - FALSE - CurrentPosition=%s close to TargetPosition=%s; Distance=%fm"),
			*LoggingUtils::GetName(MyOwner), *GetName(), *CurrentPosition.ToString(), *TargetPosition.ToString(), FMath::Sqrt(DistSqTarget) / 100);
		return false;
	}

	return true;
}

bool UGolfShotClearanceComponent::PositionAdjustmentNotAllowed() const
{
#if PG_DEBUG_ENABLED

	if (bFirstShot)
	{
		FVector StartPositionOverride;
		// If in debug mode, check for a start position override and in that case allow adjustment on the first shot
		if (const auto& OverridePosString = PG::CStartPositionOverride.GetValueOnGameThread(); !OverridePosString.IsEmpty() && StartPositionOverride.InitFromCompactString(OverridePosString))
		{
			UE_VLOG_UELOG(GetOwner(), LogPGPawn, Verbose, TEXT("%s-%s: PositionAdjustmentNotAllowed - FALSE - overriding first shot condition as valid StartPositionOverride=%s provided"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *StartPositionOverride.ToCompactString());

			return false;
		}
	}
#endif

	// Don't adjust first shot
	UE_CVLOG_UELOG(bFirstShot, GetOwner(), LogPGPawn, Verbose, TEXT("%s-%s: PositionAdjustmentNotAllowed - TRUE - first shot"),
		*LoggingUtils::GetName(GetOwner()), *GetName());

	return bFirstShot;
}

float UGolfShotClearanceComponent::GetActorHeight() const
{
	if (ActorHeight > 0)
	{
		return ActorHeight;
	}

	const auto MyOwner = GetOwner();
	check(MyOwner);

	const auto Bounds = GetOwnerAABB();
	const auto& Extent = Bounds.GetExtent();

	ActorHeight = FMath::Max3(Extent.X, Extent.Y, Extent.Z) * 2;

	UE_VLOG_UELOG(MyOwner, LogPGPawn, Log, TEXT("%s-%s: GetActorHeight - %f"),
		*LoggingUtils::GetName(MyOwner), *GetName(), ActorHeight);

	return ActorHeight;
}

FBox UGolfShotClearanceComponent::GetOwnerAABB() const
{
	const auto MyOwner = GetOwner();
	check(MyOwner);

	FBox Bounds;

	if (const auto Mesh = MyOwner->FindComponentByClass<UStaticMeshComponent>(); Mesh)
	{
		return PG::CollisionUtils::GetAABB(*Mesh);
	}

	return PG::CollisionUtils::GetAABB(*MyOwner);
}

bool UGolfShotClearanceComponent::IsClearanceNeeded(FHitResultData& OutHitResultData) const
{
	const auto MyOwner = GetOwner();
	check(MyOwner);

	auto World = GetWorld();
	check(World);

	const auto& CurrentPosition = MyOwner->GetActorLocation();
	const auto& ForwardDirection = MyOwner->GetActorForwardVector();
	const auto& UpVector = MyOwner->GetActorUpVector();

	const auto TraceHeight = GetActorHeight() * 0.5f;

	// Rotator from ActorUp
	const auto RotationQuat = ForwardDirection.ToOrientationQuat();

	// Offset so overlap is at the top of where we want to adjust
	const auto TraceOffset = UpVector * TraceHeight * 2.0f;
	const auto TracePosition = CurrentPosition + TraceOffset;
	const auto TraceShape = FCollisionShape::MakeBox(FVector{ AdjustmentDistance, AdjustmentDistance, TraceHeight });

	// Cannot get the hit normal from the sweep trace with a box shape so need to do a line trace after to the hit location direction
	bool bResult = World->OverlapAnyTestByChannel(TracePosition, RotationQuat, PG::CollisionChannel::StaticObstacleTrace, TraceShape);

#if ENABLE_VISUAL_LOG
	if (bResult && FVisualLogger::IsRecording())
	{
		// Place at origin as translation is provided by the transform
		const auto LogShape = FBox::BuildAABB(FVector::ZeroVector, TraceShape.GetExtent());

		const FTransform LogTransform{ RotationQuat, TracePosition };

		UE_VLOG_OBOX(MyOwner, LogPGPawn, Log, LogShape, LogTransform.ToMatrixNoScale(),
			bResult ? FColor::Red : FColor::Green, TEXT("Clearance"));
	}
#endif

	if (bResult)
	{
		// Send out line traces to get hit location and normal
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.bIgnoreBlocks = false;
		QueryParams.bIgnoreTouches = true;
		QueryParams.bFindInitialOverlaps = false;

		const auto& RightVector = MyOwner->GetActorRightVector();

		for (int32 Index = 0; const auto& Direction : { ForwardDirection, -ForwardDirection, RightVector, -RightVector })
		{
			FHitResult HitResult;
			const auto TraceEnd = TracePosition + Direction * AdjustmentDistance * 2;

			if (World->LineTraceSingleByChannel(HitResult, TracePosition, TraceEnd, PG::CollisionChannel::StaticObstacleTrace, QueryParams))
			{
				OutHitResultData.Location = HitResult.Location;
				OutHitResultData.Normal = HitResult.Normal;

				UE_VLOG_UELOG(MyOwner, LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - TRUE - HitLocation=%s; HitNormal=%s"),
					*LoggingUtils::GetName(MyOwner), *GetName(), *OutHitResultData.Location.ToCompactString(), *OutHitResultData.Normal.ToCompactString());

				UE_VLOG_ARROW(MyOwner, LogPGPawn, Log, HitResult.Location, HitResult.Location + HitResult.Normal * 100.0f, FColor::Yellow, TEXT("Clr: Hit Normal(%d)"), Index);

				return true;
			}
			else
			{
				UE_VLOG_ARROW(MyOwner, LogPGPawn, VeryVerbose, TracePosition, TraceEnd, FColor::Green, TEXT("Clr: No Hit(%d)"), Index);
			}

			++Index;
		}
	}

	UE_VLOG_UELOG(MyOwner, LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - FALSE - No valid overlap detected"),
			*LoggingUtils::GetName(MyOwner), *GetName());

	return false;
}

bool UGolfShotClearanceComponent::CalculateClearanceLocation(const FHitResultData& HitResultData, FVector& OutNewLocation) const
{
	// Prefer the hit normal and then fallback toward the previous shot

	const auto MyOwner = GetOwner();
	check(MyOwner);

	const auto& UpVector = MyOwner->GetActorUpVector();
	const auto& HitNormal = HitResultData.Normal;

	std::array<FVector, 2> PushbackDirections;
	auto End = PushbackDirections.begin();

	// Immediately discount the hit normal if it is pointing too much in direction of actor up vector
	if ((UpVector | HitNormal) <= HitNormalAlignmentAngleCos)
	{
		UE_VLOG_UELOG(MyOwner, LogPGPawn, Verbose, TEXT("%s-%s: AdjustPositionForClearance - Add HitNormal=%s"),
			*LoggingUtils::GetName(MyOwner), *GetName(), *HitNormal.ToCompactString());

		*End++ = HitNormal;
	}
	else
	{
		UE_VLOG_UELOG(MyOwner, LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - Hit normal %s is too aligned with actor up vector %s"),
			*LoggingUtils::GetName(MyOwner), *GetName(), *HitNormal.ToCompactString(), *UpVector.ToCompactString());

		UE_VLOG_ARROW(MyOwner, LogPGPawn, Log, HitResultData.Location, HitResultData.Location + 100.0f * HitNormal, FColor::Orange, TEXT("Clearance Normal"));
	}

	// Pushback to last shot location by default
	// Don't do this if in debug mode and this is the first shot
	if (!PG_DEBUG_ENABLED || !bFirstShot)
	{
		*End++ = (LastPosition - MyOwner->GetActorLocation()).GetSafeNormal();
	}

	for (auto It = PushbackDirections.begin(); It != End; ++It)
	{
		if (CalculateClearanceLocation(HitResultData.Location, *It, OutNewLocation))
		{
			return true;
		}
	}

	return false;
}

TOptional<UGolfShotClearanceComponent::FClearanceLocationResult> UGolfShotClearanceComponent::CalculateClearanceLocationResult(const FVector& HitLocation, const FVector& RawPushbackDirection) const
{
	const auto MyOwner = GetOwner();
	check(MyOwner);

	const auto& CurrentPosition = MyOwner->GetActorLocation();
	const auto& UpVector = MyOwner->GetActorUpVector();

	// Project along plane with up vector
	const auto PushbackDirection = FVector::VectorPlaneProject(RawPushbackDirection, UpVector).GetSafeNormal();

	// Push back toward the last position and then retest
	// Project current distance along the pushback direction
	const auto ToHitLocationProjection = (CurrentPosition - HitLocation).ProjectOnToNormal(PushbackDirection);
	const auto ToHitLocationDistance = ToHitLocationProjection.Size();
	const auto PushbackDistance = AdjustmentDistance - ToHitLocationDistance;

	if (PushbackDistance <= 0)
	{
		UE_VLOG_UELOG(MyOwner, LogPGPawn, Log,
			TEXT("%s-%s: AdjustPositionForClearance - FALSE - Pushback direction: %s from location %s resulted in no pushback distance: ToHitLocationDistance=%fm; PushbackDistance=%fm; ToHitLocationProjection=%s"),
			*LoggingUtils::GetName(MyOwner), *GetName(), *PushbackDirection.ToCompactString(), *HitLocation.ToCompactString(), ToHitLocationDistance / 100, PushbackDistance / 100, *ToHitLocationProjection.ToCompactString());
		UE_VLOG_ARROW(MyOwner, LogPGPawn, Log, HitLocation, HitLocation + ToHitLocationProjection, FColor::Red, TEXT("Clr: ToHitLocationProjection"));

		return {};
	}

	const auto NewPosition = CurrentPosition + PushbackDirection * PushbackDistance;

	return FClearanceLocationResult
	{
		.CurrentPosition = CurrentPosition,
		.UpVector = UpVector,
		.PushbackDirection = PushbackDirection,
		.NewPosition = NewPosition
	};
}

bool UGolfShotClearanceComponent::CheckForNewLocationCollisions(const FClearanceLocationResult& ClearanceLocationResult, FClearanceTraceParams& OutClearanceTraceParams) const
{
	const auto MyOwner = GetOwner();
	check(MyOwner);

	auto World = GetWorld();
	check(World);

	const auto TraceHeight = GetActorHeight();

	// Test if moving to the new location results in a collision
	// TODO: Consider a different trace channel than ECC_VISIBILITY.  StaticObstacle trace is for whether something is considered an obstacle and FlickTrace is whether there is line of sight
	// so really need a new trace channel for this
	// Pushback current position so don't overlap immediately with what we checked before
	// Use trace height since this is the actor height and would be the max distance we could have fallen into a crevice of the object
	const auto TraceOffset = ClearanceLocationResult.UpVector * TraceHeight;
	const auto NewPositionTraceStart = ClearanceLocationResult.CurrentPosition + ClearanceLocationResult.PushbackDirection * TraceHeight + TraceOffset;
	const auto NewPositionTraceEnd = ClearanceLocationResult.NewPosition + TraceOffset;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(MyOwner);

	// Note that this also accounts for pushing player into golf hole as the visibility channel will be a blocking hit for the hole
	bool bTestPasses = !World->LineTraceTestByChannel(NewPositionTraceStart, NewPositionTraceEnd, ECollisionChannel::ECC_Visibility, QueryParams);

	if (!bTestPasses)
	{
#if ENABLE_VISUAL_LOG
		if (FVisualLogger::IsRecording())
		{
			UE_VLOG_ARROW(MyOwner, LogPGPawn, Log, NewPositionTraceStart, NewPositionTraceEnd, FColor::Black, TEXT("Clearance Dir"));

			// Determine what we hit and log it
			FHitResult NewPositionHitResult;
			const bool bHasPositionHitResult = World->LineTraceSingleByChannel(NewPositionHitResult, NewPositionTraceStart, NewPositionTraceEnd, ECollisionChannel::ECC_Visibility, QueryParams);
			UE_VLOG_UELOG(MyOwner, LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - FALSE - Pushback direction: %s to location %s resulted in us penetrating another object: %s -> %s"),
				*LoggingUtils::GetName(MyOwner), *GetName(),
				*ClearanceLocationResult.PushbackDirection.ToCompactString(), *ClearanceLocationResult.NewPosition.ToCompactString(),
				bHasPositionHitResult ? *LoggingUtils::GetName(NewPositionHitResult.GetActor()) : TEXT("None"),
				bHasPositionHitResult ? *LoggingUtils::GetName(NewPositionHitResult.GetComponent()) : TEXT("None")
			);
		}
#endif
		return false;
	}

	const auto TraceHalfHeight = TraceHeight * 0.5f;
	const auto BoundsTraceShape = FCollisionShape::MakeBox(FVector{ TraceHalfHeight });
	const auto NewRotationQuat = ClearanceLocationResult.PushbackDirection.ToOrientationQuat();

	bTestPasses = !World->OverlapAnyTestByChannel(NewPositionTraceEnd, NewRotationQuat, ECollisionChannel::ECC_Visibility, BoundsTraceShape, QueryParams);

#if ENABLE_VISUAL_LOG
	if (FVisualLogger::IsRecording())
	{
		UE_VLOG_ARROW(MyOwner, LogPGPawn, Log, ClearanceLocationResult.CurrentPosition, ClearanceLocationResult.NewPosition, FColor::Yellow, TEXT("Clearance Dir"));

		const auto LogShape = FBox::BuildAABB(FVector::ZeroVector, BoundsTraceShape.GetExtent());
		const FTransform LogTransform{ NewRotationQuat, NewPositionTraceEnd };
		UE_VLOG_OBOX(MyOwner, LogPGPawn, Log, LogShape, LogTransform.ToMatrixNoScale(),
			bTestPasses ? FColor::Green : FColor::Red, TEXT("Clearance New"));
	}
#endif

	// Only output trace params if there was no collision)
	if (bTestPasses)
	{
		OutClearanceTraceParams = FClearanceTraceParams
		{
			.TraceHalfHeight = TraceHalfHeight,
			.NewPositionTraceStart = NewPositionTraceStart,
			.NewPositionTraceEnd = NewPositionTraceEnd
		};
	}
	else
	{
		UE_VLOG_UELOG(MyOwner, LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - FALSE - Pushback direction: %s to location %s resulted in a new collision"),
			*LoggingUtils::GetName(MyOwner), *GetName(),
			*ClearanceLocationResult.PushbackDirection.ToCompactString(), *ClearanceLocationResult.NewPosition.ToCompactString());
	}
	
	return bTestPasses;
}

bool UGolfShotClearanceComponent::CheckNewLocationPlayable(const FClearanceLocationResult& ClearanceLocationResult, const FClearanceTraceParams& ClearanceTraceParams) const
{
	const auto MyOwner = GetOwner();
	check(MyOwner);

	auto World = GetWorld();
	check(World);

	// Make sure that our elevation change isn't too great (example - moving off a counter)
	const auto GroundData = PG::CollisionUtils::GetGroundData(*MyOwner, ClearanceLocationResult.NewPosition);
	if (!GroundData)
	{
		UE_VLOG_UELOG(MyOwner, LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - FALSE - Pushback direction: %s to location %s resulted in not being able to determine ground"),
			*LoggingUtils::GetName(MyOwner), *GetName(),
			*ClearanceLocationResult.PushbackDirection.ToCompactString(), *ClearanceLocationResult.NewPosition.ToCompactString());

		return false;
	}

	// Make sure elevation change not too great - project onto up vector
	check(GroundData);
	const auto ToNewPositionGround = (GroundData->Location - ClearanceLocationResult.CurrentPosition);
	const auto NewPositionGroundProjection = ToNewPositionGround.ProjectOnToNormal(ClearanceLocationResult.UpVector);

	if (const auto DistSq = NewPositionGroundProjection.SizeSquared(); DistSq > FMath::Square(GroundDeltaAllowance))
	{
		UE_VLOG_UELOG(MyOwner, LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - FALSE - Pushback direction: %s to location %s resulted in too great of an elevation change %fm > %fm"),
			*LoggingUtils::GetName(MyOwner), *GetName(),
			*ClearanceLocationResult.PushbackDirection.ToCompactString(), *ClearanceLocationResult.NewPosition.ToCompactString(),
			FMath::Sqrt(DistSq) / 100, GroundDeltaAllowance / 100);

		UE_VLOG_ARROW(MyOwner, LogPGPawn, Log, ClearanceLocationResult.CurrentPosition, GroundData->Location, FColor::Red, TEXT("Clr Elevation"));

		return false;
	}

	const auto ToNewPositionGroundHalfHeight = FMath::Abs(ToNewPositionGround.Z * 0.5f);
	const auto HazardBoundsTraceShape = FCollisionShape::MakeBox(
		FVector{ ClearanceTraceParams.TraceHalfHeight, ClearanceTraceParams.TraceHalfHeight, ToNewPositionGroundHalfHeight + ClearanceTraceParams.TraceHalfHeight }
	);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(MyOwner);

	// Check if overlap a hazard - use the ground position and offset the height so that the overlap check sweeps to the ground
	const auto HazardTraceCenter = ClearanceLocationResult.NewPosition - FVector::ZAxisVector * ToNewPositionGroundHalfHeight;
	bool bTestPasses = !World->OverlapAnyTestByObjectType(HazardTraceCenter, FQuat::Identity, PG::CollisionObjectType::Hazard, HazardBoundsTraceShape, QueryParams);

	if (!bTestPasses)
	{
		UE_VLOG_UELOG(MyOwner, LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - FALSE - Pushback direction: %s to location %s resulted in going in a hazard"),
			*LoggingUtils::GetName(MyOwner), *GetName(),
			*ClearanceLocationResult.PushbackDirection.ToCompactString(), *ClearanceLocationResult.NewPosition.ToCompactString());

#if ENABLE_VISUAL_LOG
		if (FVisualLogger::IsRecording())
		{
			const auto LogShape = FBox::BuildAABB(HazardTraceCenter, HazardBoundsTraceShape.GetExtent());
			UE_VLOG_BOX(MyOwner, LogPGPawn, Log, LogShape, FColor::Red, TEXT("Clr New: Hazard"));
		}
#endif
	}

	return bTestPasses;
}

bool UGolfShotClearanceComponent::CalculateClearanceLocation(const FVector& HitLocation, const FVector& RawPushbackDirection, FVector& OutNewLocation) const
{
	const auto MyOwner = GetOwner();
	check(MyOwner);

	auto World = GetWorld();
	check(World);

	const auto ClearanceLocationResult = CalculateClearanceLocationResult(HitLocation, RawPushbackDirection);
	if (!ClearanceLocationResult)
	{
		return false;
	}

	FClearanceTraceParams ClearanceTraceParams;
	if (!CheckForNewLocationCollisions(*ClearanceLocationResult, ClearanceTraceParams))
	{
		return false;
	}

	if (!CheckNewLocationPlayable(*ClearanceLocationResult, ClearanceTraceParams))
	{
		return false;
	}

	UE_VLOG_UELOG(MyOwner, LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - TRUE - Pushback direction: %s to location %s"),
		*LoggingUtils::GetName(MyOwner), *GetName(),
		*ClearanceLocationResult->PushbackDirection.ToCompactString(), *ClearanceLocationResult->NewPosition.ToCompactString());

	OutNewLocation = ClearanceLocationResult->NewPosition;

	UE_VLOG_LOCATION(MyOwner, LogPGPawn, Verbose, ClearanceLocationResult->CurrentPosition, 10.0f, FColor::Orange, TEXT("Clr: Prev Pos"));
	UE_VLOG_LOCATION(MyOwner, LogPGPawn, Verbose, ClearanceLocationResult->NewPosition, 10.0f, FColor::Yellow, TEXT("Clr: New Pos"));

	return true;
}

