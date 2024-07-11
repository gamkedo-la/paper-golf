// Copyright Game Salutes. All Rights Reserved.


#include "Library/PaperGolfPawnUtilities.h"

#include "Components/PrimitiveComponent.h"

// Draw point
#include "Components/LineBatchComponent.h"

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

void UPaperGolfPawnUtilities::ResetPhysicsState(UPrimitiveComponent* PhysicsComponent)
{
	if (!PhysicsComponent)
	{
		return;
	}

	//PhysicsComponent->RecreatePhysicsState();
	PhysicsComponent->SetSimulatePhysics(false);
	//PhysicsComponent->ResetRelativeTransform();
	//if (auto Owner = PhysicsComponent->GetOwner(); Owner)
	//{
	////	Owner->GetRootComponent()->ResetRelativeTransform();
	//	Owner->SetActorLocation(PhysicsComponent->GetComponentLocation());
	//}
	////	Owner->SetActorRotation(PhysicsComponent->GetComponentRotation());
	////	//Owner->SetActorRelativeTransform(FTransform::Identity, false, nullptr, ETeleportType::ResetPhysics);
	////	//Owner->SetActorRelativeRotation(FRotator::ZeroRotator);
	////}
	//auto Parent = PhysicsComponent->GetAttachParent();
	//if (auto Parent = PhysicsComponent->GetAttachParent(); Parent)
	//{
	//	PhysicsComponent->DetachFromParent();
	//	PhysicsComponent->AttachToComponent(Parent, FAttachmentTransformRules::KeepWorldTransform);
	//}
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
