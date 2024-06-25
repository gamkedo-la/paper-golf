// Copyright Game Salutes. All Rights Reserved.


#include "PaperGolfPawn.h"

#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h" 

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

#include "Kismet/KismetMathLibrary.h"

#include "BuildUtilities.h"

#include "PaperGolfUtilities.h"

#include "VisualLogger/VisualLogger.h"
#include "PaperGolfLogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfPawn)

// Sets default values
APaperGolfPawn::APaperGolfPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
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

	// TODO: Create ground trace channel

	auto World = GetWorld();
	
	if (!ensure(World))
	{
		return;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const auto& TraceStart = GetActorLocation();

	FHitResult HitResult;
	
	if (!World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceStart - 2000 * GetActorUpVector(),
		ECollisionChannel::ECC_Visibility,
		QueryParams))
	{
		UE_VLOG_UELOG(this,
			LogPaperGolfGame,
			Warning,
			TEXT("%s: Could not determine ground!"),
			*GetName()
		);

		return;
	}

	SetActorLocation(HitResult.Location);
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

FVector APaperGolfPawn::GetFlickForce(float Accuracy, float Power) const
{
	const auto& PerfectShotFlickDirection = GetFlickDirection();

	// Apply accuracy offsets
	const auto AccuracyOffset = FMath::Sign(Accuracy) * FMath::Pow(Accuracy, PowerAccuracyExp);

	// Transform only the yaw
	const auto FlickDirectionDeviation = GetActorTransform().TransformVectorNoScale(
		FVector{ 0.0, AccuracyOffset, 0.0 });

	const auto FlickDirection = (PerfectShotFlickDirection + FlickDirectionDeviation).GetSafeNormal();

	const auto FlickMaxPower = GetFlickMaxForce();

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

	UPaperGolfUtilities::ResetPhysicsState(_PaperGolfMesh);
}

void APaperGolfPawn::Flick(float LocalZOffset, float PowerFraction, float Accuracy)
{
	check(_PaperGolfMesh);

	SetCameraForFlick();

	// Turn off physics at first so can move the actor
	_PaperGolfMesh->SetSimulatePhysics(true);
	_PaperGolfMesh->AddImpulseAtLocation(
		GetFlickForce(Accuracy, PowerFraction),
		GetFlickLocation(LocalZOffset, Accuracy, PowerFraction)
	);

	_PaperGolfMesh->SetEnableGravity(true);
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

float APaperGolfPawn::GetFlickMaxForce() const
{
	if (IsCloseShot())
	{
		return FlickMaxForceCloseShot;
	}


	return UBuildUtilities::GetBuildCharacteristics().bGame ? FlickMaxForce * StandaloneGameMutliplier : FlickMaxForce;
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
