// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/MovingObstacle.h"

// Sets default values
AMovingObstacle::AMovingObstacle()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
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

// Called when the game starts or when spawned
void AMovingObstacle::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMovingObstacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SetDistance();
	EvaluatePosition();
}
void AMovingObstacle::EvaluatePosition()
{
	staticMesh->SetWorldTransform(movementPath->GetTransformAtDistanceAlongSpline(distance,ESplineCoordinateSpace::World));
}
float valueToAdd = 1;
void AMovingObstacle::SetDistance()
{
	if (distance >= movementPath->GetSplineLength())
		valueToAdd = -1;
	if (distance <= 0)
		valueToAdd = 1;

	distance+=valueToAdd*speed;
}

