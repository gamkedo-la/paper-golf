// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Library/PaperGolfPawnUtilities.h"

#include "Components/PrimitiveComponent.h"

// Draw point
#include "Components/LineBatchComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGPawnLogging.h"

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
}

void UPaperGolfPawnUtilities::ClampDeltaRotation(const FRotator& MaxRotationExtent, FRotator& DeltaRotation, FRotator& TotalRotation)
{
	const auto MaxRotatorAngles = ToConstRotatorArray(MaxRotationExtent);

	auto DeltaRotationAngles = ToRotatorArray(DeltaRotation);
	auto TotalRotationAngles = ToRotatorArray(TotalRotation);

	for (int32 i = 0; i < 3; ++i)
	{
		auto DeltaRotationAngle = DeltaRotationAngles[i];
		auto TotalRotationAngle = TotalRotationAngles[i];
		const auto MaxRotationAngle = MaxRotatorAngles[i];

		const auto RawSum = *DeltaRotationAngle + *TotalRotationAngle;
		const auto RawSumAbs = FMath::Abs(RawSum);
		const auto MaxRotationDiff = MaxRotationAngle - RawSumAbs;

		if (MaxRotationDiff >= 0)
		{
			*TotalRotationAngle = RawSum;
		}
		else if (MaxRotationAngle >= AxisFullRotation[i] && AxisSupportsFullRotation[i])
		{
			// Wrap around to the other side of the rotation axis and preserve the delta rotation
			*TotalRotationAngle = FMath::Fmod(RawSum, MaxRotationAngle);
		}
		else
		{
			// Add in value up to the clamping to max rotation

			const auto RawSumSign = FMath::Sign(RawSum);
			*TotalRotationAngle = RawSumSign * MaxRotationAngle;
			// Adding here since the difference will always be negative and want to reduce the delta rotation by the overshoot
			*DeltaRotationAngle = *DeltaRotationAngle + RawSumSign * MaxRotationDiff;
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
