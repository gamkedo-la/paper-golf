// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Pawn/PaperGolfPawn.h"

#include "PaperGolfTypes.h"

#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h" 

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

#include "Engine/StaticMeshSocket.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Build/BuildUtilities.h"

#include "Library/PaperGolfPawnUtilities.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "Utils/StringUtils.h"
#include "Utils/CollisionUtils.h"

#include "Utils/VisualLoggerUtils.h"
#include "PGPawnLogging.h"

#include "Net/UnrealNetwork.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfPawn)

namespace
{
	constexpr float ValidateFloatEpsilon = 0.001f;
	constexpr float ValidateZOffsetExtentFraction = 0.5f;

	// TODO: Move into separate constants namespace exported if needed outside this class
	const FName BottomSocketName = TEXT("Bottom");
	const FName FlickSocketName = TEXT("Flick");
}

APaperGolfPawn::APaperGolfPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	bAlwaysRelevant = true;
	bReplicates = true;
}

void APaperGolfPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APaperGolfPawn, FocusActor);
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

FVector APaperGolfPawn::GetCenterOfMassPosition() const
{
	if (!ensure(_PaperGolfMesh))
	{
		return FVector::ZeroVector;
	}

	FBodyInstance* Body = _PaperGolfMesh->GetBodyInstance();
	if (!ensure(Body))
	{
		return FVector::ZeroVector;
	}

	return Body->GetCOMPosition();
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

// We need to check the _PaperGolfMesh since a physics actor pops out of the hierarchy

FVector APaperGolfPawn::GetPaperGolfPosition() const
{
	if (ensure(_PaperGolfMesh) && GetRootComponent() != _PaperGolfMesh->GetAttachmentRoot())
	{
		return _PaperGolfMesh->GetComponentLocation();
	}

	return GetActorLocation();
}

FRotator APaperGolfPawn::GetPaperGolfRotation() const
{
	if (ensure(_PaperGolfMesh) && GetRootComponent() != _PaperGolfMesh->GetAttachmentRoot())
	{
		return _PaperGolfMesh->GetComponentRotation();
	}

	return GetActorRotation();
}

void APaperGolfPawn::PostNetReceive()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: PostNetReceive"), *GetName());

	Super::PostNetReceive();
}

void APaperGolfPawn::OnRep_ReplicatedMovement()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnRep_ReplicatedMovement"), *GetName());

	Super::OnRep_ReplicatedMovement();
}

void APaperGolfPawn::PostNetReceiveLocationAndRotation()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: PostNetReceiveLocationAndRotation"), *GetName());

	Super::PostNetReceiveLocationAndRotation();
}

void APaperGolfPawn::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: PostNetReceiveVelocity"), *GetName());

	Super::PostNetReceiveVelocity(NewVelocity);
}

void APaperGolfPawn::PostNetReceivePhysicState()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: PostNetReceivePhysicState"), *GetName());

	Super::PostNetReceivePhysicState();
}

void APaperGolfPawn::AddCameraRelativeRotation(const FRotator& DeltaRotation)
{
	if(!ensure(_CameraSpringArm))
	{
		return;
	}

	FRotator RelativeRotation = _CameraSpringArm->GetRelativeRotation();
	RelativeRotation.Pitch = FMath::ClampAngle(RelativeRotation.Pitch + DeltaRotation.Pitch, MinCameraRotation.Pitch, MaxCameraRotation.Pitch);
	RelativeRotation.Yaw = FMath::ClampAngle(RelativeRotation.Yaw + DeltaRotation.Yaw, MinCameraRotation.Yaw, MaxCameraRotation.Yaw);

	_CameraSpringArm->SetRelativeRotation(RelativeRotation);

	UE_VLOG_UELOG(this, LogPGPawn, VeryVerbose, TEXT("%s: AddCameraRelativeRotation: %s -> %s"),
		*GetName(), *DeltaRotation.ToCompactString(), *_CameraSpringArm->GetRelativeRotation().ToCompactString());
}

void APaperGolfPawn::ResetCameraRelativeRotation()
{
	if (!ensure(_CameraSpringArm))
	{
		return;
	}

	UE_VLOG_UELOG(this, LogPGPawn, VeryVerbose, TEXT("%s: ResetCameraRelativeRotation: %s -> %s"),
		*GetName(), *_CameraSpringArm->GetRelativeRotation().ToCompactString(), *InitialSpringArmRotation.ToCompactString());

	_CameraSpringArm->SetRelativeRotation(InitialSpringArmRotation);
	_CameraSpringArm->TargetArmLength = InitialCameraSpringArmLength;
}

void APaperGolfPawn::AddCameraZoomDelta(float ZoomDelta)
{
	if (!ensure(_CameraSpringArm))
	{
		return;
	}

	const auto NewTargetArmLength = FMath::Clamp(_CameraSpringArm->TargetArmLength + ZoomDelta, MinCameraSpringArmLength, MaxCameraSpringArmLength);

	UE_VLOG_UELOG(this, LogPGPawn, VeryVerbose, TEXT("%s: AddCameraZoomDelta: %f -> %f"),
		*GetName(), _CameraSpringArm->TargetArmLength, NewTargetArmLength);

	_CameraSpringArm->TargetArmLength = NewTargetArmLength;
}

void APaperGolfPawn::SetFocusActor(AActor* Focus)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: SetFocusActor - Focus=%s"), *GetName(), *LoggingUtils::GetName(Focus));

	FocusActor = Focus;

	ResetCameraForShotSetup();
}

void APaperGolfPawn::OnRep_FocusActor()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnRep_FocusActor - Focus=%s"), *GetName(), *LoggingUtils::GetName(FocusActor));

	// If client spectating, need to update camera focus
	if (GetLocalRole() == ENetRole::ROLE_SimulatedProxy)
	{
		ResetRotation();
	}
}

void APaperGolfPawn::SnapToGround()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: SnapToGround"), *GetName());

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

	const auto& BoundsExtent = Bounds.GetExtent();
	const auto UpExtent = BoundsExtent * ActorUpVector;

	const auto StartLocation = TraceStart + UpExtent;
	const auto EndLocation = TraceStart - 2000 * ActorUpVector;

	UE_VLOG_SEGMENT_THICK(this, LogPGPawn, Log, StartLocation, EndLocation, FColor::Yellow, 10.0, TEXT("GroundTrace"));

	if (!World->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		PG::CollisionChannel::FlickTraceType,
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

	if (HasAuthority())
	{
		MulticastReliableSetTransform(Location, true);
	}
}

void APaperGolfPawn::ResetRotation()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: ResetRotation: %s -> %s"), *GetName(), *GetActorRotation().ToCompactString(), *InitialRotation.ToCompactString());

	SetActorRotation(InitialRotation);

	check(_PaperGolfMesh);

	_PaperGolfMesh->SetWorldRotation(InitialRotation);

	ResetCameraForShotSetup();
}

FVector APaperGolfPawn::GetFlickDirection() const
{
	return GetActorRotation().Vector();
}

FVector APaperGolfPawn::GetFlickLocation(float LocationZ, float Accuracy, float Power) const
{
	check(_FlickReference);

	//const auto RawLocationAccuracyOffset = FMath::Abs(Accuracy) * LocationAccuracyMultiplier;
	//const auto AccuracyAdjustedLocationZ = LocationZ * (1 - 2 * RawLocationAccuracyOffset);

	const auto& FlickTransform = _FlickReference->GetComponentTransform();

	return FlickTransform.TransformPosition(
		FVector{ 0.0, 0.0, LocationZ });
}

FVector APaperGolfPawn::GetFlickForce(EShotType ShotType, float Accuracy, float Power) const
{
	const auto& PerfectShotFlickDirection = GetFlickDirection();

	// Apply accuracy offsets
	// It's inverted to get the correct hook and slice behavior
	const auto AccuracyOffset = -FMath::Sign(Accuracy) * FMath::Pow(Accuracy, PowerAccuracyExp);

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
	ResetPhysicsState();
}

void APaperGolfPawn::MulticastReliableSetTransform_Implementation(const FVector_NetQuantize& Position, bool bSnapToGround, bool bUseRotation, const FRotator& Rotation)
{
	if (GetLocalRole() != ENetRole::ROLE_SimulatedProxy)
	{
		// Skip on non-authoritative clients that are controlling the pawn as they receive a more substantial client event from server
		UE_VLOG_UELOG(this, LogPGPawn, Log,
			TEXT("%s: MulticastReliableSetTransform_Implementation - Skipping as LocalRole=%s"),
			*GetName(), *LoggingUtils::GetName(GetLocalRole())
		);
		return;
	}

	UE_VLOG_UELOG(this, LogPGPawn, Log,
		TEXT("%s: MulticastReliableSetTransform_Implementation - Position=%s; bUseRotation=%s; Rotation=%s"),
		*GetName(), *Position.ToCompactString(), LoggingUtils::GetBoolString(bUseRotation), *Rotation.ToCompactString()
	);

	// FIXME: Should we be calling this anymore on simulated proxies?
	// Re-evaluate all the multicast and client position update calls since it should be handled via replication now
	SetTransform(Position, bUseRotation ? TOptional<FRotator> {Rotation} : TOptional<FRotator>{});

	if (bSnapToGround)
	{
		UPaperGolfPawnUtilities::ReattachPhysicsComponent(_PaperGolfMesh, PaperGolfMeshInitialTransform);
		SnapToGround();
	}
}

void APaperGolfPawn::SetTransform(const FVector& Position, const TOptional<FRotator>& Rotation)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log,
		TEXT("%s: SetTransform - Position=%s; Rotation=%s"),
		*GetName(), *Position.ToCompactString(), *PG::StringUtils::ToString(Rotation)
	);

	SetUpForNextShot();

	if (Rotation)
	{
		SetActorLocationAndRotation(
			Position,
			*Rotation,
			false,
			nullptr,
			TeleportFlagToEnum(true)
		);
	}
	else
	{
		SetActorLocation(
			Position,
			false,
			nullptr,
			TeleportFlagToEnum(true)
		);
	}

	UE_VLOG_LOCATION(this, LogPGPawn, Log, Position, 20.0f, FColor::Red, TEXT("SetPositionTo"));
}

void APaperGolfPawn::MulticastSetCollisionEnabled_Implementation(bool bEnabled)
{
	if (GetLocalRole() != ENetRole::ROLE_SimulatedProxy)
	{
		// Skip on non-authoritative clients that are controlling the pawn as they receive a more substantial client event from server
		UE_VLOG_UELOG(this, LogPGPawn, Log,
			TEXT("%s: MulticastSetCollisionEnabled_Implementation - bEnabled=%s: Skip as LocalRole=%s"),
			*GetName(), LoggingUtils::GetBoolString(bEnabled), *LoggingUtils::GetName(GetLocalRole())
		);
		return;
	}

	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: MulticastSetCollisionEnabled_Implementation - bEnabled=%s"), *GetName(), LoggingUtils::GetBoolString(bEnabled));

	SetCollisionEnabled(bEnabled);
}

void APaperGolfPawn::SetCollisionEnabled(bool bEnabled)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: SetCollisionEnabled - bEnabled=%s"), *GetName(), LoggingUtils::GetBoolString(bEnabled));

	check(_PaperGolfMesh);

	if (!bEnabled)
	{
		_PaperGolfMesh->SetSimulatePhysics(false);
	}
	// Will explicitly opt-in to physics simulation when flicking
	
	_PaperGolfMesh->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void APaperGolfPawn::Flick(const FFlickParams& FlickParams)
{
	SetCameraForFlick();

	DoFlick(FlickParams);

	if (HasAuthority())
	{
		// Broadcast to other clients
		MulticastFlick(ToNetworkParams(FlickParams));
	}
	else
	{
		// Send to server
		ServerFlick(ToNetworkParams(FlickParams));
	}

	// Wait until next shot to be able to hit again
	bReadyForShot = false;
}

void APaperGolfPawn::AddDeltaRotation(const FRotator& DeltaRotation)
{
	UE_VLOG_UELOG(this, LogPGPawn, VeryVerbose, TEXT("%s: AddDeltaRotation - DeltaRotation=%s"), *GetName(), *DeltaRotation.ToCompactString());

	if (!ensure(_PaperGolfMesh))
	{
		return;
	}

	if (FMath::IsNearlyZero(DeltaRotation.Yaw))
	{
		AddActorLocalRotation(DeltaRotation);
	}
	else
	{
		AddActorWorldRotation(DeltaRotation);
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

void APaperGolfPawn::DoFlick(FFlickParams FlickParams)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log,
		TEXT("%s: DoFlick - ShotType=%s; LocalZOffset=%f; PowerFraction=%f; Accuracy=%f"),
		*GetName(), *LoggingUtils::GetName(FlickParams.ShotType), FlickParams.LocalZOffset, FlickParams.PowerFraction, FlickParams.Accuracy
	);

	check(_PaperGolfMesh);

	FlickParams.Clamp();

	SetCollisionEnabled(true);

	// Turn off physics at first so can move the actor
	_PaperGolfMesh->SetSimulatePhysics(true);

	const auto& Impulse = GetFlickForce(FlickParams.ShotType, FlickParams.Accuracy, FlickParams.PowerFraction);
	const auto& Location = GetFlickLocation(FlickParams.LocalZOffset, FlickParams.Accuracy, FlickParams.PowerFraction);

#if ENABLE_VISUAL_LOG

	UE_VLOG_LOCATION(this, LogPGPawn, Log, Location, 5.0f, FColor::Green, TEXT("Flick"));
	UE_VLOG_ARROW(this, LogPGPawn, Log, Location, Location + Impulse, FColor::Green, TEXT("Flick"));
	UE_VLOG_LOCATION(this, LogPGPawn, Log, GetCenterOfMassPosition(), 5.0f, FColor::Yellow, TEXT("COM"));

	if (FVisualLogger::IsRecording())
	{
		DrawPawn(FColor::Cyan);
	}
#endif

	_PaperGolfMesh->AddImpulseAtLocation(Impulse, Location);

	OnFlick.Broadcast();
}

void APaperGolfPawn::DoNetworkFlick(const FNetworkFlickParams& Params)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log,
		TEXT("%s: DoNetworkFlick - Rotation=%s; LocalZOffset=%f; PowerFraction=%f; Accuracy=%f"),
		*GetName(), *Params.Rotation.ToCompactString(), Params.FlickParams.LocalZOffset, Params.FlickParams.PowerFraction, Params.FlickParams.Accuracy
	);

	SetActorRotation(Params.Rotation);

	DoFlick(Params.FlickParams);
}

void APaperGolfPawn::MulticastFlick_Implementation(const FNetworkFlickParams& Params)
{
	if (GetLocalRole() != ENetRole::ROLE_SimulatedProxy)
	{
		// Skip on non-authoritative clients that are controlling the pawn as they receive a more substantial client event from server
		UE_VLOG_UELOG(this, LogPGPawn, Log,
			TEXT("%s: MulticastFlick_Implementation - Skip as LocalRole=%s"),
			*GetName(), *LoggingUtils::GetName(GetLocalRole())
		);
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

	// Broadcast to other clients
	MulticastFlick(Params);
}

bool APaperGolfPawn::ServerFlick_Validate(const FNetworkFlickParams& Params)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log,
		TEXT("%s: ServerFlick_Validate - Rotation=%s; LocalZOffset=%f; PowerFraction=%f; Accuracy=%f"),
		*GetName(), *Params.Rotation.ToCompactString(), Params.FlickParams.LocalZOffset, Params.FlickParams.PowerFraction, Params.FlickParams.Accuracy
	);

	// TODO: If this becomes problematic may just want to log a warning in ServerFlick_Implementation and not process the RPC
	if(!bReadyForShot)
	{
		UE_VLOG_UELOG(this, LogPGPawn, Warning,
			TEXT("%s: ServerFlick_Validate - FALSE - Not ready for shot"),
			*GetName()
		);

		return false;
	}

	const auto& FlickParams = Params.FlickParams;

	FFlickParams ClampedFlickParmas{ FlickParams };
	ClampedFlickParmas.Clamp();

	if (!FMath::IsNearlyEqual(FlickParams.PowerFraction, ClampedFlickParmas.PowerFraction, ValidateFloatEpsilon))
	{
		UE_VLOG_UELOG(this, LogPGPawn, Warning,
			TEXT("%s: ServerFlick_Validate - FALSE - PowerFraction=%f is not in range [0, 1]"),
			*GetName(), FlickParams.PowerFraction
		);

		return false;
	}

	if (!FMath::IsNearlyEqual(FlickParams.Accuracy, ClampedFlickParmas.Accuracy, ValidateFloatEpsilon))
	{
		UE_VLOG_UELOG(this, LogPGPawn, Warning,
			TEXT("%s: ServerFlick_Validate - FALSE - PowerFraction=%f is not in range [-1, 1]"),
			*GetName(), FlickParams.PowerFraction
		);

		return false;
	}

	if(FlickParams.ShotType == EShotType::MAX)
	{
		UE_VLOG_UELOG(this, LogPGPawn, Warning,
			TEXT("%s: ServerFlick_Validate - FALSE - ShotType is MAX"),
			*GetName()
		);

		return false;
	}

	// Make sure that OffsetZ not way out of bounds
	const auto Box = PG::CollisionUtils::GetAABB(*this);
	const auto ZExtent = Box.GetExtent().Z;

	const auto ClampedZ = FMath::Clamp(FlickParams.LocalZOffset, -ZExtent, ZExtent);

	// Be lenient on the z range to avoid potential legitimate failure cases
	if (!FMath::IsNearlyEqual(ClampedZ, FlickParams.LocalZOffset, ZExtent * ValidateZOffsetExtentFraction))
	{
		UE_VLOG_UELOG(this, LogPGPawn, Warning,
			TEXT("%s: ServerFlick_Validate - FALSE - LocalZOffset=%f is out of bounds. Expected [%f,%f]"),
			*GetName(), FlickParams.LocalZOffset, -ZExtent, ZExtent
		);

		return false;
	}

	UE_VLOG_UELOG(this, LogPGPawn, Verbose,
		TEXT("%s: ServerFlick_Validate - TRUE - LocalZOffset=%f within [%f,%f] when accounting for extent fraction [%f,%f]"),
		*GetName(), FlickParams.LocalZOffset, -ZExtent, ZExtent, -ZExtent * (1 + ValidateZOffsetExtentFraction), ZExtent * (1 + ValidateZOffsetExtentFraction)
	);

	return true;
}


float APaperGolfPawn::ClampFlickZ(float OriginalZOffset, float DeltaZ) const
{
	// TODO: Use the Top and Bottom sockets instead to get the extensions of DeltaZ
	// This is more precise than using the bounding box transformed and gives designers more control over the extents
	// 
	// Do a sweep test from OriginalZOffset + DeltaZ back to OriginalZOfset with GetFlickLocation
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

bool APaperGolfPawn::PredictFlick(const FFlickParams& FlickParams, const FFlickPredictParams& FlickPredictParams, FPredictProjectilePathResult& Result) const
{
	const auto FlickImpulse = GetFlickForce(FlickParams.ShotType, FlickParams.Accuracy, FlickParams.PowerFraction);
	const auto DragForceMultiplier = GetFlickDragForceMultiplier(FlickImpulse.Size());
	const auto DragAdjustedFlickImpulse = FlickImpulse * DragForceMultiplier;

	const auto& FlickDirection = FlickImpulse.GetSafeNormal();

	FPredictProjectilePathParams Params;
	// Offset start location a bit so that we don't collide with walls so that edge of sphere is on the flick location
	Params.StartLocation = GetFlickLocation(FlickParams.LocalZOffset) + FlickDirection * FlickPredictParams.CollisionRadius;
	Params.ActorsToIgnore.Add(const_cast<APaperGolfPawn*>(this));
	Params.bTraceWithCollision = true;
	Params.bTraceWithChannel = true;
	Params.TraceChannel = PG::CollisionChannel::FlickTraceType;
	Params.ProjectileRadius = FlickPredictParams.CollisionRadius;
	Params.MaxSimTime = FlickPredictParams.MaxSimTime;
	Params.SimFrequency = FlickPredictParams.SimFrequency;

	// Impulse = change in momentum
	Params.LaunchVelocity = DragAdjustedFlickImpulse / GetMass();

	// TODO: Later toggle with console variable
	//Params.DrawDebugType = EDrawDebugTrace::ForDuration;
	//Params.DrawDebugTime = 10.0f;

	UE_VLOG_UELOG(this, LogPGPawn, Log, 
		TEXT("%s: PredictFlick - FlickDirection=%s; FlickForceMagnitude=%.1f; DragForceMultiplier=%.1f; StartLocation=%s; LaunchVelocity=%scm/s"),
		*GetName(),
		*FlickDirection.ToCompactString(),
		DragAdjustedFlickImpulse.Size(),
		DragForceMultiplier,
		*Params.StartLocation.ToCompactString(),
		*Params.LaunchVelocity.ToCompactString());

	const bool bHit = UGameplayStatics::PredictProjectilePath(
		GetWorld(),
		Params,
		Result);

#if ENABLE_VISUAL_LOG
	if (FVisualLogger::IsRecording())
	{
		if (bHit)
		{
			UE_VLOG_UELOG(this, LogPGPawn, Verbose, TEXT("%s: PredictFlick - Hit %s-%s at %s"), 
				*GetName(),
				*LoggingUtils::GetName(Result.HitResult.GetActor()), *LoggingUtils::GetName(Result.HitResult.GetComponent()), 
				*Result.HitResult.ImpactPoint.ToCompactString());

			UE_VLOG_BOX(this, LogPGPawn, Verbose, FBox::BuildAABB(Result.HitResult.ImpactPoint, FVector{ 10.0 }), FColor::Red, TEXT("Hit"));
		}
		else
		{
			UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: PredictFlick - No hit found"), *GetName());
		}

		for (int32 i = 0; const auto & PathDatum : Result.PathData)
		{
			UE_VLOG_LOCATION(
				this, LogPGPawn, Verbose, PathDatum.Location, Params.ProjectileRadius, FColor::Green, TEXT("P%d"), i);
			++i;
		}
	}
#endif

	return bHit;
}

void APaperGolfPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// TODO: Move to timer on flick at lower tick rate
	SampleState();
}

void APaperGolfPawn::BeginPlay()
{
	InitialRotation = GetActorRotation();

	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: BeginPlay: InitialRotation=%s"), *GetName(), *InitialRotation.ToCompactString());

	Super::BeginPlay();

	SetReplicateMovement(true);

	States.Reserve(NumSamples);

	InitDebugDraw();
}

void APaperGolfPawn::FellOutOfWorld(const UDamageType& DmgType)
{
	UE_VLOG_UELOG(this, LogPGPawn, Warning, TEXT("%s: FellOutOfWorld - %s"), *GetName(), *LoggingUtils::GetName(DmgType));

	Super::FellOutOfWorld(DmgType);

	auto World = GetWorld();
	if(!ensure(World))
	{
		return;
	}

	if (auto GolfEventsSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventsSubsystem))
	{
		GolfEventsSubsystem->OnPaperGolfPawnClippedThroughWorld.Broadcast(this);
	}
}

void APaperGolfPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: EndPlay - %s"), *GetName(), *LoggingUtils::GetName(EndPlayReason));

	Super::EndPlay(EndPlayReason);

	States.Reset();
	StateIndex = 0;

	CleanupDebugDraw();
}

void APaperGolfPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	_PivotComponent = GetRootComponent();

	ensureMsgf(_PivotComponent, TEXT("%s: PivotComponent is NULL"), *GetName());

	_CameraSpringArm = FindComponentByClass<USpringArmComponent>();

	if (ensureMsgf(_CameraSpringArm, TEXT("%s: CameraSpringArm is NULL"), *GetName()))
	{
		OriginalCameraRotationLag = _CameraSpringArm->CameraRotationLagSpeed;
		InitialSpringArmRotation = _CameraSpringArm->GetRelativeRotation();
		InitialCameraSpringArmLength = _CameraSpringArm->TargetArmLength;

		_Camera = Cast<UCameraComponent>(_CameraSpringArm->GetChildComponent(0));
		ensureMsgf(_Camera, TEXT("%s: Camera is NULL"), *GetName());
	}

	_PaperGolfMesh = FindComponentByClass<UStaticMeshComponent>();
	if (ensureMsgf(_PaperGolfMesh, TEXT("%s: PaperGolfMesh is NULL"), *GetName()))
	{
		ensureMsgf(_PaperGolfMesh != _PivotComponent, TEXT("%s: PaperGolfMesh is the same as the root component"), *GetName());

		PaperGolfMeshInitialTransform = _PaperGolfMesh->GetRelativeTransform();

		TArray<USceneComponent*> Components;
		_PaperGolfMesh->GetChildrenComponents(false, Components);

		const auto FindResult = Components.FindByPredicate([&](const auto Component)
		{
			return Component != _CameraSpringArm;
		});

		if (ensureMsgf(FindResult, TEXT("%s: Cannot find FlickReference"), *GetName()))
		{
			_FlickReference = *FindResult;
			ensureMsgf(_FlickReference, TEXT("%s: FlickReference is NULL"), *GetName());
		}

		Mass = CalculateMass();
	}
}

float APaperGolfPawn::GetFlickMaxForce(EShotType ShotType) const
{
	switch (ShotType)
	{
		case EShotType::Medium: return FlickMaxForceMediumShot;
		case EShotType::Close: return FlickMaxForceCloseShot;
		default: return FlickMaxForce;
	}
}

float APaperGolfPawn::GetFlickDragForceMultiplier(float Power) const
{
	if (!ensureMsgf(FlickDragForceCurve, TEXT("%s: FlickDragForceCurve is NULL"), *GetName()))
	{
		return 1.0f;
	}

	return FlickDragForceCurve->FloatCurve.Eval(Power, 1.0f);
}

float APaperGolfPawn::GetMass() const
{
	return Mass;
}

void APaperGolfPawn::SetCameraForFlick()
{
	check(_CameraSpringArm);

	_CameraSpringArm->bInheritYaw = false;
	_CameraSpringArm->bEnableCameraRotationLag = true;
	_CameraSpringArm->CameraRotationLagSpeed = OriginalCameraRotationLag;

	ResetCameraRelativeRotation();
}

bool APaperGolfPawn::ShouldEnableCameraRotationLagForShotSetup() const
{
	// If this is an autonomous proxy make sure rotation lag is enabled to avoid jerky-looking aiming
	return !IsLocallyControlled();
}

void APaperGolfPawn::ResetPhysicsState() const
{
	check(_PaperGolfMesh);

	UPaperGolfPawnUtilities::ResetPhysicsState(_PaperGolfMesh, PaperGolfMeshInitialTransform);
}

void APaperGolfPawn::ResetCameraForShotSetup()
{
	check(_CameraSpringArm);

	_CameraSpringArm->bInheritYaw = true;
	ResetCameraRelativeRotation();

	const bool bEnableCameraRotationLag = ShouldEnableCameraRotationLagForShotSetup();
	// Enable on next tick so focus happens immediately
	// TODO: Sometimes the camera still spins on the first shot set up even though we are setting bEnableCameraRotationLag to false here
	_CameraSpringArm->bEnableCameraRotationLag = false;

	if (bEnableCameraRotationLag)
	{
		_CameraSpringArm->CameraRotationLagSpeed = SpectatorCameraRotationLag;

		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			// Make sure that camera rotation lag should still be enabled when this fires
			if (ShouldEnableCameraRotationLagForShotSetup())
			{
				_CameraSpringArm->bEnableCameraRotationLag = true;
			}
		}), 0.05f, false);
	}
	else
	{
		// Set back to defaults
		_CameraSpringArm->CameraRotationLagSpeed = OriginalCameraRotationLag;
	}

	if (!FocusActor)
	{
		UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: ResetCameraForShotSetup - Skipping as there is no focus actor"), *GetName());

		return;
	}

	const auto LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), FocusActor->GetActorLocation());

	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: ResetCameraForShotSetup - FocusActor=%s; LookAtRotationYaw=%f"), *GetName(), *LoggingUtils::GetName(FocusActor), LookAtRotation.Yaw);
	UE_VLOG_LOCATION(this, LogPGPawn, Log, FocusActor->GetActorLocation(), 20.0, FColor::Turquoise, TEXT("Focus"));

	SetActorRotation(FRotator{ 0, LookAtRotation.Yaw, 0 });
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

float APaperGolfPawn::CalculateMass() const
{
	// TODO: Should we be setting Mass as replicated and then this can only be called on the server to avoid needing to change the physics simulation state on clients?
	// Would need to call early on in PostInitializeComponents for instance so that the value can propagate to clients before they need to read that value
	// even then there could be a race condition
	// Ideally we want to remove all calls to SetSimulatePhysics on clients and only do that on server as it also affects attachment setup that should be done on server so it can replicate properly
	// Alternatively this could just be a property
	if(!FMath::IsNearlyZero(Mass))
	{
		return Mass;
	}

	if (!ensure(_PaperGolfMesh))
	{
		return 0.0f;
	}

	// Need to set an override on the mass to avoid needing to calculate it at runtime. Make sure that has been done
	const auto BodyInstance = _PaperGolfMesh->GetBodyInstance();
	if (!ensure(BodyInstance))
	{
		return 0.0f;
	}

	if(const auto OverrideMass = BodyInstance->GetMassOverride(); 
		ensureMsgf(!FMath::IsNearlyZero(OverrideMass), TEXT("%s: CalculateMass - OverrideMass is zero - set this to a non-zero value in the blueprint"), *GetName()))
	{
		return OverrideMass;
	}

	bool bSetSimulatePhysics{};

	if (!_PaperGolfMesh->IsSimulatingPhysics())
	{
		// necessary to read the mass
		_PaperGolfMesh->SetSimulatePhysics(true);

		bSetSimulatePhysics = true;
	}

	const auto CalculatedMass = _PaperGolfMesh->GetMass();

	if (bSetSimulatePhysics)
	{
		ResetPhysicsState();
	}

	return CalculatedMass;
}

void FFlickParams::Clamp()
{
	PowerFraction = FMath::Clamp(PowerFraction, 0.0f, 1.0f);
	Accuracy = FMath::Clamp(Accuracy, -1.0f, 1.0f);
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

	if (_PivotComponent)
	{
		FVisualLogStatusCategory PivotCategory;
		PivotCategory.Category = TEXT("Pivot");
		PivotCategory.Add(TEXT("Location"), _PivotComponent->GetComponentLocation().ToCompactString());
		PivotCategory.Add(TEXT("Rotation"), _PivotComponent->GetComponentRotation().ToCompactString());
		Category.AddChild(PivotCategory);
	}

	if (_PaperGolfMesh)
	{
		FVisualLogStatusCategory MeshCategory;
		MeshCategory.Category = TEXT("Mesh");
		MeshCategory.Add(TEXT("World Location"), _PaperGolfMesh->GetComponentLocation().ToCompactString());
		MeshCategory.Add(TEXT("World Rotation"), _PaperGolfMesh->GetComponentRotation().ToCompactString());
		MeshCategory.Add(TEXT("Relative Location"), _PaperGolfMesh->GetRelativeLocation().ToCompactString());
		MeshCategory.Add(TEXT("Relative Rotation"), _PaperGolfMesh->GetRelativeRotation().ToCompactString());
		Category.AddChild(MeshCategory);
	}

	DrawPawn(FColor::Blue, Snapshot);

	Snapshot->Status.Add(Category);
}

void APaperGolfPawn::DrawPawn(const FColor& Color, FVisualLogEntry* Snapshot) const
{
	if(!IsValid(_PaperGolfMesh))
	{
		return;
	}

	if (!Snapshot)
	{
		Snapshot = FVisualLogger::GetEntryToWrite(this, LogPGPawn);
	}

	PG::VisualLoggerUtils::DrawStaticMeshComponent(*Snapshot, LogPGPawn.GetCategoryName(), *_PaperGolfMesh, Color);
}

void APaperGolfPawn::InitDebugDraw()
{
	// Ensure that state logged regularly so we see the updates in the visual logger
	// Only need to do this for simulated proxies as otherwise it will be logged with the controller
	if (GetLocalRole() != ENetRole::ROLE_SimulatedProxy)
	{
		return;
	}

	FTimerDelegate DebugDrawDelegate = FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			UE_VLOG(this, LogPGPawn, Log, TEXT("Get Player State"));
		});

	GetWorldTimerManager().SetTimer(VisualLoggerTimer, DebugDrawDelegate, 0.05f, true);
}

void APaperGolfPawn::CleanupDebugDraw()
{
	GetWorldTimerManager().ClearTimer(VisualLoggerTimer);
}

#else

void APaperGolfPawn::InitDebugDraw() {}
void APaperGolfPawn::CleanupDebugDraw() {}

#endif

#pragma endregion Visual Logger
