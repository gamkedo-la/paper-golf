// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/MovingObstacle.h"

#include "VisualLogger/VisualLogger.h"

#include "Logging/LoggingUtils.h"

#include "PGGameplayLogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovingObstacle)

AMovingObstacle::AMovingObstacle()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	if (RootComponent)
	{
		SetRootComponent(RootComponent);
	}

	movementPath = CreateDefaultSubobject<USplineComponent>(TEXT("Path"));
	if (movementPath)
	{
		movementPath->SetupAttachment(RootComponent);
	}

	staticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Obstacle"));
	if (staticMesh)
	{
		staticMesh->SetupAttachment(RootComponent);
	}
}

void AMovingObstacle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

// Called when the game starts or when spawned
void AMovingObstacle::BeginPlay()
{
	Super::BeginPlay();

	Init();
}

void AMovingObstacle::EnableTick(bool bEnabled)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: EnableTick - bEnabled=%s"), *GetName(), LoggingUtils::GetBoolString(bEnabled));

	PrimaryActorTick.SetTickFunctionEnable(bEnabled);
}

void AMovingObstacle::Init()
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: Init"), *GetName());

	// Only tick on the server as we will replicate the movement down to clients
	EnableTick(HasAuthority());

	check(staticMesh);
	staticMesh->SetIsReplicated(true);

	// Only enable collision on server and let the collision response replicate to clients
	if (HasAuthority())
	{
		const auto& RelativeTransform = staticMesh->GetRelativeTransform();
		InitialRelativeLocation = RelativeTransform.GetLocation();
		InitialRelativeRotation = RelativeTransform.GetRotation();
	}
	else
	{
		staticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AMovingObstacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetDistance();
	EvaluatePosition();
}

void AMovingObstacle::EvaluatePosition()
{
	const auto& SplineTransform = movementPath->GetTransformAtDistanceAlongSpline(distance, ESplineCoordinateSpace::World);
	// Preserve local transform of static mesh
	const auto& LocalTransform = staticMesh->GetRelativeTransform();

	// Combine the local transform with the spline transform
	FTransform UpdatedTransform = staticMesh->GetComponentTransform();
	UpdatedTransform.SetLocation(SplineTransform.GetLocation() + InitialRelativeLocation);
	UpdatedTransform.SetRotation(InitialRelativeRotation * SplineTransform.GetRotation());

	// Set the final world transform of the static mesh
	staticMesh->SetWorldTransform(UpdatedTransform);
}

void AMovingObstacle::SetDistance()
{
	if (distance >= movementPath->GetSplineLength())
	{
		if(!movementPath->IsClosedLoop())
			valueToAdd = -1;
		else
			distance = 0;
	}

	if (distance <= 0)
		valueToAdd = 1;

	distance+=valueToAdd*speed;

	UE_VLOG_UELOG(this, LogPGGameplay, VeryVerbose, TEXT("%s: SetDistance - Distance=%f; ValueToAdd=%f"), *GetName(), distance, valueToAdd);
}
