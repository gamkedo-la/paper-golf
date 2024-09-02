// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Controller/GolfAIController.h"

#include "Components/GolfControllerCommonComponent.h"
#include "Components/GolfAIShotComponent.h"

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
	GolfAIShotComponent = CreateDefaultSubobject<UGolfAIShotComponent>(TEXT("GolfAIShot"));
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

	ClearTimers();
	CleanupDebugDraw();
}

void AGolfAIController::ClearTimers()
{
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: ClearTimers"), *GetName());

	GetWorldTimerManager().ClearTimer(TurnTimerHandle);

	ClearShotAnimationTimers();
}

void AGolfAIController::ClearShotAnimationTimers()
{
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: ClearShotAnimationTimers"), *GetName());

	auto& TimerManager = GetWorldTimerManager();

	TimerManager.ClearTimer(YawAnimationState.TimerHandle);
	TimerManager.ClearTimer(PitchAnimationState.TimerHandle);

	YawAnimationState = PitchAnimationState = {};
}

void AGolfAIController::Reset()
{
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: Reset"), *GetName());

	Super::Reset();

	ClearTimers();

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

	if (!ShotSetupParams)
	{
		UE_VLOG_UELOG(this, LogPGAI, Warning, TEXT("%s: ExecuteTurn - ShotSetupParams is not defined"), *GetName());
		return;
	}

	auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	AddStroke();

	PaperGolfPawn->Flick(ShotSetupParams->FlickParams);

	if (auto GolfPlayerState = GetGolfPlayerState(); ensure(GolfPlayerState))
	{
		GolfPlayerState->SetReadyForShot(false);
	}

	bCanFlick = false;
}

bool AGolfAIController::SetupShot()
{
	ShotSetupParams.Reset();

	if (!IsValid(PlayerPawn))
	{
		return false;
	}

	TArray<FShotFocusScores> FocusActorScores;
	GolfControllerCommonComponent->GetBestFocusActor(&FocusActorScores);

	const auto ShotSetupResult = GolfAIShotComponent->SetupShot(
		{
			.PlayerPawn = PlayerPawn,
			.PlayerState = GetGolfPlayerState(),
			.GolfHole = GolfControllerCommonComponent->GetCurrentGolfHole(),
			.FocusActorScores = std::move(FocusActorScores),
			.ShotType = ShotType
		}
	);

	PlayerPawn->SetFocusActor(ShotSetupResult.FocusActor);

	ShotType = ShotSetupResult.FlickParams.ShotType;
	ShotSetupParams = ShotSetupResult;

	return true;
}

void AGolfAIController::InterpolateShotSetup(float ShootDeltaTime)
{
	check(ShotSetupParams);

	ClearShotAnimationTimers();

	auto World = GetWorld();

	if (!ensure(World) || !IsValid(PlayerPawn))
	{
		return;
	}

	const auto TotalSetupTime = ShootDeltaTime - ShotAnimationFinishDeltaTime;

	if (ShootDeltaTime <= ShotAnimationMinTime || TotalSetupTime <= 0)
	{
		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: InterpolateShotSetup - ShootDeltaTime=%.1f; ShotAnimationMinTime=%.1f; TotalSetupTime=%.1f - just changing rotation immediately"),
			*GetName(), ShootDeltaTime, ShotAnimationMinTime, TotalSetupTime);
		PlayerPawn->AddDeltaRotation(
			FRotator(ShotSetupParams->ShotPitch, ShotSetupParams->ShotYaw, 0.0f));

		return;
	}

	const auto YawSetupTime = TotalSetupTime / 2;
	const auto PitchSetupTime = TotalSetupTime - YawSetupTime;

	YawAnimationState.StartTimeSeconds = World->GetTimeSeconds();
	YawAnimationState.EndTimeSeconds = YawAnimationState.StartTimeSeconds + YawSetupTime;
	YawAnimationState.TargetValue = ShotSetupParams->ShotYaw;

	PitchAnimationState.StartTimeSeconds = YawAnimationState.EndTimeSeconds;
	PitchAnimationState.EndTimeSeconds = PitchAnimationState.StartTimeSeconds + PitchSetupTime;
	PitchAnimationState.TargetValue = ShotSetupParams->ShotPitch;

	auto& TimerManager = GetWorldTimerManager();
	if (!FMath::IsNearlyZero(YawAnimationState.TargetValue))
	{
		TimerManager.SetTimer(YawAnimationState.TimerHandle, this, &ThisClass::InterpolateYawAnimation, ShotAnimationDeltaTime, true);
	}

	if (!FMath::IsNearlyZero(PitchAnimationState.TargetValue))
	{
		TimerManager.SetTimer(PitchAnimationState.TimerHandle, this, &ThisClass::InterpolatePitchAnimation, ShotAnimationDeltaTime, true, YawSetupTime);
	}
}

bool AGolfAIController::InterpolateShotAnimationState(FShotAnimationState& AnimationState, double& DeltaValue) const
{
	auto World = GetWorld();

	const auto EarlyExitDelta = [&]()
	{
		DeltaValue = AnimationState.TargetValue - AnimationState.CurrentValue;
	};

	if (!World)
	{
		EarlyExitDelta();
		return true;
	}

	const auto CurrentTimeSeconds = World->GetTimeSeconds();
	if (CurrentTimeSeconds >= AnimationState.EndTimeSeconds)
	{
		EarlyExitDelta();
		return true;
	}

	const auto Alpha = FMath::Clamp((CurrentTimeSeconds - AnimationState.StartTimeSeconds) / (AnimationState.EndTimeSeconds - AnimationState.StartTimeSeconds), 0.0f, 1.0f);
	const auto NewValue = FMath::InterpEaseInOut(0.0, AnimationState.TargetValue, Alpha, ShotAnimationEaseFactor);

	DeltaValue = NewValue - AnimationState.CurrentValue;
	AnimationState.CurrentValue = NewValue;

	return false;
}

void AGolfAIController::InterpolateAnimation(FShotAnimationState& AnimationState, TFunction<FRotator(double)> RotatorCreator) const
{
	if (!IsValid(PlayerPawn))
	{
		return;
	}

	double DeltaValue{};
	const bool bClearTimer = InterpolateShotAnimationState(AnimationState, DeltaValue);

	PlayerPawn->AddDeltaRotation(RotatorCreator(DeltaValue));

	if (bClearTimer)
	{
		GetWorldTimerManager().ClearTimer(AnimationState.TimerHandle);
	}
}

void AGolfAIController::InterpolateYawAnimation()
{
	InterpolateAnimation(YawAnimationState, [](double DeltaValue) { return FRotator(0.0, DeltaValue, 0.0); });
}

void AGolfAIController::InterpolatePitchAnimation()
{
	InterpolateAnimation(PitchAnimationState, [](double DeltaValue) { return FRotator(DeltaValue, 0.0, 0.0); });
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

		if (SetupShot())
		{
			UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: SetupNextShot - Shooting after %.1fs"), *GetName(), ShotDelayTime);

			InterpolateShotSetup(ShotDelayTime);

			GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &AGolfAIController::ExecuteTurn, ShotDelayTime, false);
		}
		else
		{
			UE_VLOG_UELOG(this, LogPGAI, Error, TEXT("%s: SetupNextShot - Shot set up failed - no shot will occur"), *GetName());
		}
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

	if (auto GolfPlayerState = GetGolfPlayerState(); GolfPlayerState)
	{
		GolfPlayerState->GrabDebugSnapshot(Snapshot);
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

void AGolfAIController::InitDebugDraw() {}
void AGolfAIController::CleanupDebugDraw() {}

#endif

#pragma endregion Visual Logger
