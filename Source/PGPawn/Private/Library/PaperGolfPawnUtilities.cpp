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

		if (FMath::Abs(RawSum) <= MaxRotationAngle)
		{
			*TotalRotationAngle = RawSum;
		}
		else
		{
			// zero out the delta component
			*DeltaRotationAngle = 0;
		}
	}
}

void UPaperGolfPawnUtilities::BlueprintResetPhysicsState(UPrimitiveComponent* PhysicsComponent, const FTransform& RelativeTransform)
{
	if (!PhysicsComponent)
	{
		return;
	}

	PhysicsComponent->SetSimulatePhysics(false);

	// When we simulate physics and the primitive component is not the root component, it becomes detached from its parent
	// In this case we need to manually reattach it to its original parent, move the parent to where it was and then reset any relative transform of the physics component
	// back to the original values (RelativeTransform)
	// FIXME: Putting check for role as a workaround for "phantom pawns" replicated from SetTransform and a concurrency issue with the actor attachments - this messes up the camera
	// Note that ideally we might check for Owner->HasAuthority() as the components attachments should only be changed on the server (this includes SetSimulatePhysics(true/false) as setting to true detaches
	// the static mesh from the actor's root component. But using Owner->HasAuthority() results in the client sometimes not seeing the rotation of their paper football
	if (auto Owner = PhysicsComponent->GetOwner(); Owner && Owner->GetLocalRole() != ENetRole::ROLE_SimulatedProxy) // Owner->HasAuthority())
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

	RootComponent->SetWorldLocation(PhysicsComponent->GetComponentLocation() - RelativeTransform.GetLocation());
	RootComponent->SetWorldRotation(PhysicsComponent->GetComponentRotation() - RelativeTransform.GetRotation().Rotator());

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
