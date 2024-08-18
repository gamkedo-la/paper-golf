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

#include "Components/TextRenderComponent.h"

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
	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: BeginPlay"), *GetName());
	Super::BeginPlay();
}

void UShotArcPreviewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: EndPlay"), *GetName());

	Super::EndPlay(EndPlayReason);

	UnregisterPowerText();
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
	return ShotType != FlickParams.ShotType ||
		   !FMath::IsNearlyEqual(LocalZOffset, FlickParams.LocalZOffset) ||
		   !FMath::IsNearlyEqual(PowerFraction, FlickParams.PowerFraction) ||
		   !Pawn.GetActorTransform().EqualsNoScale(LastCalculatedTransform);
}

void UShotArcPreviewComponent::HidePowerText()
{
	if (!PowerText)
	{
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: HidePowerText"), *GetName());

	PowerText->SetVisibility(false);
}

void UShotArcPreviewComponent::ShowPowerText()
{
	if (!ensure(PowerText))
	{
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: ShowPowerText: %s"), *GetName(), *PowerText->Text.ToString());

	PowerText->SetVisibility(true);
}

void UShotArcPreviewComponent::RegisterPowerText(const APaperGolfPawn& Pawn)
{
	auto NewParent = Pawn.GetRootComponent();

	// Seems need to create it with the right parent component from the start; otherwise, it doesn't show up
	if (PowerText)
	{
		if (PowerText->GetAttachmentRoot() == NewParent)
		{
			return;
		}

		UnregisterPowerText();
	}

	PowerText = NewObject<UTextRenderComponent>(const_cast<APaperGolfPawn*>(&Pawn));
	PowerText->SetIsReplicated(false);
	PowerText->RegisterComponent();
	PowerText->SetTextRenderColor(ShotPowerColor.ToFColor(true));
	PowerText->SetWorldSize(ShotPowerTextSize);
	PowerText->AttachToComponent(NewParent, FAttachmentTransformRules::KeepRelativeTransform);
}

void UShotArcPreviewComponent::UnregisterPowerText()
{
	if (!PowerText)
	{
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: UnregisterPowerText"), *GetName());

	PowerText->DetachFromComponent( FDetachmentTransformRules { EDetachmentRule::KeepRelative, false });
	PowerText->UnregisterComponent();
	PowerText = nullptr;
}

TOptional<FVector> UShotArcPreviewComponent::GetCameraLocation(const APaperGolfPawn& Pawn) const
{
	if (auto PlayerController = Cast<APlayerController>(Pawn.GetController()); PlayerController && PlayerController->PlayerCameraManager)
	{
		return PlayerController->PlayerCameraManager->GetCameraLocation();
	}

	return {};
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

	ShotType = FlickParams.ShotType;
	LocalZOffset = FlickParams.LocalZOffset;
	PowerFraction = FlickParams.PowerFraction;

	UpdatePowerText(Pawn, Result);
}

FVector UShotArcPreviewComponent::GetPowerFractionTextLocation(const APaperGolfPawn& Pawn, const FPredictProjectilePathResult& PredictResult) const
{
	const auto& PathData = PredictResult.PathData;
	const auto& PawnLocation = Pawn.GetActorLocation();

	const auto CameraZ = [&]()
	{
		if (auto CameraLocationOptional = GetCameraLocation(Pawn); CameraLocationOptional)
		{
			return CameraLocationOptional->Z;
		}
		
		return PawnLocation.Z + ShotPowerZMinLocationOffset;
	}();

	if (PathData.IsEmpty())
	{
		// Set Z height to be PawnLocation + fixed value
		return FVector{ PawnLocation.X, PawnLocation.Y, CameraZ };
	}

	// Place at desired index with Z offseted
	const auto Index = FMath::Min(ShotPowerDesiredTextPointIndex, PathData.Num() - 1);
	FVector Location = PathData[Index].Location;
	Location.Z = CameraZ;

	return Location;
}

void UShotArcPreviewComponent::UpdatePowerText(const APaperGolfPawn& Pawn, const FPredictProjectilePathResult& PredictResult)
{
	RegisterPowerText(Pawn);

	const auto Location = GetPowerFractionTextLocation(Pawn, PredictResult);

	// Face Camera
	if (auto CameraLocationOptional = GetCameraLocation(Pawn); CameraLocationOptional)
	{
		// This is backwards from what is expected but doing it to Location makes the text backwards
		const auto ToCamera = *CameraLocationOptional - Location;
		PowerText->SetWorldRotation(ToCamera.ToOrientationRotator());
	}

	PowerText->SetWorldLocation(Location);
	PowerText->SetText(FText::FromString(FString::Printf(TEXT("%d %%"), FMath::RoundToInt32(PowerFraction * 100))));
}

void UShotArcPreviewComponent::DoShowShotArc()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: DoShowShotArc"), *GetName());

	bVisible = true;
	SetComponentTickEnabled(true);

	ShowPowerText();
}

void UShotArcPreviewComponent::HideShotArc()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: HideShotArc"), *GetName());

	bVisible = false;
	SetComponentTickEnabled(false);

	HidePowerText();
}
