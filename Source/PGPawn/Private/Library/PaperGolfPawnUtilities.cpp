// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Library/PaperGolfPawnUtilities.h"

#include "Components/PrimitiveComponent.h"

// Draw point
#include "Components/LineBatchComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGPawnLogging.h"

#include "Pawn/PaperGolfPawn.h"

#include "Utils/CollisionUtils.h"
#include "Utils/PGMathUtils.h"

#include <array>

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfPawnUtilities)

namespace
{
	using RotatorArray = std::array<double*, 3>;
	using ConstRotatorArray = std::array<double, 3>;

	RotatorArray ToRotatorArray(FRotator& Rotator);
	ConstRotatorArray ToConstRotatorArray(const FRotator& Rotator);

	// Maximum possible total range of each rotation axis.  Pitch can only go from -90 to 90 so total is 180 but the rest are free to go full 360.
	constexpr const std::array<bool, 3> AxisSupportsFullRotation{ true, false, true };
	constexpr const ConstRotatorArray AxisFullRotation{ 360.0, 180.0, 360.0 };

	const UObject* GetVisualLoggerOwner(const UObject* WorldContextObject);
}

// Define here to avoid C4686 since declaring a template in the return type of a function signature does not instantiate it
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4686?view=msvc-170
// https://developercommunity.visualstudio.com/t/msvc-fails-to-instantiate-stdarray-and-erroneously/1150191
namespace
{
	RotatorArray ToRotatorArray(FRotator& Rotator)
	{
		return
		{
			&Rotator.Roll,
			&Rotator.Pitch,
			&Rotator.Yaw
		};
	}

	ConstRotatorArray ToConstRotatorArray(const FRotator& Rotator)
	{
		return
		{
			Rotator.Roll,
			Rotator.Pitch,
			Rotator.Yaw
		};
	}
	
	const UObject* GetVisualLoggerOwner(const UObject* WorldContextObject)
	{
		if (auto ActorComponent = Cast<UActorComponent>(WorldContextObject); ActorComponent)
		{
			return ActorComponent->GetOwner();
		}
		
		return WorldContextObject;
	}
}

void UPaperGolfPawnUtilities::ClampDeltaRotation(const FRotator& MinRotationExtent, const FRotator& MaxRotationExtent, FRotator& DeltaRotation, FRotator& TotalRotation)
{
	const auto MinRotatorAngles = ToConstRotatorArray(MinRotationExtent);
	const auto MaxRotatorAngles = ToConstRotatorArray(MaxRotationExtent);

	auto DeltaRotationAngles = ToRotatorArray(DeltaRotation);
	auto TotalRotationAngles = ToRotatorArray(TotalRotation);

	for (int32 i = 0; i < 3; ++i)
	{
		auto DeltaRotationAngle = DeltaRotationAngles[i];
		auto TotalRotationAngle = TotalRotationAngles[i];

		const auto MinRotationAngle = MinRotatorAngles[i];
		const auto MaxRotationAngle = MaxRotatorAngles[i];

		const auto RawSum = *DeltaRotationAngle + *TotalRotationAngle;
		const auto ClampedSum = FMath::Clamp(RawSum, MinRotationAngle, MaxRotationAngle);

		// If the total rotation is within the min and max rotation extents, then we can just add the delta rotation
		if (FMath::IsNearlyEqual(RawSum, ClampedSum, KINDA_SMALL_NUMBER))
		{
			*TotalRotationAngle = ClampedSum;
		}
		else if (MaxRotationAngle >= AxisFullRotation[i] && AxisSupportsFullRotation[i])
		{
			// Wrap around to the other side of the rotation axis and preserve the delta rotation
			*TotalRotationAngle = FMath::Fmod(RawSum, MaxRotationAngle);
		}
		else
		{
			// Add in value up to the clamping to min/max rotation
			*TotalRotationAngle = ClampedSum;
			// Adding here since the difference will always be negative and want to reduce the delta rotation by the overshoot
			*DeltaRotationAngle = *DeltaRotationAngle + (ClampedSum - RawSum);
		}
	}
}

void UPaperGolfPawnUtilities::BlueprintResetPhysicsState(UPrimitiveComponent* PhysicsComponent, const FTransform& RelativeTransform)
{
	if (!PhysicsComponent)
	{
		return;
	}

	if(auto Owner = PhysicsComponent->GetOwner(); Owner && Owner->GetLocalRole() != ROLE_SimulatedProxy)
	{
		// Need to disable physics simulation on clients since client movement happens immediately and cannot apply rotation if simulating physics
		PhysicsComponent->SetSimulatePhysics(false);
	}

	// When we simulate physics and the primitive component is not the root component, it becomes detached from its parent
	// In this case we need to manually reattach it to its original parent, move the parent to where it was and then reset any relative transform of the physics component
	// back to the original values (RelativeTransform)
	// Check for Owner->HasAuthority() as the components attachments should only be changed on the server (this includes SetSimulatePhysics(true/false) as setting to true detaches
	// the static mesh from the actor's root component.
	if (auto Owner = PhysicsComponent->GetOwner(); Owner && Owner->HasAuthority())
	{
		ReattachPhysicsComponent(PhysicsComponent, RelativeTransform);
	}
}

void UPaperGolfPawnUtilities::ReattachPhysicsComponent(UPrimitiveComponent* PhysicsComponent, const FTransform& RelativeTransform, bool bForceUpdate )
{
	auto Owner = PhysicsComponent->GetOwner();
	if (!Owner)
	{
		return;
	}

	auto RootComponent = Owner->GetRootComponent();
	if (!RootComponent)
	{
		return;
	}

	if(!bForceUpdate && RootComponent == PhysicsComponent->GetAttachmentRoot())
	{
		return;
	}

	UE_VLOG_UELOG(Owner, LogPGPawn, Log, TEXT("%s: ResetPhysicsState - Update Transforms - PhysicsComponent=%s, RelativeTransform=%s; RootComponentTransform=%s; PhysicsComponentTransform=%s"),
		*Owner->GetName(), *PhysicsComponent->GetName(), *RelativeTransform.ToString(),
		*RootComponent->GetComponentTransform().ToString(), *PhysicsComponent->GetComponentTransform().ToString());

	RootComponent->SetWorldLocation(PhysicsComponent->GetComponentLocation()); // -RelativeTransform.GetLocation());
	RootComponent->SetWorldRotation(PhysicsComponent->GetComponentRotation()); // RelativeTransform.GetRotation().Rotator());

	PhysicsComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	PhysicsComponent->SetRelativeTransform(RelativeTransform);

	// force a network update if we are on the server
	if (Owner->HasAuthority())
	{
		Owner->ForceNetUpdate();
	}
}

void UPaperGolfPawnUtilities::DrawPoint(const UObject* WorldContextObject, const FVector& Position, const FLinearColor& Color, float PointSize)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	// See https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Engine/ESceneDepthPriorityGroup/
	// https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Components/ULineBatchComponent/
	// https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Engine/UWorld/
	World->ForegroundLineBatcher->DrawPoint(Position, Color, PointSize, SDPG_World, 0.0f);
}

void UPaperGolfPawnUtilities::DrawSphere(const UObject* WorldContextObject, const FVector& Position, const FLinearColor& Color, float Radius, int32 NumSegments)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	World->ForegroundLineBatcher->DrawSphere(Position, Radius, NumSegments, Color, 0.0f, SDPG_World, 1.0f);
}

void UPaperGolfPawnUtilities::DrawBox(const UObject* WorldContextObject, const FVector& Position, const FLinearColor& Color, const FVector& Extent)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	const auto Box = FBox::BuildAABB(Position, Extent);

	World->ForegroundLineBatcher->DrawSolidBox(Box, FTransform::Identity, Color.ToFColor(true), 0.0f, SDPG_World);
}

bool UPaperGolfPawnUtilities::TraceShotAngle(const UObject* WorldContextObject, const APaperGolfPawn* PlayerPawn, const FVector& TraceStart, const FVector& FlickDirection, float FlickSpeed, float FlickAngleDegrees, float MinTraceDistance)
{
	if (!ensure(WorldContextObject))
	{
		return false;
	}

	if (!ensure(PlayerPawn))
	{
		return false;
	}

	auto World = WorldContextObject->GetWorld();
	check(World);

	const auto TraceAngle = FlickAngleDegrees > 0 ? FlickAngleDegrees : 0;
	const auto MaxHeight = TraceAngle > 0 ? PG::MathUtils::GetMaxProjectileHeight(WorldContextObject, FlickAngleDegrees, FlickSpeed) : 0;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PlayerPawn);

	// Pitch up or down based on the FlickAngleDegrees
	const auto PitchedFlickDirection = FlickDirection.RotateAngleAxis(TraceAngle, -PlayerPawn->GetActorRightVector());

	// Need the horizontal distance to be MinTraceLength and the Z to be MaxHeigth
	// Pythagorean theorem: a^2 + b^2 = c^2
	const auto TraceLength = FMath::Sqrt(FMath::Square(MinTraceDistance) + FMath::Square(MaxHeight));
	const FVector TraceEnd = TraceStart + PitchedFlickDirection * TraceLength;

	// Try line trace directly to trace length as an approximation
	bool bPass = !World->LineTraceTestByChannel(TraceStart, TraceEnd, PG::CollisionChannel::FlickTraceType, QueryParams);

	UE_VLOG_ARROW(GetVisualLoggerOwner(WorldContextObject), LogPGPawn, Log, TraceStart, TraceEnd, bPass ? FColor::Green : FColor::Red, TEXT("Trace %.1f"), FlickAngleDegrees);
	UE_VLOG_UELOG(GetVisualLoggerOwner(WorldContextObject), LogPGPawn, Verbose, TEXT("%s-%s: TraceShotAngle - TraceStart=%s; FlickDirection=%s; PitchedFlickDirection=%s; FlickAngle=%.1f; FlickSpeed=%.1f; MaxHeight=%.1f; bPass=%s"),
		*LoggingUtils::GetName(GetVisualLoggerOwner(WorldContextObject)), *WorldContextObject->GetName(),
		*TraceStart.ToCompactString(), *FlickDirection.ToCompactString(), *PitchedFlickDirection.ToCompactString(), FlickAngleDegrees, FlickSpeed, MaxHeight, LoggingUtils::GetBoolString(bPass));

	return bPass;
}

bool UPaperGolfPawnUtilities::TraceCurrentShotWithParameters(const UObject* WorldContextObject, const APaperGolfPawn* PlayerPawn, const FFlickParams& FlickParams, float FlickAngleDegrees, float MinTraceDistance)
{
	if (!ensure(WorldContextObject))
	{
		return false;
	}

	if (!ensure(PlayerPawn))
	{
		return false;
	}

	const auto TraceStart = PlayerPawn->GetFlickLocation(FlickParams.LocalZOffset);
	const auto& FlickDirection = PlayerPawn->GetFlickDirection();
	const auto FlickSpeed = [&]()
	{
		if (FMath::IsNearlyEqual(FlickParams.PowerFraction, 1.0f, KINDA_SMALL_NUMBER) && 
			FMath::IsNearlyZero(FlickParams.Accuracy, KINDA_SMALL_NUMBER))
		{
			return PlayerPawn->GetFlickMaxSpeed(FlickParams.ShotType);
		}
		return PlayerPawn->GetFlickSpeed(FlickParams.ShotType, FlickParams.Accuracy, FlickParams.PowerFraction);
	}();

	return UPaperGolfPawnUtilities::TraceShotAngle(
		WorldContextObject, PlayerPawn, TraceStart, FlickDirection, FlickSpeed, FlickAngleDegrees);
}

bool UPaperGolfPawnUtilities::ShotWillReachDestination(const UObject* WorldContextObject, const APaperGolfPawn* PlayerPawn, const FFlickParams& FlickParams, const AActor* FocusActorOverride)
{
	if (!ensure(WorldContextObject))
	{
		return false;
	}

	if (!ensure(PlayerPawn))
	{
		return false;
	}

	const auto World = WorldContextObject->GetWorld();
	if (!ensure(World))
	{
		return false;
	}

	const auto FocusActor = FocusActorOverride ? FocusActorOverride : PlayerPawn->GetFocusActor();
	if (!FocusActor)
	{
		UE_VLOG_UELOG(GetVisualLoggerOwner(WorldContextObject), LogPGPawn, Warning, TEXT("ShotWillReachDestination - TRUE - FocusActorOverride is NULL and default FocusActor is NULL"));
		return false;
	}

	// See UGolfAIShotComponent::CalculateInitialShotParams for details about the calculation
	// Assumes a 45 degree shot angle for calculations
	const auto FlickLocation = PlayerPawn->GetFlickLocation(FlickParams.LocalZOffset);
	const auto FlickMaxForce = PlayerPawn->GetFlickMaxForce(FlickParams.ShotType);
	const auto FlickMaxSpeed = FlickMaxForce / PlayerPawn->GetMass();

	const auto& FocusActorLocation = FocusActor->GetActorLocation();

	const auto PositionDelta = FocusActorLocation - FlickLocation;
	const auto HorizontalDistance = PositionDelta.Size2D();
	const auto VerticalDistance = PositionDelta.Z;
	const auto DistanceSum = HorizontalDistance + VerticalDistance;

	if (DistanceSum <= 0)
	{
		UE_VLOG_UELOG(GetVisualLoggerOwner(WorldContextObject), LogPGPawn, Log,
			TEXT("ShotWillReachDestination - TRUE - DistanceSum is less than or equal to 0: FlickParams=%s; FocusActor=%s; HorizontalDistance=%.1fm; VerticalDistance=%.1fm"),
			*FlickParams.ToString(), *LoggingUtils::GetName(FocusActor),
			HorizontalDistance, VerticalDistance);
		return true;
	}

	// Check if calculated speed is <= FlickMaxSpeed
	const auto Gravity = FMath::Abs(World->GetGravityZ());
	const auto Speed = HorizontalDistance * FMath::Sqrt(Gravity) / FMath::Sqrt(DistanceSum);

	const bool bWillReachDestination = Speed <= FlickMaxSpeed;

	UE_VLOG_UELOG(GetVisualLoggerOwner(WorldContextObject), LogPGPawn, Log,
		TEXT("ShotWillReachDestination - %s - FlickParams=%s; FocusActor=%s; HorizontalDistance=%.1fm; VerticalDistance=%.1fm; DistanceSum=%.1fm; Speed=%.1fm; FlickMaxSpeed=%.1fm"),
		LoggingUtils::GetBoolString(bWillReachDestination),
		*FlickParams.ToString(), *LoggingUtils::GetName(FocusActor),
		HorizontalDistance, VerticalDistance, DistanceSum, Speed, FlickMaxSpeed);

	return bWillReachDestination;
}
