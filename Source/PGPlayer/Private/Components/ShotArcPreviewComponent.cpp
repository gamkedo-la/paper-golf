// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/ShotArcPreviewComponent.h"

#include "Pawn/PaperGolfPawn.h"

#include "Library/PaperGolfPawnUtilities.h"

#include "PGPlayerLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Utils/ActorComponentUtils.h"

#include "Utils/CollisionUtils.h"

#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShotArcPreviewComponent)


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

	const FFlickPredictParams PredictParams
	{
		.MaxSimTime = MaxSimTime,
		.SimFrequency = SimFrequency,
		.CollisionRadius = CollisionRadius
	};

	FPredictProjectilePathResult Result;

	bLastPointIsHit = Pawn.PredictFlick(FlickParams, PredictParams, Result);
	LastCalculatedTransform = Pawn.GetActorTransform();

	ArcPoints.Reserve(Result.PathData.Num() + (bLastPointIsHit ? 1 : 0));

	for (const auto& PathDatum : Result.PathData)
	{
		ArcPoints.Add(PathDatum.Location);
	}

	if (bLastPointIsHit)
	{
		ArcPoints.Add(Result.HitResult.ImpactPoint);
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
