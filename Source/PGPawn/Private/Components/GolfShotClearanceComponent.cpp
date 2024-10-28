// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/GolfShotClearanceComponent.h"


#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"

#include "Utils/VisualLoggerUtils.h"

#include "Utils/CollisionUtils.h"

#include "Components/StaticMeshComponent.h"

#include "PGPawnLogging.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfShotClearanceComponent)

namespace
{
	const FVector DefaultPosition{ 1e9, 1e9, 1e9 };
	constexpr float TraceOffsetFactor = 1.5f;
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

	if (!ShouldAdjustPosition())
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - FALSE - ShouldAdjustPosition returned false"),
			*LoggingUtils::GetName(GetOwner()), *GetName());

		bFirstShot = false;
		if (auto MyOwner = GetOwner(); MyOwner)
		{
			LastPosition = MyOwner->GetActorLocation();
		}

		return false;
	}

	auto World = GetWorld();
	if (!ensureAlways(World))
	{
		return false;
	}

	if (!IsClearanceNeeded())
	{
		return false;
	}

	FVector NewPosition;
	if (!CalculateClearanceLocation(NewPosition))
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
	bFirstShot = true;
}

bool UGolfShotClearanceComponent::ShouldAdjustPosition() const
{
	const auto MyOwner = GetOwner();
	if (!ensureAlways(MyOwner))
	{
		return false;
	}

	// Don't adjust first shot
	if (bFirstShot)
	{
		UE_VLOG_UELOG(MyOwner, LogPGPawn, Verbose, TEXT("%s-%s: ShouldAdjustPosition - FALSE - First shot"),
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

bool UGolfShotClearanceComponent::IsClearanceNeeded() const
{
	const auto MyOwner = GetOwner();
	check(MyOwner);

	auto World = GetWorld();
	check(World);

	const auto& CurrentPosition = MyOwner->GetActorLocation();
	const auto& ForwardDirection = MyOwner->GetActorForwardVector();
	const auto& UpVector = MyOwner->GetActorUpVector();

	const auto TraceHeight = GetActorHeight();

	// Rotator from ActorUp
	const auto RotationQuat = ForwardDirection.ToOrientationQuat();

	// Offset so overlap is at the top of where we want to adjust
	const auto TraceOffset = UpVector * TraceHeight * TraceOffsetFactor;
	const auto TracePosition = CurrentPosition + TraceOffset;
	const auto TraceShape = FCollisionShape::MakeBox(FVector{ AdjustmentDistance, AdjustmentDistance, TraceHeight });

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

	if (!bResult)
	{
		UE_VLOG_UELOG(MyOwner, LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - FALSE - No overlap detected"),
			*LoggingUtils::GetName(MyOwner), *GetName());
	}

	return bResult;
}

bool UGolfShotClearanceComponent::CalculateClearanceLocation(FVector& OutNewLocation) const
{
	const auto MyOwner = GetOwner();
	check(MyOwner);

	auto World = GetWorld();
	check(World);

	const auto& CurrentPosition = MyOwner->GetActorLocation();
	const auto& UpVector = MyOwner->GetActorUpVector();
	const auto TraceHeight = GetActorHeight();

	const auto RawPushbackDirection = (LastPosition - CurrentPosition).GetSafeNormal();
	// Project along plane with up vector
	const auto PushbackDirection = FVector::VectorPlaneProject(RawPushbackDirection, UpVector).GetSafeNormal();

	// Push back toward the last position and then retest
	// TODO: May want to consider a hit normal as a fallback as need to make sure we don't push player through the floor in a way that snap to ground won't correct
	const auto NewPosition = CurrentPosition + PushbackDirection * AdjustmentDistance;

	// Test if moving to the new location results in a collision
	// TODO: Consider a different trace channel than ECC_VISIBILITY.  StaticObstacle trace is for whether something is considered an obstacle and FlickTrace is whether there is line of sight
	// so really need a new trace channel for this
	// Pushback current position so don't overlap immediately with what we checked before
	// Use trace height since this is the actor height and would be the max distance we could have fallen into a crevice of the object
	const auto NewPositionTraceStart = CurrentPosition + PushbackDirection * TraceHeight;
	bool bResult = World->LineTraceTestByChannel(NewPositionTraceStart, NewPosition, ECollisionChannel::ECC_Visibility);

#if ENABLE_VISUAL_LOG
	if (bResult && FVisualLogger::IsRecording())
	{
		UE_VLOG_ARROW(MyOwner, LogPGPawn, Log, CurrentPosition, NewPosition, FColor::Red, TEXT("Clearance Dir"));
		UE_VLOG_UELOG(MyOwner, LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - FALSE - Pushback direction: %s to location %s resulted in us penetrating another object"),
			*LoggingUtils::GetName(MyOwner), *GetName(), *PushbackDirection.ToCompactString(), *NewPosition.ToCompactString());

		return false;
	}
#endif

	const auto BoundsTraceShape = FCollisionShape::MakeBox(FVector{ TraceHeight * 0.5f });
	const auto NewRotationQuat = PushbackDirection.ToOrientationQuat();
	const auto TraceOffset = UpVector * TraceHeight * TraceOffsetFactor;

	bResult = World->OverlapAnyTestByChannel(NewPosition + TraceOffset, NewRotationQuat, ECollisionChannel::ECC_Visibility, BoundsTraceShape);

#if ENABLE_VISUAL_LOG
	if (FVisualLogger::IsRecording())
	{
		UE_VLOG_ARROW(MyOwner, LogPGPawn, Log, CurrentPosition, NewPosition, FColor::Yellow, TEXT("Clearance Dir"));
		const auto LogShape = FBox::BuildAABB(FVector::ZeroVector, BoundsTraceShape.GetExtent());

		const FTransform LogTransform{ NewRotationQuat, NewPosition + TraceOffset };
		UE_VLOG_OBOX(MyOwner, LogPGPawn, Log, LogShape, LogTransform.ToMatrixNoScale(),
			bResult ? FColor::Red : FColor::Green, TEXT("Clearance New"));
	}
#endif

	if (bResult)
	{
		UE_VLOG_UELOG(MyOwner, LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - FALSE - Pushback direction: %s to location %s resulted in a new collision"),
			*LoggingUtils::GetName(MyOwner), *GetName(), *PushbackDirection.ToCompactString(), *NewPosition.ToCompactString());

		return false;
	}

	UE_VLOG_UELOG(MyOwner, LogPGPawn, Log, TEXT("%s-%s: AdjustPositionForClearance - TRUE - Pushback direction: %s to location %s"),
		*LoggingUtils::GetName(MyOwner), *GetName(), *PushbackDirection.ToCompactString(), *NewPosition.ToCompactString());

	OutNewLocation = NewPosition;
	return true;
}
