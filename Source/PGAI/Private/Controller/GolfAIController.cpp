// Copyright Game Salutes. All Rights Reserved.


#include "Controller/GolfAIController.h"

#include "Components/GolfControllerCommonComponent.h"

#include "Pawn/PaperGolfPawn.h"

#include "State/GolfPlayerState.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"

#include "Kismet/GameplayStatics.h"

#include "Utils/CollisionUtils.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include "PGAILogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfAIController)

AGolfAIController::AGolfAIController()
{
	bWantsPlayerState = true;

	GolfControllerCommonComponent = CreateDefaultSubobject<UGolfControllerCommonComponent>(TEXT("GolfControllerCommon"));
}

void AGolfAIController::BeginPlay()
{
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();

	DoBeginPlay([this](auto& GolfSubsystem)
		{
			GolfSubsystem.OnPaperGolfPawnClippedThroughWorld.AddDynamic(this, &ThisClass::OnFellThroughFloor);
		});
}

void AGolfAIController::MarkScored()
{
	bScored = true;
	OnScored();
	DestroyPawn();
}

bool AGolfAIController::IsReadyForNextShot() const
{
	if (!IsActivePlayer())
	{
		UE_VLOG_UELOG(this, LogPGAI, VeryVerbose,
			TEXT("%s: IsReadyForNextShot - Skip - Not active player"),
			*GetName());
		return false;
	}
	if (HasScored())
	{
		UE_VLOG_UELOG(this, LogPGAI, VeryVerbose,
			TEXT("%s: IsReadyForNextShot - Skip - Scored"),
			*GetName());
		return false;
	}

	if (bOutOfBounds)
	{
		UE_VLOG_UELOG(this, LogPGAI, VeryVerbose,
			TEXT("%s: IsReadyForNextShot - Skip - Out of Bounds"),
			*GetName());
		return false;
	}

	const auto PaperGolfPawn = GetPaperGolfPawn();

	if (!PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGAI, VeryVerbose, TEXT("%s: IsReadyForNextShot - FALSE - PaperGolfPawn is NULL"),
			*GetName());

		return false;
	}

	const bool bAtRest = PaperGolfPawn->IsAtRest();

	if (!bAtRest)
	{
		UE_VLOG_UELOG(this, LogPGAI, VeryVerbose,
			TEXT("%s: IsReadyForNextShot - Skip - Not at rest"),
			*GetName());
	}

	return true;
}

void AGolfAIController::ActivateTurn()
{
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: ActivateTurn"), *GetName());

	GolfControllerCommonComponent->BeginTurn();

	if (bTurnActivated)
	{
		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: DoActivateTurn - Turn already activated"), *GetName());
		return;
	}

	const auto PaperGolfPawn = Cast<APaperGolfPawn>(GetPawn());

	// Activate the turn if switching from spectator to player
	// Doing this here as this function is called when replication of the pawn possession completes
	if (!PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: ActivateTurn - Skipping turn activation as Pawn=%s is not APaperGolfPawn"),
			*GetName(), *LoggingUtils::GetName(GetPawn()));
		return;
	}

	if (!PaperGolfPawn->IsAtRest())
	{
		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: ActivateTurn - Resetting shot state as paper golf pawn is not at rest"),
			*GetName(), *LoggingUtils::GetName(PaperGolfPawn));
		// Force reset of physics state to avoid triggering assertion
		PaperGolfPawn->SetUpForNextShot();
	}

	// Turn must be activated before Setup can happen as it requires the player to be active
	bTurnActivated = true;
	SetupNextShot(true);

	PaperGolfPawn->SetReadyForShot(true);
}

void AGolfAIController::Spectate(APaperGolfPawn* InPawn)
{
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: Spectate - %s"), *GetName(), *LoggingUtils::GetName(InPawn));

	if (IsValid(PlayerPawn))
	{
		PlayerPawn->SetCollisionEnabled(false);
	}

	bTurnActivated = false;
	GolfControllerCommonComponent->EndTurn();
}

bool AGolfAIController::HandleOutOfBounds()
{
	if (bOutOfBounds)
	{
		return false;
	}

	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: HandleOutOfBounds"), *GetName());

	bOutOfBounds = true;

	// Make sure we don't process this is as a normal shot 
	GolfControllerCommonComponent->EndTurn();

	if (auto World = GetWorld(); ensure(World))
	{
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, this, &ThisClass::ResetShotAfterOutOfBounds, OutOfBoundsDelayTime);
	}

	return true;
}

APaperGolfPawn* AGolfAIController::GetPaperGolfPawn()
{
	if (IsValid(PlayerPawn))
	{
		return PlayerPawn;
	}

	const auto PaperGolfPawn = Cast<APaperGolfPawn>(GetPawn());

	if (!ensureMsgf(PaperGolfPawn, TEXT("%s: GetPaperGolfPawn - %s is not a APaperGolfPawn"), *GetName(), *LoggingUtils::GetName(GetPawn())))
	{
		UE_VLOG_UELOG(this, LogPGAI, Error, TEXT("%s: GetPaperGolfPawn - %s is not a APaperGolfPawn"), *GetName(), *LoggingUtils::GetName(GetPawn()));
		return nullptr;
	}

	return PaperGolfPawn;
}

void AGolfAIController::SetPawn(APawn* InPawn)
{
	// Note that this is also called on server from game mode when RestartPlayer is called
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: SetPawn - InPawn=%s"), *GetName(), *LoggingUtils::GetName(InPawn));

	Super::SetPawn(InPawn);

	const auto PaperGolfPawn = Cast<APaperGolfPawn>(InPawn);

	// The player pawn will be first paper golf pawn possessed so set if not already set
	if (!IsValid(PlayerPawn) && IsValid(PaperGolfPawn))
	{
		UE_VLOG_UELOG(this, LogPGAI, Display, TEXT("%s: SetPawn - PlayerPawn=%s"), *GetName(), *LoggingUtils::GetName(InPawn));
		PlayerPawn = PaperGolfPawn;
	}

	if (IsValid(PaperGolfPawn))
	{
		ActivateTurn();
	}
}

void AGolfAIController::OnFellThroughFloor(APaperGolfPawn* InPaperGolfPawn)
{
	DoOnFellThroughFloor(InPaperGolfPawn);
}

void AGolfAIController::OnScored()
{
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s-%s: OnScored - %s"),
		*GetName(), *LoggingUtils::GetName(GetPawn()), LoggingUtils::GetBoolString(bScored));

	// Nothing to do if scored is false
	if (!bScored)
	{
		return;
	}

	auto GolfPlayerState = GetGolfPlayerState();
	if (!ensure(GolfPlayerState))
	{
		return;
	}

	GolfPlayerState->SetReadyForShot(false);

	// GetShots may not necessary be accurate if the Shots variable replication happens after the score replication;
	// however, it is only for logging purposes
	UE_VLOG_UELOG(this, LogPGAI, Display, TEXT("%s-%s: OnScored: NumStrokes=%d"),
		*GetName(), *LoggingUtils::GetName(GetPawn()), GolfPlayerState->GetShots());
}

void AGolfAIController::ExecuteTurn()
{
	auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	AddStroke();

	auto FlickParams = CalculateFlickParams();
	if (!FlickParams)
	{
		FlickParams = CalculateDefaultFlickParams();
	}

	check(FlickParams);

	PaperGolfPawn->Flick(*FlickParams);

	if (auto GolfPlayerState = GetGolfPlayerState(); ensure(GolfPlayerState))
	{
		GolfPlayerState->SetReadyForShot(false);
	}
}

TOptional<FFlickParams> AGolfAIController::CalculateFlickParams() const
{
	if (!ensure(PlayerPawn))
	{
		return {};
	}

	// Predict flick to focus actor
	auto FocusActor = PlayerPawn->GetFocusActor();
	if (!ensure(FocusActor))
	{
		UE_VLOG_UELOG(this, LogPGAI, Error, TEXT("%s: ExecuteTurn - FocusActor is NULL"), *GetName());
		return {};
	}

	UE_VLOG_LOCATION(GetOwner(), LogPGAI, Log, FocusActor->GetActorLocation(), 10.0f, FColor::Green, TEXT("Target"));

	// TODO: Refine logic - this is a very basic implementation
	// Try to hit at approx 45 degree angle

	const auto Box = PG::CollisionUtils::GetAABB(*PlayerPawn);
	const auto ZExtent = Box.GetExtent().Z;

	FPredictProjectilePathResult Result;

	FFlickParams FlickParams;
	FlickParams.ShotType = ShotType;
	FlickParams.LocalZOffset = static_cast<float>(-ZExtent * 0.5);

	const bool bHit = PlayerPawn->PredictFlick(FlickParams, {}, Result);

	if(!bHit)
	{
		UE_VLOG_UELOG(this, LogPGAI, Warning, TEXT("%s: ExecuteTurn - No hit found for FocusActor=%s"), *GetName(), *FocusActor->GetName());
		return {};
	}

	// determine overshoot via dot product to target
	const auto& HitLocation = Result.HitResult.ImpactPoint;

	const auto& CurrentLocation = PlayerPawn->GetActorLocation();
	const auto ToHitLocation = HitLocation - CurrentLocation;
	const auto FromHitToFocusActor = FocusActor->GetActorLocation() - HitLocation;

	float PowerFraction;

	if ((ToHitLocation | FromHitToFocusActor) > 0)
	{
		// coming up short - hit full power
		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: ExecuteTurn - DotProduct > 0 - coming up short hit full power"), *GetName());
		PowerFraction = 1.0f;
	}
	else
	{
		const auto OverhitDistance = FromHitToFocusActor.Size();
		const auto TargetDistance = ToHitLocation.Size();

		const auto RelativeOverhit = OverhitDistance / TargetDistance + BounceOverhitCorrectionFactor;

		// Impulse is proportional to sqrt of the distance ratio if overshoot

		const auto ForceReduction = FMath::Sqrt(RelativeOverhit);
		PowerFraction = 1 - ForceReduction;

		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: ExecuteTurn - DotProduct <= 0 - Over hit by %fm to distance=%fm; RelativeOverhit=%.2f; ForceReduction=%.2f"),
			*GetName(), OverhitDistance / 100, TargetDistance / 100, RelativeOverhit, ForceReduction);
	}

	const auto Accuracy = FMath::FRandRange(-AccuracyDeviation, AccuracyDeviation);

	FlickParams.PowerFraction = PowerFraction;
	FlickParams.Accuracy = Accuracy;

	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: ExecuteTurn - ShotType=%s; Power=%.2f; Accuracy=%.2f; ZOffset=%.1f"),
		*GetName(), *LoggingUtils::GetName(FlickParams.ShotType), FlickParams.PowerFraction, FlickParams.Accuracy, FlickParams.LocalZOffset);

	return FlickParams;
}

FFlickParams AGolfAIController::CalculateDefaultFlickParams() const
{
	const auto Box = PG::CollisionUtils::GetAABB(*PlayerPawn);
	const auto ZExtent = Box.GetExtent().Z;

	// Hit full power
	const FFlickParams FlickParams =
	{
		.ShotType = ShotType,
		.LocalZOffset = static_cast<float>(-ZExtent * 0.5),
		.PowerFraction = 1.0f,
		.Accuracy = FMath::FRandRange(-AccuracyDeviation, AccuracyDeviation)
	};

	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: CalculateDefaultFlickParams - ShotType=%s; Power=%.2f; Accuracy=%.2f; ZOffset=%.1f"),
		*GetName(), *LoggingUtils::GetName(FlickParams.ShotType), FlickParams.PowerFraction, FlickParams.Accuracy, FlickParams.LocalZOffset);

	return FlickParams;
}

void AGolfAIController::DoAdditionalOnShotFinished()
{
	bTurnActivated = false;
}

void AGolfAIController::DoAdditionalFallThroughFloor()
{
	// Nothing to do
}

void AGolfAIController::DestroyPawn()
{
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: DestroyPawn: %s"), *GetName(), *LoggingUtils::GetName(PlayerPawn));

	if (!IsValid(PlayerPawn))
	{
		UE_VLOG_UELOG(this, LogPGAI, Warning, TEXT("%s: DestroyPawn - PlayerPawn is NULL"), *GetName());
		return;
	}

	GolfControllerCommonComponent->DestroyPawn();

	PlayerPawn = nullptr;
}

void AGolfAIController::SetupNextShot(bool bSetCanFlick)
{
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: SetupNextShot: bSetCanFlick=%s"), *GetName(), LoggingUtils::GetBoolString(bSetCanFlick));

	if (!GolfControllerCommonComponent->SetupNextShot(bSetCanFlick))
	{
		return;
	}

	ResetShot();
	GolfControllerCommonComponent->SetPaperGolfPawnAimFocus();
	
	if (ensure(PlayerPawn))
	{
		PlayerPawn->SnapToGround();
	}

	if (bSetCanFlick)
	{
		const auto ShotDelayTime = FMath::FRandRange(MinFlickReactionTime, MaxFlickReactionTime);

		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: SetupNextShot - Shooting after %.1fs"), *GetName(), ShotDelayTime);

		GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &AGolfAIController::ExecuteTurn, ShotDelayTime, false);
	}
}

void AGolfAIController::ResetShot()
{
	ShotType = EShotType::Default;

	GolfControllerCommonComponent->ResetShot();

	DetermineShotType();
}

void AGolfAIController::DetermineShotType()
{
	const auto NewShotType = GolfControllerCommonComponent->DetermineShotType();

	if (NewShotType != EShotType::Default)
	{
		ShotType = NewShotType;
	}
}

void AGolfAIController::ResetShotAfterOutOfBounds()
{
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: ResetShotAfterOutOfBounds"), *GetName());

	auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	PaperGolfPawn->SetUpForNextShot();

	bOutOfBounds = false;
	bTurnActivated = false;

	const auto& LastShotOptional = GolfControllerCommonComponent->GetLastShot();

	// Set location to last in shot history
	if (ensureMsgf(LastShotOptional, TEXT("%s-%s: ResetShotAfterOutOfBounds - ShotHistory is empty"),
		*GetName(), *PaperGolfPawn->GetName()))
	{
		const auto& ResetPosition = LastShotOptional->Position;
		SetPositionTo(ResetPosition);
		ActivateTurn();

		PaperGolfPawn->MulticastReliableSetTransform(ResetPosition, true, PaperGolfPawn->GetActorRotation());
	}
}

void AGolfAIController::SetPositionTo(const FVector& Position, const TOptional<FRotator>& OptionalRotation)
{
	UE_VLOG_UELOG(this, LogPGAI, Log,
		TEXT("%s: SetPositionTo - Position=%s; Rotation=%s"),
		*GetName(), *Position.ToCompactString(), *PG::StringUtils::ToString(OptionalRotation));

	GolfControllerCommonComponent->SetPositionTo(Position, OptionalRotation);
	SetupNextShot(false);
}
