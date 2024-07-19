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
#include "Kismet/GameplayStatics.h"
#include "Build/BuildUtilities.h"

#include "Library/PaperGolfPawnUtilities.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "Utils/StringUtils.h"
#include "Utils/CollisionUtils.h"

#include "Utils/VisualLoggerUtils.h"
#include "PGPawnLogging.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfPawn)

namespace
{
	constexpr float ValidateFloatEpsilon = 0.001f;
	constexpr float ValidateZOffsetExtentFraction = 0.5f;
}

APaperGolfPawn::APaperGolfPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	bAlwaysRelevant = true;
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
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: SetFocusActor - Focus=%s"), *GetName(), *LoggingUtils::GetName(Focus));

	FocusActor = Focus;

	ResetCameraForShotSetup();
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

	const auto StartLocation = TraceStart + Bounds.GetExtent().Z * ActorUpVector;
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
		//MulticastReliableSetTransform(Location, true, GetActorRotation());
		MulticastReliableSetTransform(Location, false);
	}
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
	check(_PaperGolfMesh);

	UPaperGolfPawnUtilities::ResetPhysicsState(_PaperGolfMesh);
}

void APaperGolfPawn::MulticastReliableSetTransform_Implementation(const FVector_NetQuantize& Position, bool bUseRotation, const FRotator& Rotation)
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

	SetTransform(Position, bUseRotation ? TOptional<FRotator> {Rotation} : TOptional<FRotator>{});
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

	RefreshMass();

	const auto& Impulse = GetFlickForce(FlickParams.ShotType, FlickParams.Accuracy, FlickParams.PowerFraction);
	const auto& Location = GetFlickLocation(FlickParams.LocalZOffset, FlickParams.Accuracy, FlickParams.PowerFraction);

#if ENABLE_VISUAL_LOG
	UE_VLOG_LOCATION(this, LogPGPawn, Log, Location, 5.0f, FColor::Green, TEXT("Flick"));
	UE_VLOG_ARROW(this, LogPGPawn, Log, Location, Location + Impulse, FColor::Green, TEXT("Flick"));

	if (FVisualLogger::IsRecording())
	{
		DrawPawn(FColor::Cyan);
	}
#endif

	_PaperGolfMesh->AddImpulseAtLocation(Impulse, Location);
	_PaperGolfMesh->SetEnableGravity(true);
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
	Params.LaunchVelocity = FlickImpulse / GetMass();

	// TODO: Later toggle with console variable
	//Params.DrawDebugType = EDrawDebugTrace::ForDuration;
	//Params.DrawDebugTime = 10.0f;

	UE_VLOG_UELOG(this, LogPGPawn, Log, 
		TEXT("%s: PredictFlick - FlickDirection=%s; FlickForceMagnitude=%.1f; StartLocation=%s; LaunchVelocity=%scm/s"),
		*GetName(),
		*FlickDirection.ToCompactString(),
		FlickImpulse.Size(),
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
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();

	// TODO: May change this if undeterministic physics too much of a problem
	// but then need to send client rpcs with rotation updates at a regular interval to show other clients
	// what player is doing
	SetReplicateMovement(false);

	States.Reserve(NumSamples);

	InitDebugDraw();
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

		if (ensureMsgf(FindResult, TEXT("%s: Cannot find FlickReference"), *GetName()))
		{
			_FlickReference = *FindResult;
			ensureMsgf(_FlickReference, TEXT("%s: FlickReference is NULL"), *GetName());
		}
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

float APaperGolfPawn::GetMass() const
{
	return Mass = CalculateMass();
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

float APaperGolfPawn::CalculateMass() const
{
	if(!FMath::IsNearlyZero(Mass))
	{
		return Mass;
	}

	if (!ensure(_PaperGolfMesh))
	{
		return 0.0f;
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
		UPaperGolfPawnUtilities::ResetPhysicsState(_PaperGolfMesh);
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