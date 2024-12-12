// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/ShotArcPreviewComponent.h"

#include "Pawn/PaperGolfPawn.h"

#include "ShotPreview/ShotArc.h"

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
	PrimaryComponentTick.bCanEverTick = false;
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

	if (!ensureMsgf(ShotArcSpawnActorClass, TEXT("ShotArcSpawnActorClass not set")))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Error, TEXT("%s: ShotArcSpawnActorClass not set"), *GetName());
	}
}

void UShotArcPreviewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: EndPlay"), *GetName());

	UnregisterPowerText();
	DestroyShotArcActor();

	Super::EndPlay(EndPlayReason);
}

bool UShotArcPreviewComponent::NeedsToRecalculateArc(const APaperGolfPawn& Pawn, const FFlickParams& FlickParams) const
{
	return ShotType != FlickParams.ShotType ||
		   !FMath::IsNearlyEqual(LocalZOffset, FlickParams.LocalZOffset) ||
		   !FMath::IsNearlyEqual(PowerFraction, FlickParams.PowerFraction) ||
		   !Pawn.GetActorTransform().EqualsNoScale(LastCalculatedTransform);
}

void UShotArcPreviewComponent::HidePowerText() const
{
	if (!PowerText)
	{
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: HidePowerText"), *GetName());

	PowerText->SetVisibility(false);
}

void UShotArcPreviewComponent::ShowPowerText() const
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

	if (TextMaterial)
	{
		PowerText->SetMaterial(0, TextMaterial);
	}

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

const APlayerCameraManager* UShotArcPreviewComponent::GetCameraManager(const APaperGolfPawn& Pawn) const
{
	if (auto PlayerController = Cast<APlayerController>(Pawn.GetController()); PlayerController && PlayerController->PlayerCameraManager)
	{
		return PlayerController->PlayerCameraManager;
	}

	return nullptr;
}

TOptional<FVector> UShotArcPreviewComponent::GetCameraLocation(const APaperGolfPawn& Pawn) const
{
	if (auto CameraManager = GetCameraManager(Pawn); CameraManager)
	{
		return CameraManager->GetCameraLocation();
	}

	return {};
}

TOptional<FRotator> UShotArcPreviewComponent::GetCameraRotation(const APaperGolfPawn& Pawn) const
{
	if (auto CameraManager = GetCameraManager(Pawn); CameraManager)
	{
		return CameraManager->GetCameraRotation();
	}

	return {};
}

void UShotArcPreviewComponent::CalculateShotArc(const APaperGolfPawn& Pawn, const FFlickParams& FlickParams)
{
	const FFlickPredictParams PredictParams
	{
		.MaxSimTime = MaxSimTime,
		.SimFrequency = SimFrequency,
		.CollisionRadius = CollisionRadius
	};

	FPredictProjectilePathResult Result;

	const bool bLastPointIsHit = Pawn.PredictFlick(FlickParams, PredictParams, Result);
	LastCalculatedTransform = Pawn.GetActorTransform();

	ShotArc = SpawnShotArcActor(Pawn);
	if (!ShotArc)
	{
		return;
	}

	ShotArc->SetData(Result, bLastPointIsHit);

	ShotType = FlickParams.ShotType;
	LocalZOffset = FlickParams.LocalZOffset;
	PowerFraction = FlickParams.PowerFraction;

	UpdatePowerText(Pawn, Result);
}

AShotArc* UShotArcPreviewComponent::SpawnShotArcActor(const APaperGolfPawn& Pawn)
{
	if (!ShotArcSpawnActorClass)
	{
		return nullptr;
	}

	if (ShotArc)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: SpawnShotArcActor: Shot arc actor already exists, returning existing"), *GetName());
		return ShotArc;
	}

	auto World = GetWorld();
	if (!ensure(World))
	{
		return nullptr;
	}

	// Don't set a spawn transform and instead rely on the implementation to place the points at the
	// correct orientation

	const auto SpawnedArc = World->SpawnActorDeferred<AShotArc>(ShotArcSpawnActorClass, FTransform::Identity,
		GetOwner(), const_cast<APaperGolfPawn*>(&Pawn), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	ShotArc = Cast<AShotArc>(UGameplayStatics::FinishSpawningActor(SpawnedArc, FTransform::Identity));

	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: SpawnShotArcActor: %s -> %s"), *GetName(), *ShotArcSpawnActorClass->GetName(), *ShotArc->GetName());
	return ShotArc;
}

void UShotArcPreviewComponent::DestroyShotArcActor()
{
	if (ShotArc)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: DestroyShotArcActor: %s"), *GetName(), *ShotArc->GetName());
		ShotArc->Destroy();
		ShotArc = nullptr;
	}
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
		// Set to fixed value
		return FVector{ PawnLocation.X, PawnLocation.Y, CameraZ };
	}

	// Place at desired index with Z offseted
	const auto Index = FMath::Min(ShotPowerDesiredTextPointIndex, PathData.Num() - 1);
	FVector Location = PathData[Index].Location;
	Location.Z = CameraZ;

	if (const auto CameraRotationOptional = GetCameraRotation(Pawn); !FMath::IsNearlyZero(TextHorizontalOffset) && CameraRotationOptional)
	{
		// offset by camera right vector - rotation and up vector cross product
		const auto CameraRight = CameraRotationOptional->RotateVector(FVector::RightVector).GetSafeNormal2D();
		Location += CameraRight * TextHorizontalOffset;
	}

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

	if (ShotArc)
	{
		ShotArc->SetActorHiddenInGame(false);
	}

	ShowPowerText();
}

void UShotArcPreviewComponent::HideShotArc()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPlayer, Log, TEXT("%s: HideShotArc"), *GetName());

	bVisible = false;

	if (ShotArc)
	{
		ShotArc->SetActorHiddenInGame(true);
	}

	HidePowerText();
}
