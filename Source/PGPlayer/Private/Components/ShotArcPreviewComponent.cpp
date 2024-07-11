// Copyright Game Salutes. All Rights Reserved.


#include "Components/ShotArcPreviewComponent.h"

#include "Pawn/PaperGolfPawn.h"

#include "Library/PaperGolfPawnUtilities.h"

#include "PGPlayerLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Utils/ActorComponentUtils.h"

#include "Utils/CollisionUtils.h"

#include "Kismet/GameplayStatics.h"

UShotArcPreviewComponent::UShotArcPreviewComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UShotArcPreviewComponent::ShowShotArc(const FFlickParams& FlickParams)
{
	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: ShowShotArc"),
		*GetName());

	const auto Pawn = PG::ActorComponentUtils::GetOwnerPawn<const APaperGolfPawn>(*this);
	if (!Pawn)
	{
		// Already logged
		return;
	}

	if(NeedsToRecalculateArc(*Pawn, FlickParams))
	{
		CalculateShotArc(*Pawn, FlickParams);
	}

	DoShowShotArc();
}

void UShotArcPreviewComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UShotArcPreviewComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// we shouldn't be ticking if not visible
	if (!ensure(bVisible))
	{
		return;
	}

	for(int32 i = 0, Len = ArcPoints.Num(); const auto& Point : ArcPoints)
	{
		if (i == Len - 1 && bLastPointIsHit)
		{
			UPaperGolfPawnUtilities::DrawBox(this, Point, FColor::Red, FVector { HitRadiusSize, HitRadiusSize, 20.0f });
		}
		else
		{
			UPaperGolfPawnUtilities::DrawSphere(this, Point, FColor::Green, CollisionRadius, 6);
		}

		++i;
	}
}

bool UShotArcPreviewComponent::NeedsToRecalculateArc(const APaperGolfPawn& Pawn, const FFlickParams& FlickParams) const
{
	// Not using accuracy or power at moment, both are at 100%
	return ShotType != FlickParams.ShotType ||
		   !FMath::IsNearlyEqual(LocalZOffset, FlickParams.LocalZOffset) ||
		   !Pawn.GetActorTransform().EqualsNoScale(LastCalculatedTransform);
}

void UShotArcPreviewComponent::CalculateShotArc(const APaperGolfPawn& Pawn, const FFlickParams& FlickParams)
{
	ArcPoints.Reset();
	bLastPointIsHit = false;

	const auto& FlickDirection = Pawn.GetFlickDirection();
	const auto FlickForceMagnitude = Pawn.GetFlickMaxForce(FlickParams.ShotType);
	const auto FlickImpulse = FlickDirection * FlickForceMagnitude;

	FPredictProjectilePathParams Params;
	// Offset start location a bit so that we don't collide with walls so that edge of sphere is on the flick location
	Params.StartLocation = Pawn.GetFlickLocation(FlickParams.LocalZOffset) + FlickDirection * CollisionRadius;
	Params.ActorsToIgnore.Add(const_cast<APaperGolfPawn*>(&Pawn));
	Params.bTraceWithCollision = true;
	//Params.bTraceWithChannel = true;
	//Params.TraceChannel = ECollisionChannel::ECC_Visibility;
	Params.ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	Params.ProjectileRadius = CollisionRadius;
	Params.MaxSimTime = MaxSimTime;
	Params.SimFrequency = SimFrequency;

	// Impulse = change in momentum
	Params.LaunchVelocity = FlickImpulse / Pawn.GetMass();

	// TODO: Later toggle with console variable
	//Params.DrawDebugType = EDrawDebugTrace::ForDuration;
	//Params.DrawDebugTime = 10.0f;

	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: CalculateShotArc - Pawn=%s; FlickDirection=%s; FlickForceMagnitude=%.1f; StartLocation=%s; LaunchVelocity=%scm/s"),
		*GetName(),
		*Pawn.GetName(),
		*FlickDirection.ToCompactString(),
		FlickForceMagnitude,
		*Params.StartLocation.ToCompactString(),
		*Params.LaunchVelocity.ToCompactString());

	FPredictProjectilePathResult Result;

	const bool bHit = bLastPointIsHit = UGameplayStatics::PredictProjectilePath(
		GetWorld(),
		Params,
		Result);

	LastCalculatedTransform = Pawn.GetActorTransform();

	if (!bHit)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: CalculateShotArc - No hit found"), *GetName());
	}

	ArcPoints.Reserve(Result.PathData.Num());

	for (int32 i = 0; const auto& PathDatum : Result.PathData)
	{
		UE_VLOG_LOCATION(
			GetOwner(), LogPGPlayer, Verbose, PathDatum.Location, Params.ProjectileRadius, FColor::Green, TEXT("P%d"), i);
		++i;

		ArcPoints.Add(PathDatum.Location);
	}

	if (bLastPointIsHit)
	{
		ArcPoints.Add(Result.HitResult.ImpactPoint);
		UE_VLOG_BOX(GetOwner(), LogPGPlayer, Verbose, FBox::BuildAABB(Result.HitResult.ImpactPoint, FVector{ 10.0 }), FColor::Red, TEXT("Hit"));
	}

}

void UShotArcPreviewComponent::DoShowShotArc()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: DoShowShotArc"), *GetName());

	bVisible = true;
	SetComponentTickEnabled(true);
}

void UShotArcPreviewComponent::HideShotArc()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: HideShotArc"), *GetName());

	bVisible = false;
	SetComponentTickEnabled(false);
}
