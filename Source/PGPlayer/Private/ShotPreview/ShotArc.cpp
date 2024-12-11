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
	bReplicates = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	SetRootComponent(SplineComponent);

	LandingMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LandingMesh"));
	LandingMeshComponent->SetupAttachment(RootComponent);

	SetStaticMeshProperties(*LandingMeshComponent);
}

void AShotArc::SetStaticMeshProperties(UStaticMeshComponent& MeshComponent)
{
	MeshComponent.SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent.SetMobility(EComponentMobility::Movable);
}

void AShotArc::ClearPreviousState()
{
	LandingData.Reset();
	ArcPoints.Reset();

	SplineComponent->ClearSplinePoints();
	ClearSplineMeshes();
	LandingMeshComponent->SetVisibility(false);
}

float AShotArc::CalculateMeshSize() const
{
	check(SplineMesh);
	
	if(SplineMeshSize > 0)
	{
		return SplineMeshSize;
	}

	const auto& BoundsSize = SplineMesh->GetBounds().BoxExtent * 2;

	float CalculatedSize{};
	
	switch(MeshForwardAxis)
	{
		case ESplineMeshAxis::X: CalculatedSize = BoundsSize.X; break;
		case ESplineMeshAxis::Y: CalculatedSize = BoundsSize.Y; break;
		case ESplineMeshAxis::Z: CalculatedSize = BoundsSize.Z; break;
		default: checkNoEntry();
	}

	UE_CVLOG_UELOG(CalculatedSize <= 0, this, LogPGPlayer, Warning, TEXT("%s: CalculateMeshSize: Unable to calculate mesh size of %s"),
		*GetName(), *SplineMesh->GetName());
	UE_CVLOG_UELOG(CalculatedSize > 0, this, LogPGPlayer, Log, TEXT("%s: CalculateMeshSize: Calculated mesh size of %f for %s"),
		*GetName(), CalculatedSize, *SplineMesh->GetName());

	return CalculatedSize;
}

void AShotArc::ClearSplineMeshes()
{
	// just set existing ones to hidden instead of destroying so they can be reused
	for (auto& Element : ArcElements)
	{
		if(IsValid(Element.SplineMesh))
		{
			Element.SplineMesh->SetVisibility(false);
		}
	}
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

	SplineMeshSize = CalculateMeshSize();

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

	// Path data relative to actor location which is the first point
	const auto& ReferencePoint = GetActorLocation();

	for (const auto& PathDatum : Path.PathData)
	{
		ArcPoints.Add(PathDatum.Location - ReferencePoint);
	}

	if (bDrawHit)
	{
		const auto LandingRelativeLocation = Path.HitResult.ImpactPoint - ReferencePoint;
		
		LandingData = FLandingInfo
		{
			.Location = LandingRelativeLocation,
			.Normal = Path.HitResult.ImpactNormal
		};
	}
}

void AShotArc::BuildSpline()
{
	for (const auto& ArcPoint : ArcPoints)
	{
		SplineComponent->AddSplinePoint(ArcPoint, ESplineCoordinateSpace::Local, false);
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

	const auto SplineLength = SplineComponent->GetSplineLength();

	const auto ActualMaxSplineMeshes = [&]()
	{
		if(SplineMeshSize <= 0)
		{
			return MaxSplineMeshes;
		}

		const auto CalculatedMax = FMath::CeilToInt(SplineLength / SplineMeshSize);
		return FMath::Min(CalculatedMax, MaxSplineMeshes);
	}();
	
	const auto NumPoints = SplineComponent->GetNumberOfSplinePoints();
	const auto NumSections = FMath::Min(ActualMaxSplineMeshes, NumPoints - 1);
	
	if (NumSections <= 0)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: BuildMeshAlongSpline: %d is not enough points to build mesh"), *GetName(), NumPoints);
		return;
	}

	const auto DistancePerSection = SplineLength / NumSections;
	
	UE_VLOG_UELOG(this, LogPGPlayer, Log,
	TEXT("%s: BuildMeshAlongSpline: NumPoints=%d; NumSections=%d; ActualMaxSplineMeshes=%d; MaxSplineMeshes=%d; MeshSize=%f; DistancePerSection=%f; SplineLength=%f"),
	*GetName(), NumPoints, NumSections, ActualMaxSplineMeshes, MaxSplineMeshes, SplineMeshSize, DistancePerSection, SplineLength);
	

	// Create instances for each section
	ArcElements.Reserve(NumSections);
	
	float TotalDistance{};
	
	for (int32 Index = 0; Index < NumSections; ++Index)
	{
		const auto SplineMeshComponent = GetOrCreateSplineComponent(Index);

		const auto CurrentDistance = TotalDistance;
		const auto NextDistance = TotalDistance + DistancePerSection;
		
		const auto& StartLocation = SplineComponent->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::Local);
		const auto& StartTangent = SplineComponent->GetTangentAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::Local);

		// Not that if go beyond the last point, then the spline will use the last point
		
		const auto& EndLocation = SplineComponent->GetLocationAtDistanceAlongSpline(NextDistance, ESplineCoordinateSpace::Local);
		const auto& EndTangent = SplineComponent->GetTangentAtDistanceAlongSpline(NextDistance, ESplineCoordinateSpace::Local);

		SplineMeshComponent->SetStartAndEnd(
			StartLocation, StartTangent, 
			EndLocation, EndTangent
		);

		TotalDistance = NextDistance;
	}
}

USplineMeshComponent* AShotArc::GetOrCreateSplineComponent(int32 Index)
{
	if(Index < ArcElements.Num())
	{
		if(auto SplineMeshComponent = ArcElements[Index].SplineMesh; IsValid(SplineMeshComponent))
		{
			SplineMeshComponent->SetVisibility(true);
			return SplineMeshComponent;
		}
	}

	const auto SplineMeshComponent = NewObject<USplineMeshComponent>(this);
	SplineMeshComponent->SetStaticMesh(SplineMesh);
	SplineMeshComponent->SetForwardAxis(MeshForwardAxis);
	SetStaticMeshProperties(*SplineMeshComponent);

	SplineMeshComponent->RegisterComponent();
	SplineMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	ArcElements.Add(FShotArcData {
		.SplineMesh = SplineMeshComponent
	});

	return SplineMeshComponent;
}

void AShotArc::UpdateLandingMesh()
{
	if (!LandingData)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: UpdateLandingMesh: LandingData not available - landing mesh will not be drawn"), *GetName());
		return;
	}

	auto Mesh = LandingMeshComponent->GetStaticMesh();
	check(Mesh);

	// determine bounds scaling
	if(LandingMeshSize <= 0)
	{
		const auto BoundingRadius = Mesh->GetBounds().GetSphere().W;
		LandingMeshSize = BoundingRadius;

		// Only need to do this the first time as the value doesn't change
		if(LandingMeshSize > 0)
		{
			auto Scale = LandingMeshComponent->GetComponentScale();

			const auto DesiredScalingFactor = HitRadiusSize / LandingMeshSize;
			Scale *= DesiredScalingFactor;

			LandingMeshComponent->SetWorldScale3D(Scale);

			UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: UpdateLandingMesh: Calculated landing mesh size of %f for %s; DesiredScalingFactor=%f; HitRadiusSize=%f; Scale=%s"),
				*GetName(), BoundingRadius, *Mesh->GetName(), DesiredScalingFactor, HitRadiusSize, *Scale.ToCompactString());
		}
		else
		{
			UE_VLOG_UELOG(this, LogPGPlayer, Warning, TEXT("%s: UpdateLandingMesh: Could not calculate landing mesh size for %s"),
				*GetName(), *Mesh->GetName());
		}
	}

	const auto& RelativeLocation = LandingData->Location;
	const auto DesiredOrientation = LandingData->Normal.ToOrientationRotator();
	const auto CurrentOrientation = LandingMeshUpVector.ToOrientationRotator();

	const auto DesiredWorldRotation = (DesiredOrientation - CurrentOrientation).GetNormalized();

	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: UpdateLandingMesh: RelativeLocation=%s; Normal=%s; DesiredOrientation=%s; CurrentOrientation=%s; DesiredWorldRotation=%s"),
		*GetName(), *LandingData->Location.ToCompactString(), *LandingData->Normal.ToCompactString(),
		*DesiredOrientation.ToCompactString(), *CurrentOrientation.ToCompactString(), *DesiredWorldRotation.ToCompactString());
	
	LandingMeshComponent->SetRelativeLocation(RelativeLocation);
	LandingMeshComponent->SetWorldRotation(DesiredWorldRotation);
	LandingMeshComponent->SetVisibility(true);
}
