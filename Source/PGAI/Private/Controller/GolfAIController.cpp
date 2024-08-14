// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Controller/GolfAIController.h"

#include "Components/GolfControllerCommonComponent.h"

#include "Pawn/PaperGolfPawn.h"

#include "State/GolfPlayerState.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"

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

	bCanFlick = false;

	DoBeginPlay([this](auto& GolfSubsystem)
	{
		GolfSubsystem.OnPaperGolfPawnClippedThroughWorld.AddDynamic(this, &ThisClass::OnFellThroughFloor);
	});

	InitDebugDraw();
}

void AGolfAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: EndPlay - EndPlayReason=%s"), *GetName(), *UEnum::GetValueAsString(EndPlayReason));

	Super::EndPlay(EndPlayReason);

	CleanupDebugDraw();
}

void AGolfAIController::Reset()
{
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: Reset"), *GetName());

	Super::Reset();

	ShotType = EShotType::Default;
	PlayerPawn = nullptr;

	bTurnActivated = false;
	bCanFlick = false;
	bOutOfBounds = false;
	bScored = false;

	check(GolfControllerCommonComponent);

	GolfControllerCommonComponent->Reset();
}

void AGolfAIController::MarkScored()
{
	bScored = true;
	GolfControllerCommonComponent->OnScored();

	OnScored();
	DestroyPawn();
}

bool AGolfAIController::IsReadyForNextShot() const
{
	if (bCanFlick)
	{
		UE_VLOG_UELOG(this, LogPGAI, VeryVerbose, TEXT("%s: IsReadyForNextShot - FALSE - bCanFlick is TRUE"),
			*GetName());

		return false;
	}

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

		return false;
	}

	return true;
}

void AGolfAIController::ActivateTurn()
{
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: ActivateTurn"), *GetName());

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

	GolfControllerCommonComponent->BeginTurn();

	// Turn must be activated before Setup can happen as it requires the player to be active
	bTurnActivated = true;
	SetupNextShot(true);

	PaperGolfPawn->SetReadyForShot(true);
}

void AGolfAIController::Spectate(APaperGolfPawn* InPawn, AGolfPlayerState* InPlayerState)
{
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: Spectate - InPawn=%s; InPlayerState=%s"),
		*GetName(), *LoggingUtils::GetName(InPawn), InPlayerState ? *InPlayerState->GetPlayerName() : TEXT("NULL"));

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

	// Cancel any outstanding execute timers
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);

	bCanFlick = false;

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
	if(!bCanFlick)
	{
		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: ExecuteTurn - bCanFlick is FALSE - Skipping"), *GetName());
		return;
	}

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

	bCanFlick = false;
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
		UE_VLOG_UELOG(this, LogPGAI, Error, TEXT("%s: CalculateFlickParams - FocusActor is NULL"), *GetName());
		return {};
	}

	UE_VLOG_LOCATION(GetOwner(), LogPGAI, Log, FocusActor->GetActorLocation(), 10.0f, FColor::Green, TEXT("Target"));

	// TODO: Refine logic - this is a very basic implementation
	// Try to hit at approx 45 degree angle
	// TODO: Technically this should be done outside this const function as it has a side effect
	PlayerPawn->AddActorLocalRotation(FRotator(45, 0, 0));

	auto World = GetWorld();
	if (!ensure(World))
	{
		return {};
	}

	// Hitting at 45 degrees so can simplify the projectile calculation
	// https://en.wikipedia.org/wiki/Projectile_motion

	FFlickParams FlickParams;
	FlickParams.ShotType = ShotType;

	const auto FlickLocation = PlayerPawn->GetFlickLocation(FlickParams.LocalZOffset);
	const auto RawFlickMaxForce = PlayerPawn->GetFlickMaxForce(FlickParams.ShotType);
	// Account for drag
	//const auto FlickMaxForce = PlayerPawn->GetFlickDragForceMultiplier(RawFlickMaxForce) * RawFlickMaxForce;
	const auto FlickMaxForce = RawFlickMaxForce;
	const auto FlickMaxSpeed = FlickMaxForce / PlayerPawn->GetMass();

	// See https://en.wikipedia.org/wiki/Range_of_a_projectile
	// Using wolfram alpha to solve the equation when theta is 45 for v, we get
	// v = d * sqrt(g) / sqrt(d + y)
	// where d is the horizontal distance XY and y is the vertical distance Z and v is vxy
	const auto& FocusActorLocation = FocusActor->GetActorLocation();

	const auto PositionDelta = FocusActorLocation - FlickLocation;
	const auto HorizontalDistance = PositionDelta.Size2D();
	const auto VerticalDistance = PositionDelta.Z;
	const auto DistanceSum = HorizontalDistance + VerticalDistance;

	float PowerFraction;

	// if TotalHorizontalDistance + VerticalDistance <= 0 then we use minimum power as so far above the target
	if (DistanceSum <= 0)
	{
		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: CalculateFlickParams - Way above target, using min force. HorizontalDistance=%.1fm; VerticalDistance=%.1fm"),
			*GetName(), HorizontalDistance / 100, VerticalDistance / 100);

		PowerFraction = MinShotPower;
	}
	else
	{
		const auto Gravity = FMath::Abs(World->GetGravityZ());
		const auto Speed = HorizontalDistance * FMath::Sqrt(Gravity) / FMath::Sqrt(DistanceSum);

		// Compare speed to max flick speed
		// Impulse is proportional to sqrt of the distance ratio if overshoot

		if (Speed >= FlickMaxSpeed)
		{
			UE_VLOG_UELOG(this, LogPGAI, Log, 
				TEXT("%s: CalculateFlickParams - Coming up short hit full power - Speed=%.1f; FlickMaxSpeed=%.1f, HorizontalDistance=%.1fm; VerticalDistance=%.1fm"),
				*GetName(), Speed, FlickMaxSpeed, HorizontalDistance / 100, VerticalDistance / 100);

			PowerFraction = 1.0f;
		}
		else
		{
			// also account for bounce distance
			const auto ReductionRatio = Speed / FlickMaxSpeed * (1 - BounceOverhitCorrectionFactor);

			// Impulse is proportional to sqrt of the velocity ratio
			const auto RawPowerFraction = FMath::Sqrt(ReductionRatio);
			PowerFraction = FMath::Max(MinShotPower, RawPowerFraction);

			UE_VLOG_UELOG(this, LogPGAI, Log,
				TEXT("%s: CalculateFlickParams - Overhit - Speed=%.1f; FlickMaxSpeed=%.1f, HorizontalDistance=%.1fm; VerticalDistance=%.1fm; RawPowerFraction=%.2f; PowerFraction=%.2f"),
				*GetName(), Speed, FlickMaxSpeed, HorizontalDistance / 100, VerticalDistance / 100, RawPowerFraction, PowerFraction);
		}
	}

	// Account for drag by increasing the power
	const auto FlickDragForceMultiplier = PlayerPawn->GetFlickDragForceMultiplier(PowerFraction * FlickMaxForce);
	PowerFraction = FMath::Clamp(PowerFraction / FlickDragForceMultiplier, 0.0f, 1.0f);

	// Add error to power calculation and accuracy
	FlickParams.PowerFraction = GeneratePowerFraction(PowerFraction);
	FlickParams.Accuracy = GenerateAccuracy();

	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: CalculateFlickParams - ShotType=%s; Power=%.2f; Accuracy=%.2f; ZOffset=%.1f; FlickDragForceMultiplier=%1.f"),
		*GetName(), *LoggingUtils::GetName(FlickParams.ShotType), FlickParams.PowerFraction, FlickParams.Accuracy, FlickParams.LocalZOffset, FlickDragForceMultiplier);

	return FlickParams;
}

float AGolfAIController::GenerateAccuracy() const
{
	return FMath::FRandRange(-AccuracyDeviation, AccuracyDeviation);
}

float AGolfAIController::GeneratePowerFraction(float InPowerFraction) const
{
	return FMath::Clamp(InPowerFraction * (1 + FMath::RandRange(-PowerDeviation, PowerDeviation)), MinShotPower, 1.0f);
}

FFlickParams AGolfAIController::CalculateDefaultFlickParams() const
{
	const auto Box = PG::CollisionUtils::GetAABB(*PlayerPawn);
	const auto ZExtent = Box.GetExtent().Z;

	// Hit full power
	const FFlickParams FlickParams =
	{
		.ShotType = ShotType,
		.LocalZOffset = 0,
		.PowerFraction = 1.0f,
		.Accuracy = GenerateAccuracy()
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
	DetermineShotType();
	
	if (ensure(PlayerPawn))
	{
		PlayerPawn->SnapToGround();
	}

	if (bSetCanFlick)
	{
		bCanFlick = true;
		const auto ShotDelayTime = FMath::FRandRange(MinFlickReactionTime, MaxFlickReactionTime);

		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: SetupNextShot - Shooting after %.1fs"), *GetName(), ShotDelayTime);

		GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &AGolfAIController::ExecuteTurn, ShotDelayTime, false);
	}
}

void AGolfAIController::ResetShot()
{
	ShotType = EShotType::Default;

	GolfControllerCommonComponent->ResetShot();
}

void AGolfAIController::DetermineShotType()
{
	GolfControllerCommonComponent->SetPaperGolfPawnAimFocus();

	const auto NewShotType = GolfControllerCommonComponent->DetermineShotType(EShotFocusType::Focus);

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

#pragma region Visual Logger
#if ENABLE_VISUAL_LOG
void AGolfAIController::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	Super::GrabDebugSnapshot(Snapshot);

	FVisualLogStatusCategory& Category = Snapshot->Status[0];

	Category.Add(TEXT("TurnActivated"), LoggingUtils::GetBoolString(bTurnActivated));
	Category.Add(TEXT("CanFlick"), LoggingUtils::GetBoolString(bCanFlick));
	Category.Add(TEXT("OutOfBounds"), LoggingUtils::GetBoolString(bOutOfBounds));
	Category.Add(TEXT("Scored"), LoggingUtils::GetBoolString(bScored));
	Category.Add(TEXT("ShotType"), LoggingUtils::GetName(ShotType));
	Category.Add(TEXT("TurnTimer"), LoggingUtils::GetBoolString(TurnTimerHandle.IsValid()));

	// TODO: Consider moving logic to PlayerState itself
	if (auto GolfPlayerState = GetGolfPlayerState(); GolfPlayerState)
	{
		FVisualLogStatusCategory PlayerStateCategory;
		PlayerStateCategory.Category = FString::Printf(TEXT("PlayerState"));

		PlayerStateCategory.Add(TEXT("Name"), FString::Printf(TEXT("%d"), *GolfPlayerState->GetPlayerName()));
		PlayerStateCategory.Add(TEXT("Shots"), FString::Printf(TEXT("%d"), GolfPlayerState->GetShots()));
		PlayerStateCategory.Add(TEXT("TotalShots"), FString::Printf(TEXT("%d"), GolfPlayerState->GetTotalShots()));
		PlayerStateCategory.Add(TEXT("IsReadyForShot"), LoggingUtils::GetBoolString(GolfPlayerState->IsReadyForShot()));

		Category.AddChild(PlayerStateCategory);
	}
}
void AGolfAIController::InitDebugDraw()
{
	// Ensure that state logged regularly so we see the updates in the visual logger

	FTimerDelegate DebugDrawDelegate = FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UE_VLOG(this, LogPGAI, Log, TEXT("Get Player State"));
	});

	GetWorldTimerManager().SetTimer(VisualLoggerTimer, DebugDrawDelegate, 0.05f, true);
}

void AGolfAIController::CleanupDebugDraw()
{
	GetWorldTimerManager().ClearTimer(VisualLoggerTimer);
}

#else

void AGolfPlayerController::InitDebugDraw() {}
void AGolfPlayerController::CleanupDebugDraw() {}

#endif

#pragma endregion Visual Logger
