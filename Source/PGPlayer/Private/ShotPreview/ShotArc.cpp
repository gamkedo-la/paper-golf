// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "ShotPreview/ShotArc.h"

#include "PGPlayerLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Kismet/GameplayStatics.h"

#include "Components/SplineComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineMeshComponent.h"

#include "Engine/StaticMesh.h"

AShotArc::AShotArc()
{
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	SplineComponent->SetupAttachment(RootComponent);

	LandingMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LandingMesh"));
	LandingMeshComponent->SetupAttachment(RootComponent);

	SetStaticMeshProperties(*LandingMeshComponent);
}

void AShotArc::SetStaticMeshProperties(UStaticMeshComponent& MeshComponent)
{
	MeshComponent.SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AShotArc::ClearPreviousState()
{
	HitLocation.Reset();
	ArcPoints.Reset();

	SplineComponent->ClearSplinePoints();

	// delete all existing static mesh component instances
	ClearSplineMeshes();

	LandingMeshComponent->SetVisibility(false);
}

void AShotArc::ClearSplineMeshes()
{
	for (auto SplineMeshComponent : SplineMeshes)
	{
		SplineMeshComponent->DestroyComponent();
	}

	SplineMeshes.Reset();
}

void AShotArc::SetData(const FPredictProjectilePathResult& Path, bool bDrawHit)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: SetData: NumPoints=%d; bDrawHit=%s"),
		*GetName(), Path.PathData.Num(), LoggingUtils::GetBoolString(bDrawHit));

	if (!ensureMsgf(SplineMesh, TEXT("Spline Mesh is not set")))
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Error, TEXT("%s: SetData: SplineMesh is not set"), *GetName());
		return;
	}

	ClearPreviousState();

	const auto& PathData = Path.PathData;
	if (PathData.IsEmpty())
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Warning, TEXT("%s: SetData: PathData is empty"), *GetName());
		return;
	}

	// Set actor location to be the first point
	SetActorLocation(PathData[0].Location);

	// All locations will then be relative to that first point
	CalculateArcPoints(Path, bDrawHit);
	BuildSpline();
	BuildMeshAlongSpline();

	if (ensureMsgf(LandingMeshComponent->GetStaticMesh(), TEXT("Landing Mesh is not set")))
	{
		UpdateLandingMesh();
	}
	else
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Warning, TEXT("%s: SetData: LandingMeshComponent is not set and landing indicator will not be drawn"), *GetName());
	}
}

void AShotArc::BeginPlay()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();
}

void AShotArc::CalculateArcPoints(const FPredictProjectilePathResult& Path, bool bDrawHit)
{
	ArcPoints.Reserve(Path.PathData.Num() + (bDrawHit ? 1 : 0));
	HitLocation.Reset();

	// Path data relative to actor location which is the first point
	const auto& ReferencePoint = GetActorLocation();

	for (const auto& PathDatum : Path.PathData)
	{
		ArcPoints.Add(PathDatum.Location - ReferencePoint);
	}

	if (bDrawHit)
	{
		HitLocation = Path.HitResult.ImpactPoint - ReferencePoint;
	}
}

void AShotArc::BuildSpline()
{
	for (const auto& ArcPoint : ArcPoints)
	{
		SplineComponent->AddSplinePoint(ArcPoint, ESplineCoordinateSpace::Local);
	}

	SplineComponent->UpdateSpline();
}

void AShotArc::BuildMeshAlongSpline()
{
	if (!ensureMsgf(SplineMesh, TEXT("Spline Mesh is not set")))
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Error, TEXT("%s: BuildMeshAlongSpline: SplineMesh is not set"), *GetName());
		return;
	}

	const auto NumPoints = SplineComponent->GetNumberOfSplinePoints();
	const auto NumSections = NumPoints - 1;

	if (NumSections <= 0)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: BuildMeshAlongSpline: %d is not enough points to build mesh"), *GetName(), NumPoints);
		return;
	}

	// Create instances for each section

	for (int32 Index = 0; Index < NumSections; ++Index)
	{
		const auto SplineMeshComponent = NewObject<USplineMeshComponent>(this);
		SplineMeshComponent->SetStaticMesh(SplineMesh);
		SplineMeshComponent->SetForwardAxis(MeshForwardAxis);
		SetStaticMeshProperties(*SplineMeshComponent);

		SplineMeshComponent->RegisterComponent();
		SplineMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

		const auto& StartLocation = SplineComponent->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::Local);
		const auto& StartTangent = SplineComponent->GetTangentAtSplinePoint(Index, ESplineCoordinateSpace::Local);

		// Not that if go beyond the last point, then the spline will use the last point
		const auto& EndLocation = SplineComponent->GetLocationAtSplinePoint(Index + 1, ESplineCoordinateSpace::Local);
		const auto& EndTangent = SplineComponent->GetTangentAtSplinePoint(Index + 1, ESplineCoordinateSpace::Local);

		SplineMeshComponent->SetStartAndEnd(
			StartLocation, StartTangent, 
			EndLocation, EndTangent
		);
	}
}

void AShotArc::UpdateLandingMesh()
{
	if (!HitLocation)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: UpdateLandingMesh: HitLocation not available - landing mesh will not be drawn"), *GetName());
		return;
	}

	LandingMeshComponent->SetRelativeLocation(*HitLocation);
	LandingMeshComponent->SetVisibility(true);
}
