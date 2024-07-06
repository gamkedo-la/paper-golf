// Copyright Game Salutes. All Rights Reserved.


#include "Pawn/PaperGolfPawn.h"

#include "PaperGolfTypes.h"

#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h" 

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

#include "Kismet/KismetMathLibrary.h"

#include "Build/BuildUtilities.h"

#include "Library/PaperGolfPawnUtilities.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "Utils/CollisionUtils.h"

#include "Utils/VisualLoggerUtils.h"
#include "PGPawnLogging.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfPawn)

APaperGolfPawn::APaperGolfPawn()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APaperGolfPawn::DebugDrawCenterOfMass(float DrawTime)
{
#if !UE_BUILD_SHIPPING

	auto PrimitiveComponent = FindComponentByClass<UPrimitiveComponent>();
	if (!PrimitiveComponent)
	{
		return;
	}

	if (FBodyInstance* Body = PrimitiveComponent->GetBodyInstance(); Body)
	{
		DrawDebugSphere(GetWorld(), Body->GetCOMPosition(), 10.f, 16, FColor::Yellow, false, DrawTime);
	}

#endif
}

bool APaperGolfPawn::IsStuckInPerpetualMotion() const
{
	if (States.Num() < NumSamples)
	{
		return false;
	}

	const int32 OldestIndex = (StateIndex + 1) % NumSamples;

	// Check the delta against threshold
	const auto Delta = States[StateIndex].Position - States[OldestIndex].Position;

	return Delta.Size() < MinDistanceThreshold;
}

void APaperGolfPawn::SetFocusActor(AActor* Focus)
{
	FocusActor = Focus;

	ResetCameraForShotSetup();
}

void APaperGolfPawn::SnapToGround()
{
	ResetRotation();

	auto World = GetWorld();
	
	if (!ensure(World))
	{
		return;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const auto& TraceStart = GetActorLocation();

	FHitResult HitResult;
	
	const auto& Bounds = _PaperGolfMesh ? PG::CollisionUtils::GetAABB(*_PaperGolfMesh) : PG::CollisionUtils::GetAABB(*this);
	const auto& ActorUpVector = GetActorUpVector();

	const auto StartLocation = TraceStart + Bounds.GetExtent().Z * ActorUpVector;
	const auto EndLocation = TraceStart - 2000 * ActorUpVector;

	UE_VLOG_SEGMENT_THICK(this, LogPGPawn, Log, StartLocation, EndLocation, FColor::Yellow, 10.0, TEXT("GroundTrace"));

	// TODO: Create ground trace channel
	if (!World->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_Visibility,
		QueryParams))
	{
		UE_VLOG_UELOG(this,
			LogPGPawn,
			Warning,
			TEXT("%s: Could not determine ground!"),
			*GetName()
		);

		return;
	}

	const auto& Location = HitResult.Location;

	SetActorLocation(Location);

	UE_VLOG_LOCATION(this,
		LogPGPawn,
		Log,
		Location,
		20.0,
		FColor::Green,
		TEXT("SnapToGround")
	);
}

void APaperGolfPawn::ResetRotation()
{
	SetActorRotation(InitialRotation);

	check(_PaperGolfMesh);

	_PaperGolfMesh->SetWorldRotation(InitialRotation);

	ResetCameraForShotSetup();
}

FVector APaperGolfPawn::GetFlickDirection() const
{
	check(_PaperGolfMesh);

	const auto& RelativeRotation = _PaperGolfMesh->GetRelativeRotation();
	const auto& InitialRotationInverse = InitialRotation.GetInverse();

	// Flick direction is a combination of the player rotation and the initial rotation of the base mesh 
	return UKismetMathLibrary::ComposeRotators(RelativeRotation, InitialRotationInverse).Vector();
}

FVector APaperGolfPawn::GetFlickLocation(float LocationZ, float Accuracy, float Power) const
{
	check(_FlickReference);

	const auto RawLocationAccuracyOffset = (LocationZ <= 0 ? 1.0 : -1.0) * (1 - FMath::Abs(Accuracy)) * LocationAccuracyMultiplier;
	const auto AccuracyAdjustedLocationZ = LocationZ * (1 + RawLocationAccuracyOffset);
	const auto& FlickTransform = _FlickReference->GetComponentTransform();

	return FlickTransform.TransformPosition(
		FVector{ 0.0, 0.0, AccuracyAdjustedLocationZ });
}

FVector APaperGolfPawn::GetFlickForce(EShotType ShotType, float Accuracy, float Power) const
{
	const auto& PerfectShotFlickDirection = GetFlickDirection();

	// Apply accuracy offsets
	const auto AccuracyOffset = FMath::Sign(Accuracy) * FMath::Pow(Accuracy, PowerAccuracyExp);

	// Transform only the yaw
	const auto FlickDirectionDeviation = GetActorTransform().TransformVectorNoScale(
		FVector{ 0.0, AccuracyOffset, 0.0 });

	const auto FlickDirection = (PerfectShotFlickDirection + FlickDirectionDeviation).GetSafeNormal();

	const auto FlickMaxPower = GetFlickMaxForce(ShotType);

	// Dampen initial power by accuracy
	const auto RawDampenedPowerFactor = FMath::Pow(1 - FMath::Abs(Accuracy), PowerAccuracyDampenExp);
	const auto FlickPowerFactor = FMath::Clamp(RawDampenedPowerFactor, MinPowerMultiplier, 1.0);

	return FlickDirection * FlickMaxPower * Power * FlickPowerFactor;
}


bool APaperGolfPawn::IsAtRest() const
{
	if (GetLinearVelocity().SquaredLength() <= RestLinearVelocitySquaredMax &&
		GetAngularVelocity().SquaredLength() <= RestAngularVelocityRadsSquaredMax)
	{
		return true;
	}

	// Make sure we aren't stuck in a perpetual motion state
	return IsStuckInPerpetualMotion();
}

void APaperGolfPawn::SetUpForNextShot()
{
	check(_PaperGolfMesh);

	UPaperGolfPawnUtilities::ResetPhysicsState(_PaperGolfMesh);
}

void APaperGolfPawn::Flick(const FFlickParams& FlickParams)
{
	SetCameraForFlick();

	DoFlick(FlickParams);

	if (HasAuthority() && IsLocallyControlled())
	{
		// Broadcast to other clients
		MulticastFlick(ToNetworkParams(FlickParams));
	}
	else if (!HasAuthority())
	{
		// Send to server
		ServerFlick(ToNetworkParams(FlickParams));
	}
}

FNetworkFlickParams APaperGolfPawn::ToNetworkParams(const FFlickParams& Params) const
{
	return FNetworkFlickParams
	{
		.FlickParams = Params,
		.Rotation = GetActorRotation()
	};
}

void APaperGolfPawn::DoFlick(const FFlickParams& FlickParams)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log,
		TEXT("%s: DoFlick - ShotType=%s; LocalZOffset=%f; PowerFraction=%f; Accuracy=%f"),
		*GetName(), *LoggingUtils::GetName(FlickParams.ShotType), FlickParams.LocalZOffset, FlickParams.PowerFraction, FlickParams.Accuracy
	);

	check(_PaperGolfMesh);

	// Turn off physics at first so can move the actor
	_PaperGolfMesh->SetSimulatePhysics(true);
	_PaperGolfMesh->AddImpulseAtLocation(
		GetFlickForce(FlickParams.ShotType, FlickParams.Accuracy, FlickParams.PowerFraction),
		GetFlickLocation(FlickParams.LocalZOffset, FlickParams.Accuracy, FlickParams.PowerFraction)
	);

	_PaperGolfMesh->SetEnableGravity(true);
}

void APaperGolfPawn::DoNetworkFlick(const FNetworkFlickParams& Params)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log,
		TEXT("%s: DoNetworkFlick - Rotation=%s; LocalZOffset=%f; PowerFraction=%f; Accuracy=%f"),
		*GetName(), *Params.Rotation.ToCompactString(), Params.FlickParams.LocalZOffset, Params.FlickParams.PowerFraction, Params.FlickParams.Accuracy
	);

	check(_PaperGolfMesh);
	_PaperGolfMesh->SetWorldRotation(Params.Rotation);

	DoFlick(Params.FlickParams);

}

void APaperGolfPawn::MulticastFlick_Implementation(const FNetworkFlickParams& Params)
{
	if (HasAuthority())
	{
		return;
	}

	UE_VLOG_UELOG(this, LogPGPawn, Log,
		TEXT("%s: MulticastFlick_Implementation - Rotation=%s; LocalZOffset=%f; PowerFraction=%f; Accuracy=%f"),
		*GetName(), *Params.Rotation.ToCompactString(), Params.FlickParams.LocalZOffset, Params.FlickParams.PowerFraction, Params.FlickParams.Accuracy
	);

	DoNetworkFlick(Params);
}

void APaperGolfPawn::ServerFlick_Implementation(const FNetworkFlickParams& Params)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log,
		TEXT("%s: ServerFlick_Implementation - Rotation=%s; LocalZOffset=%f; PowerFraction=%f; Accuracy=%f"),
		*GetName(), *Params.Rotation.ToCompactString(), Params.FlickParams.LocalZOffset, Params.FlickParams.PowerFraction, Params.FlickParams.Accuracy
	);

	DoNetworkFlick(Params);
}

float APaperGolfPawn::ClampFlickZ(float OriginalZOffset, float DeltaZ) const
{
	// TODO: Do a sweep test from OriginalZOffset + DeltaZ back to OriginalZOfset with GetFlickLocation
	// If there is no intersection then return OriginalZOffset; otherwise return the impact point
	auto World = GetWorld();

	const auto ProposedZOffset = OriginalZOffset + DeltaZ;

	if (!ensure(World))
	{
		return ProposedZOffset;
	}

	FHitResult HitResult;

	// Usually we want to trace from the proposed location to the original location 
	// but if DeltaZ < 0 then we want to trace from the original location to the new location 
	const auto& StartLocation = GetFlickLocation(ProposedZOffset);
	const auto& EndLocation = GetFlickLocation(OriginalZOffset);

	FCollisionObjectQueryParams QueryObjectParams;
	QueryObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_Pawn);

	FCollisionQueryParams Params;
	Params.bReturnFaceIndex = false;
	Params.bReturnPhysicalMaterial = false;

	const auto& FlickReferenceRotation = _FlickReference->GetComponentTransform().GetRotation();

	if (!World->SweepSingleByObjectType(
		HitResult,
		StartLocation,
		EndLocation,
		FlickReferenceRotation,
		QueryObjectParams,
		FCollisionShape::MakeSphere(FlickOffsetZTraceSize),
		Params
	))
	{
		UE_VLOG_LOCATION(this,
			LogPGPawn,
			Log,
			EndLocation,
			FlickOffsetZTraceSize,
			FColor::Orange,
			TEXT("ZTraceS")
		);

		UE_VLOG_LOCATION(this,
			LogPGPawn,
			Log,
			StartLocation,
			FlickOffsetZTraceSize,
			FColor::Orange,
			TEXT("ZTraceE")
		);

		UE_VLOG_UELOG(this, LogPGPawn, Log,
			TEXT("%s: ClampFlickZ - Intersection FALSE between %s and %s with Rot=%s; OriginalZOffset=%f; DeltaZ=%f -> Returning %f"),
			*GetName(), *StartLocation.ToCompactString(), *EndLocation.ToCompactString(),
			*FlickReferenceRotation.ToString(), OriginalZOffset, DeltaZ, OriginalZOffset
		);

		return OriginalZOffset;
	}

	const auto& IntersectionPoint = HitResult.ImpactPoint;

	// Need to translate IntersectionPoint back to an offsetZ
	const auto& LocalIntersectionPoint = _FlickReference->GetComponentTransform().InverseTransformPosition(IntersectionPoint);

	UE_VLOG_LOCATION(this,
		LogPGPawn,
		VeryVerbose,
		EndLocation,
		FlickOffsetZTraceSize,
		FColor::Blue,
		TEXT("ZTraceS")
	);

	UE_VLOG_LOCATION(this,
		LogPGPawn,
		VeryVerbose,
		StartLocation,
		FlickOffsetZTraceSize,
		FColor::Blue,
		TEXT("ZTraceE")
	);

	UE_VLOG_LOCATION(this,
		LogPGPawn,
		VeryVerbose,
		IntersectionPoint,
		FlickOffsetZTraceSize,
		FColor::Green,
		TEXT("ZTraceI")
	);

	const auto NewZOffset = LocalIntersectionPoint.Z;

	UE_VLOG_UELOG(this, LogPGPawn, Verbose,
		TEXT("%s: ClampFlickZ - Intersection TRUE between %s and %s at %s with Rot=%s; LocalIntersectionPoint=%s; OriginalZOffset=%f; DeltaZ=%f -> Returning %f"),
		*GetName(), *EndLocation.ToCompactString(), *StartLocation.ToCompactString(), *IntersectionPoint.ToCompactString(),
		*FlickReferenceRotation.Rotator().ToCompactString(), *LocalIntersectionPoint.ToCompactString(),
		OriginalZOffset, DeltaZ, NewZOffset
	);

	return NewZOffset;
}

void APaperGolfPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// TODO: Move to timer on flick at lower tick rate
	SampleState();
}

void APaperGolfPawn::BeginPlay()
{
	Super::BeginPlay();

	States.Reserve(NumSamples);
}

void APaperGolfPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	_CameraSpringArm = FindComponentByClass<USpringArmComponent>();

	if (ensureMsgf(_CameraSpringArm, TEXT("%s: CameraSpringArm is NULL"), *GetName()))
	{
		_Camera = Cast<UCameraComponent>(_CameraSpringArm->GetChildComponent(0));
		ensureMsgf(_Camera, TEXT("%s: Camera is NULL"), *GetName());
	}

	_PaperGolfMesh = Cast<UStaticMeshComponent>(GetRootComponent());
	if (ensureMsgf(_PaperGolfMesh, TEXT("%s: PaperGolfMesh is NULL"), *GetName()))
	{
		TArray<USceneComponent*> Components;
		_PaperGolfMesh->GetChildrenComponents(false, Components);

		const auto FindResult = Components.FindByPredicate([&](const auto Component)
		{
			return Component != _CameraSpringArm;
		});

		ensureMsgf(FindResult, TEXT("%s: Cannot find FlickReference"), *GetName());

		_FlickReference = *FindResult;
		ensureMsgf(_FlickReference, TEXT("%s: FlickReference is NULL"), *GetName());
	}
}

float APaperGolfPawn::GetFlickMaxForce(EShotType ShotType) const
{
	return ShotType == EShotType::Close ? FlickMaxForceCloseShot : FlickMaxForce;
}

void APaperGolfPawn::SetCameraForFlick()
{
	check(_CameraSpringArm);

	_CameraSpringArm->bInheritYaw = false;
	_CameraSpringArm->bEnableCameraRotationLag = true;
}

void APaperGolfPawn::ResetCameraForShotSetup()
{
	check(_CameraSpringArm);

	_CameraSpringArm->bInheritYaw = true;
	_CameraSpringArm->bEnableCameraRotationLag = false;

	if (!FocusActor)
	{
		return;
	}

	const auto LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), FocusActor->GetActorLocation());

	check(_PaperGolfMesh);

	_PaperGolfMesh->AddRelativeRotation(
		FRotator{ 0, LookAtRotation.Yaw, 0 }
	);
}

void APaperGolfPawn::SampleState()
{
	if (!RootComponent->IsSimulatingPhysics())
	{
		States.Reset();
		StateIndex = -1;
		return;
	}

	StateIndex = (StateIndex + 1) % NumSamples;

	if (States.Num() < NumSamples)
	{
		States.Emplace(*this);
		return;
	}

	States[StateIndex] = *this;
}

APaperGolfPawn::FState::FState(const APaperGolfPawn& Pawn) : 
	Position(Pawn.GetActorLocation())
{

}

#pragma region Visual Logger

#if ENABLE_VISUAL_LOG

void APaperGolfPawn::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	FVisualLogStatusCategory Category;
	Category.Category = FString::Printf(TEXT("PaperGolfPawn (%s)"), *GetName());

	Category.Add(TEXT("FocusActor"), *LoggingUtils::GetName(FocusActor));
	Category.Add(TEXT("InPerpetualMotion"), LoggingUtils::GetBoolString(IsStuckInPerpetualMotion()));
	Category.Add(TEXT("FlickLocation"), FlickLocation.ToCompactString());
	Category.Add(TEXT("Location"), GetActorLocation().ToCompactString());
	Category.Add(TEXT("Rotation"), GetActorRotation().ToCompactString());

	DrawPawn(Snapshot);

	Snapshot->Status.Add(Category);
}

void APaperGolfPawn::DrawPawn(FVisualLogEntry* Snapshot) const
{
	if(!IsValid(_PaperGolfMesh))
	{
		return;
	}

	PG::VisualLoggerUtils::DrawStaticMeshComponent(*Snapshot, LogPGPawn.GetCategoryName(), *_PaperGolfMesh);
}

#endif
#pragma endregion Visual Logger