// Copyright Game Salutes. All Rights Reserved.


#include "Controller/GolfPlayerController.h"

#include "Pawn/PaperGolfPawn.h"

#include "Library/PaperGolfPawnUtilities.h"

#include "Kismet/GameplayStatics.h"

#include "GameFramework/SpectatorPawn.h"

#include "PGPlayerLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "UI/PGHUD.h"
#include "UI/Widget/GolfUserWidget.h"

#include "State/GolfPlayerState.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include <limits>
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfPlayerController)

AGolfPlayerController::AGolfPlayerController()
{
	bReplicates = true;
	bAlwaysRelevant = true;
}

void AGolfPlayerController::AddToShotHistory(APaperGolfPawn* PaperGolfPawn)
{
	if (!PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Warning, TEXT("%s: AddToShotHistory - PaperGolfPawn is NULL"), *GetName());
		return;
	}

	const auto& ActorLocation = PaperGolfPawn->GetActorLocation();

	const auto Size = ShotHistory.Num();

	[[maybe_unused]]const auto Index = ShotHistory.AddUnique({ ActorLocation });

	if (ShotHistory.Num() > Size)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: AddToShotHistory - PaperGolfPawn=%s - New shot added to history: %s; Count=%d"),
			*GetName(),
			*PaperGolfPawn->GetName(),
			*ActorLocation.ToCompactString(),
			ShotHistory.Num());
	}
	else
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: AddToShotHistory - PaperGolfPawn=%s - Duplicate shot not added to history: %s; ExistingIndex=%d; Count=%d"),
			*GetName(),
			*PaperGolfPawn->GetName(),
			*ActorLocation.ToCompactString(),
			Index,
			ShotHistory.Num());
	}
}

void AGolfPlayerController::SnapToGround()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: SnapToGround"), *GetName());

	const auto PaperGolfPawn = GetPaperGolfPawn();

	if (!PaperGolfPawn)
	{
		return;
	}

	PaperGolfPawn->SnapToGround();
	SetViewTargetWithBlend(PaperGolfPawn);

	AddToShotHistory(PaperGolfPawn);
}

void AGolfPlayerController::ResetRotation()
{
	const auto PaperGolfPawn = GetPaperGolfPawn();

	if (!PaperGolfPawn)
	{
		return;
	}

	TotalRotation = FRotator::ZeroRotator;

	PaperGolfPawn->ResetRotation();
}

void AGolfPlayerController::ResetShot()
{
	ShotType = EShotType::Default;

	ResetRotation();
	ResetFlickZ();
}

void AGolfPlayerController::SetPaperGolfPawnAimFocus()
{
	auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!DefaultFocus)
	{
		return;
	}

	const auto& Position = PaperGolfPawn->GetActorLocation();

	AActor* BestFocus{ DefaultFocus };

	if (HasLOSToFocus(Position, DefaultFocus))
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s-%s: SetPaperGolfPawnAimFocus: LOS to DefaultFocus; Setting to %s"),
			*GetName(), *PaperGolfPawn->GetName(), *LoggingUtils::GetName(DefaultFocus));
	}
	else
	{
		const auto ToHole = DefaultFocus->GetActorLocation() - Position;

		// Find closest
		float MinDist{ std::numeric_limits<float>::max() };

		for (auto FocusTarget : FocusableActors)
		{
			if (!FocusTarget)
			{
				continue;
			}
			const auto ToFocusTarget = FocusTarget->GetActorLocation() - Position;

			// Make sure we are facing the focus target
			if ((ToFocusTarget | ToHole) <= 0)
			{
				UE_VLOG_UELOG(this, LogPGPlayer, Verbose, TEXT("%s: SetPaperGolfPawnAimFocus - Skipping target=%s as it is behind"),
					*GetName(), *FocusTarget->GetName());
				UE_VLOG_ARROW(this, LogPGPlayer, Verbose, Position, FocusTarget->GetActorLocation(), FColor::Orange, TEXT("Target: %s"), *FocusTarget->GetName());

				continue;
			}

			// Consider Z as don't want to aim at targets way above or below us
			const auto DistSq = ToFocusTarget.SizeSquared();
			if (DistSq < MinDist && HasLOSToFocus(Position, FocusTarget))
			{
				MinDist = DistSq;
				BestFocus = FocusTarget;
			}
		} // for

		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s-%s: SetPaperGolfPawnAimFocus: BestFocus=%s"),
			*GetName(), *PaperGolfPawn->GetName(), *LoggingUtils::GetName(BestFocus));
		if (BestFocus)
		{
			UE_VLOG_ARROW(this, LogPGPlayer, Log, Position, BestFocus->GetActorLocation(), FColor::Blue, TEXT("Target: %s"), *BestFocus->GetName());
		}
	}

	PaperGolfPawn->SetFocusActor(BestFocus);
}

void AGolfPlayerController::DetermineIfCloseShot()
{
	auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	if (!DefaultFocus)
	{
		return;
	}

	const auto ShotDistance = PaperGolfPawn->GetDistanceTo(DefaultFocus);
	const bool bCloseShot = ShotDistance < CloseShotThreshold;

	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s-%s: DetermineIfCloseShot: %s - Distance=%fm"),
		*GetName(), *PaperGolfPawn->GetName(), LoggingUtils::GetBoolString(bCloseShot), ShotDistance / 100);

	PaperGolfPawn->SetCloseShot(bCloseShot);
}

void AGolfPlayerController::MarkScored()
{
	bScored = true;

	// Rep notifies are not called on the server so we need to invoke the function manually if the server is also a client
	if (HasAuthority() && IsLocalController())
	{
		OnScored();
	}
}

void AGolfPlayerController::OnRep_Scored()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s-%s: OnRep_Scored - %s"),
		*GetName(), *LoggingUtils::GetName(GetPawn()), LoggingUtils::GetBoolString(bScored));

	OnScored();
}

void AGolfPlayerController::OnScored()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s-%s: OnScored - %s"),
		*GetName(), *LoggingUtils::GetName(GetPawn()), LoggingUtils::GetBoolString(bScored));

	// Nothing to do if scored is false
	if (!bScored)
	{
		return;
	}

	bCanFlick = false;

	auto GolfPlayerState = GetPlayerState<AGolfPlayerState>();
	if (!ensure(GolfPlayerState))
	{
		return;
	}

	// GetShots may not necessary be accurate if the Shots variable replication happens after the score replication;
	// however, it is only for logging purposes
	UE_VLOG_UELOG(this, LogPGPlayer, Display, TEXT("%s-%s: OnRep_Scored: NumStrokes=%d"),
		*GetName(), *LoggingUtils::GetName(GetPawn()), GolfPlayerState->GetShots());

	if (IsLocalController())
	{
		if (auto HUD = GetHUD<APGHUD>(); ensure(HUD))
		{
			// Won't be present on non-locally controlled player
			HUD->DisplayMessageWidget(EMessageWidgetType::HoleFinished);
		}
	}
}

bool AGolfPlayerController::HasLOSToFocus(const FVector& Position, const AActor* FocusActor) const
{
	auto World = GetWorld();
	if (!ensure(World))
	{
		return false;
	}

	if (!FocusActor)
	{
		return false;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(FocusActor);

	// prefer the hole if have LOS
	// TODO: Use custom trace channel or possibly alternative strategy as this doesn't always work as intended
	const bool bLOS = !World->LineTraceTestByChannel(
		Position + 200.f,
		FocusActor->GetActorLocation() + 200.f,
		ECollisionChannel::ECC_Visibility,
		QueryParams
	);

	UE_VLOG_ARROW(this, LogPGPlayer, Verbose, Position + 200.f, FocusActor->GetActorLocation() + 200.f,
		bLOS ? FColor::Green : FColor::Red, TEXT("BestFocus: %s"), *FocusActor->GetName());

	return bLOS;
}

bool AGolfPlayerController::IsFlickedAtRest() const
{
	if (bCanFlick)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, VeryVerbose, TEXT("%s: IsFlickedAtRest - FALSE - bCanFlick is TRUE"),
			*GetName());

		return false;
	}

	const auto PaperGolfPawn = GetPaperGolfPawn();

	if (!PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, VeryVerbose, TEXT("%s: IsFlickedAtRest - FALSE - PaperGolfPawn is NULL"),
			*GetName());

		return false;
	}

	const auto bPaperGolfAtRest = PaperGolfPawn->IsAtRest();

	UE_VLOG_UELOG(this, LogPGPlayer, VeryVerbose, TEXT("%s: IsFlickedAtRest - %s - Result of PaperGolfPawn::IsAtRest"),
		*GetName(), LoggingUtils::GetBoolString(bPaperGolfAtRest));

	return bPaperGolfAtRest;
}

void AGolfPlayerController::DrawFlickLocation()
{
	const auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	UPaperGolfPawnUtilities::DrawPoint(GetWorld(), PaperGolfPawn->GetFlickLocation(FlickZ), FlickReticuleColor);
}

void AGolfPlayerController::ProcessFlickZInput(float FlickZInput)
{
	if (!bCanFlick)
	{
		return;
	}

	const auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	const auto PreviousFlickZ = FlickZ;
	const auto FlickZInputSign = FMath::Sign(FlickZInput);

	for (int32 i = 0; i <= FlickZNotUpdatedMaxRetries; ++i)
	{
		FlickZ = PaperGolfPawn->ClampFlickZ(PreviousFlickZ, FlickZInput);
		const bool bUpdated = !FMath::IsNearlyEqual(FlickZ, PreviousFlickZ);

		UE_VLOG_UELOG(this, LogPGPlayer, VeryVerbose,
			TEXT("%s: ProcessFlickZInput - Iter=%d; FlickZInput=%f; ClampedFlickZInput=%f; NewFlickZ=%f; PreviousFlickZ=%f; Updated=%s"),
			*GetName(), i,
			FlickZInput, FlickZ - PreviousFlickZ, FlickZ, PreviousFlickZ, LoggingUtils::GetBoolString(bUpdated));

		// Retry if not updated with a larger offset
		if (bUpdated)
		{
			break;
		}

		FlickZInput += FlickZInputSign * PaperGolfPawn->GetFlickOffsetZTraceSize();
	}

	DrawFlickLocation();
}

void AGolfPlayerController::InitFocusableActors()
{
	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	if (ensureMsgf(FocusableActorClass, TEXT("%s: InitFocusableActors - FocusableActorClass not set"), *GetName()))
	{
		FocusableActors.Reset();
		UGameplayStatics::GetAllActorsWithInterface(GetWorld(), FocusableActorClass, FocusableActors);
		if (FocusableActors.IsEmpty())
		{
			UE_VLOG_UELOG(this, LogPGPlayer, Warning,
				TEXT("%s: InitFocusableActors - no instances of FocusActorClass=%s present in world. Aim targeting impacted."),
				*GetName(), *FocusableActorClass->GetName());
		}
	}

	if (ensureMsgf(GolfHoleClass, TEXT("%s: InitFocusableActors - GolfHoleClass not set"), *GetName()))
	{
		DefaultFocus = UGameplayStatics::GetActorOfClass(GetWorld(), GolfHoleClass);
		ensureMsgf(DefaultFocus, TEXT("%s: InitFocusableActors - No instance of %s in world for default focus. No aim targeting will occur."), *GetName(),
			*GolfHoleClass->GetName());
	}
}

void AGolfPlayerController::ResetFlickZ()
{
	FlickZ = 0.f;
	DrawFlickLocation();
}

void AGolfPlayerController::AddPaperGolfPawnRelativeRotation(const FRotator& DeltaRotation)
{
	auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	auto RotationToApply = DeltaRotation * (RotationRate * GetWorld()->GetDeltaSeconds());

	UPaperGolfPawnUtilities::ClampDeltaRotation(RotationMax, RotationToApply, TotalRotation);

	if (FMath::IsNearlyZero(RotationToApply.Yaw))
	{
		PaperGolfPawn->AddActorLocalRotation(RotationToApply);
	}
	else
	{
		PaperGolfPawn->AddActorWorldRotation(RotationToApply);
	}
}

bool AGolfPlayerController::IsReadyForNextShot() const
{
	if (!IsFlickedAtRest())
	{
		UE_VLOG_UELOG(this, LogPGPlayer, VeryVerbose,
			TEXT("%s: IsReadyForNextShot - Skip - Not at rest"),
			*GetName());
		return false;
	}

	if (bOutOfBounds)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, VeryVerbose,
			TEXT("%s: IsReadyForNextShot - Skip - Out of Bounds"),
			*GetName());
		return false;
	}

	return true;
}

void AGolfPlayerController::SetupNextShot()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: SetupNextShot"), *GetName());

	if(!ensureMsgf(IsReadyForNextShot(), TEXT("%s-%s: SetupNextShot - Not ready for next shot"), *GetName(), *LoggingUtils::GetName(GetPawn())))
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Error, TEXT("%s-%s: SetupNextShot - Not ready for next shot"), *GetName(), *LoggingUtils::GetName(GetPawn()));
	}

	auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, VeryVerbose,
			TEXT("%s: CheckAndSetupNextShot - Skip - No PaperGolf pawn"),
			*GetName());
		return;
	}

	PaperGolfPawn->SetUpForNextShot();

	ResetShot();
	SetPaperGolfPawnAimFocus();
	SnapToGround();
	DetermineIfCloseShot();
	DrawFlickLocation();

	if (IsLocalController())
	{
		if (auto HUD = GetHUD<APGHUD>(); ensure(HUD))
		{
			if (auto GolfWidget = HUD->GetGolfWidget(); ensure(GolfWidget))
			{
				GolfWidget->ResetMeter();
			}
		}
	}

	bCanFlick = true;
}

void AGolfPlayerController::HandleFallThroughFloor()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log,
		TEXT("%s-%s: HandleFallThroughFloor"),
		*GetName(), *LoggingUtils::GetName(GetPawn()));

	auto PaperGolfPawn = GetPaperGolfPawn();

	if (!PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Warning,
			TEXT("%s: HandleFallThroughFloor - PaperGolf pawn is NULL"),
			*GetName());
		return;
	}

	const auto& Position = PaperGolfPawn->GetActorLocation() + FVector::UpVector * FallThroughFloorCorrectionTestZ;

	SetPositionTo(Position);

	if (HasAuthority() && !IsLocalController())
	{
		ClientSetPositionTo(Position);
	}
}

void AGolfPlayerController::SetPositionTo(const FVector& Position)
{
	auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	PaperGolfPawn->SetUpForNextShot();

	PaperGolfPawn->SetActorLocation(
		Position,
		false,
		nullptr,
		TeleportFlagToEnum(true)
	);

	UE_VLOG_LOCATION(this, LogPGPlayer, Log, Position, 20.0f, FColor::Red, TEXT("SetPositionTo"));

	SetupNextShot();
}

void AGolfPlayerController::ClientSetPositionTo_Implementation(const FVector_NetQuantize& Position)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log,
		TEXT("%s: ClientSetPositionTo - Position=%s"),
		*GetName(), *Position.ToCompactString());

	// Disable any timers so we don't overwrite the position
	UnregisterShotFinishedTimer();

	SetPositionTo(Position);
}

void AGolfPlayerController::CheckForNextShot()
{
	if (bScored)
	{
		UnregisterShotFinishedTimer();
		return;
	}

	if (!IsReadyForNextShot())
	{
		return;
	}

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	UnregisterShotFinishedTimer();

	bTurnActivated = false;

	// invoke a client reliable function to say the next shot is ready unless the server is the client
	if (HasAuthority() && !IsLocalPlayerController())
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log,
			TEXT("%s-%s: CheckForNextShot - Setting final authoritative position for pawn: %s"),
			*GetName(), *PaperGolfPawn->GetName(), *PaperGolfPawn->GetActorLocation().ToCompactString());

		ClientSetPositionTo(PaperGolfPawn->GetActorLocation());
	}


	if (auto GolfEventSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventSubsystem))
	{
		// This will call a function on server from game mode to set up next turn - we use above RPC to make sure 
		GolfEventSubsystem->OnPaperGolfShotFinished.Broadcast(PaperGolfPawn);
	}
}

void AGolfPlayerController::ProcessShootInput()
{
	// function should only be called on local controllers
	ensure(IsLocalController());

	if (!bCanFlick)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Verbose,
			TEXT("%s: ProcessShootInput - bCanFlick is False"),
			*GetName());
		return;
	}

	auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	auto HUD = GetHUD<APGHUD>();
	if (!ensure(HUD))
	{
		return;
	}

	auto GolfWidget = HUD->GetGolfWidget();
	if (!ensure(GolfWidget))
	{
		return;
	}

	GolfWidget->ProcessClickForMeter();

	if (!GolfWidget->IsReadyForShot())
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Verbose,
			TEXT("%s: ProcessShootInput - Not ready for shot"),
			*GetName());

		return;
	}

	UE_VLOG_UELOG(this, LogPGPlayer, Log,
		TEXT("%s: ProcessShootInput - Calling Flick"),
		*GetName());

	const auto Power = GolfWidget->GetMeterPower();
	const auto Accuracy = GolfWidget->GetMeterAccuracy();

	PaperGolfPawn->Flick(FlickZ, Power, Accuracy);
	bCanFlick = false;

	ServerProcessShootInput();
}

void AGolfPlayerController::ServerProcessShootInput_Implementation()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log,
		TEXT("%s: ServerProcessShootInput"), *GetName());

	AddStroke();
	bCanFlick = false;
}

void AGolfPlayerController::AddStroke()
{
	auto GolfPlayerState = GetPlayerState<AGolfPlayerState>();
	if (!ensure(GolfPlayerState))
	{
		return;
	}

	GolfPlayerState->AddShot();
}

void AGolfPlayerController::HandleOutOfBounds()
{
	if (bOutOfBounds)
	{
		return;
	}

	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: HandleOutOfBounds"), *GetName());

	bOutOfBounds = true;

	auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	// One stroke penalty
	AddStroke();

	if (IsLocalController())
	{
		if (auto HUD = GetHUD<APGHUD>(); ensure(HUD))
		{
			HUD->DisplayMessageWidget(EMessageWidgetType::OutOfBounds);
		}
	}

	auto World = GetWorld();
	if(!ensure(World))
	{
		return;
	}

	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, this, &ThisClass::ResetShotAfterOutOfBounds, OutOfBoundsDelayTime);
}

void AGolfPlayerController::ResetShotAfterOutOfBounds()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: ResetShotAfterOutOfBounds"), *GetName());

	if (IsLocalController())
	{
		if (auto HUD = GetHUD<APGHUD>(); ensure(HUD))
		{
			HUD->RemoveActiveMessageWidget();
		}
	}

	auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	PaperGolfPawn->SetUpForNextShot();

	bOutOfBounds = false;

	// Set location to last in shot history
	if(ensureMsgf(!ShotHistory.IsEmpty(), TEXT("%s-%s: ResetShotAfterOutOfBounds - ShotHistory is empty"),
		*GetName(), *PaperGolfPawn->GetName()))
	{
		const auto& ResetPosition = ShotHistory.Last().Position;
		SetPositionTo(ResetPosition);

		if (HasAuthority() && !IsLocalController())
		{
			ClientSetPositionTo(ResetPosition);
		}
	}
}

APaperGolfPawn* AGolfPlayerController::GetPaperGolfPawn()
{
	const auto PaperGolfPawn = Cast<APaperGolfPawn>(GetPawn());

	if (!ensureMsgf(PaperGolfPawn, TEXT("%s: GetPaperGolfPawn - %s is not a APaperGolfPawn"), *GetName(), *LoggingUtils::GetName(GetPawn())))
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Error, TEXT("%s: GetPaperGolfPawn - %s is not a APaperGolfPawn"), *GetName(), *LoggingUtils::GetName(GetPawn()));
		return nullptr;
	}

	return PaperGolfPawn;
}

const APaperGolfPawn* AGolfPlayerController::GetPaperGolfPawn() const
{
	return const_cast<AGolfPlayerController*>(this)->GetPaperGolfPawn();
}

void AGolfPlayerController::BeginPlay()
{
	Super::BeginPlay();

	Init();
}

void AGolfPlayerController::Init()
{
	// turn is activated manually so we set this to false initially
	bCanFlick = false;

	RegisterGolfSubsystemEvents();

	if (auto World = GetWorld(); ensure(World))
	{
		FTimerHandle InitTimerHandle;
		World->GetTimerManager().SetTimer(InitTimerHandle, this, &ThisClass::DeferredInit, 0.2f);
	}

	InitDebugDraw();
}

void AGolfPlayerController::DeferredInit()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: DeferredInit"), *GetName());

	InitFocusableActors();
}

void AGolfPlayerController::RegisterShotFinishedTimer()
{
	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	World->GetTimerManager().SetTimer(NextShotTimerHandle, this, &ThisClass::CheckForNextShot, RestCheckTickRate, true);
}

void AGolfPlayerController::UnregisterShotFinishedTimer()
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(NextShotTimerHandle);
}

void AGolfPlayerController::RegisterGolfSubsystemEvents()
{
	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	auto GolfSubsystem = World->GetSubsystem<UGolfEventsSubsystem>();
	if (!ensure(GolfSubsystem))
	{
		return;
	}

	GolfSubsystem->OnPaperGolfPawnOutBounds.AddDynamic(this, &ThisClass::OnOutOfBounds);
	GolfSubsystem->OnPaperGolfPawnClippedThroughWorld.AddDynamic(this, &ThisClass::OnFellThroughFloor);
}

void AGolfPlayerController::OnOutOfBounds(APaperGolfPawn* InPaperGolfPawn)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: OnOutOfBounds: InPaperGolfPawn=%s"), *GetName(), *LoggingUtils::GetName(InPaperGolfPawn));

	auto PaperGolfPawn = GetPaperGolfPawn();

	if (PaperGolfPawn != InPaperGolfPawn)
	{
		return;
	}

	HandleOutOfBounds();
}

void AGolfPlayerController::OnFellThroughFloor(APaperGolfPawn* InPaperGolfPawn)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: OnFellThroughFloor: InPaperGolfPawn=%s"), *GetName(), *LoggingUtils::GetName(InPaperGolfPawn));

	auto PaperGolfPawn = GetPaperGolfPawn();

	if (PaperGolfPawn != InPaperGolfPawn)
	{
		return;
	}

	HandleFallThroughFloor();
}

EShotType AGolfPlayerController::GetShotType() const
{
	if (ShotType != EShotType::Default)
	{
		return ShotType;
	}

	const auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Warning, TEXT("%s: GetShotType - Pawn not defined. Returning Default"), *GetName());
		return EShotType::Default;
	}

	return PaperGolfPawn->IsCloseShot() ? EShotType::Close : EShotType::Full;
}

void AGolfPlayerController::OnPossess(APawn* InPawn)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: OnPossess - InPawn=%s"), *GetName(), *LoggingUtils::GetName(InPawn));

	Super::OnPossess(InPawn);
}

void AGolfPlayerController::OnUnPossess()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: OnUnPossess - ExistingPawn=%s"), *GetName(), *LoggingUtils::GetName(GetPawn()));

	Super::OnUnPossess();
}

void AGolfPlayerController::SetPawn(APawn* InPawn)
{
	// Note that this is also called on server from game mode when RestartPlayer is called
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: SetPawn - InPawn=%s"), *GetName(), *LoggingUtils::GetName(InPawn));

	Super::SetPawn(InPawn);

	const auto PaperGolfPawn = Cast<APaperGolfPawn>(InPawn); PaperGolfPawn;

	// The player pawn will be first paper golf pawn possessed so set if not already set
	if (!IsValid(PlayerPawn) && IsValid(PaperGolfPawn))
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Display, TEXT("%s: SetPawn - PlayerPawn=%s"), *GetName(), *LoggingUtils::GetName(InPawn));
		PlayerPawn = PaperGolfPawn;
	}

	if (IsValid(PaperGolfPawn))
	{
		DoActivateTurn();
	}
}

void AGolfPlayerController::ToggleShotType()
{
	switch (GetShotType())
	{
		case EShotType::Close:
			return SetShotType(EShotType::Full);
		default:
			return SetShotType(EShotType::Close);
	}
}

void AGolfPlayerController::SetShotType(EShotType InShotType)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: SetShotType: %s"), *GetName(), *LoggingUtils::GetName(InShotType));

	const auto PaperGolfPawn = Cast<APaperGolfPawn>(GetPawn());
	if (!PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Warning, TEXT("%s: SetShotType: %s - Pawn not defined - Ignoring"), *GetName(), *LoggingUtils::GetName(InShotType));
		return;
	}

	ShotType = InShotType;

	PaperGolfPawn->SetCloseShot(ShotType == EShotType::Close);
}

#pragma region Turn and spectator logic

void AGolfPlayerController::ActivateTurn()
{
	// TODO: Unhide the player and possess the player paper golf pawn
	// This can only be called on server
	if (!ensureAlwaysMsgf(HasAuthority(), TEXT("%s: ActivateTurn - Called on client!"), *GetName()))
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Error, TEXT("%s: ActivateTurn - Called on client!"), *GetName());
		return;
	}

	if(!ensureAlwaysMsgf(PlayerPawn, TEXT("%s: ActivateTurn - PlayerPawn is NULL"), *GetName()))
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Error, TEXT("%s: ActivateTurn - PlayerPawn is NULL"), *GetName());
		return;
	}

	if (GetPawn() != PlayerPawn)
	{
		Possess(PlayerPawn);
		// Turn activation will happen on SetPawn after possession has replicated
	}
	else
	{
		ClientActivateTurn();

		// Make sure that we execute on the server if this isn't a listen server client
		if (!IsLocalController())
		{
			DoActivateTurn();
		}
	}
}

void AGolfPlayerController::ClientActivateTurn_Implementation()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: ClientActivateTurn"), *GetName());

	DoActivateTurn();
}

void AGolfPlayerController::DoActivateTurn()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: DoActivateTurn"), *GetName());

	// Ensure that input is always activated and timer is always registered
	EnableInput(this);
	RegisterShotFinishedTimer();

	if (bTurnActivated)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: DoActivateTurn - Turn already activated"), *GetName());
		return;
	}

	const auto PaperGolfPawn = Cast<APaperGolfPawn>(GetPawn());

	// Activate the turn if switching from spectator to player
	// Doing this here as this function is called when replication of the pawn possession completes
	if (!PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: DoActivateTurn - Skipping turn activation as Pawn=%s is not APaperGolfPawn"),
			*GetName(), *LoggingUtils::GetName(GetPawn()));
		return;
	}

	SetupNextShot();
	bTurnActivated = true;
}

void AGolfPlayerController::Spectate(APaperGolfPawn* InPawn)
{
	// This can only be called on server
	if(!ensureAlwaysMsgf(HasAuthority(), TEXT("%s: Spectate - InPawn=%s called on client!"), *GetName(), *LoggingUtils::GetName(InPawn)))
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Error, TEXT("%s: Spectate - InPawn=%s called on client!"), *GetName(), *LoggingUtils::GetName(InPawn));
		return;
	}

	UE_VLOG_UELOG(this, LogPGPlayer, Display, TEXT("%s: Spectate - InPawn=%s"), *GetName(), *LoggingUtils::GetName(InPawn));

	// TODO: Hide the player pawn and UI and switch to spectate the input player pawn
	// Need to account for possibly destroying the player pawn after scoring so the player pawn could be null
	// Should track the possessed paper golf pawn as need to switch back to it when activating the turn

	// Allow the spectator pawn to take over the controls; otherwise, some of the bindings will be disabled
	ClientSpectate(InPawn);

	AddSpectatorPawn(InPawn);
}

void AGolfPlayerController::ClientSpectate_Implementation(APaperGolfPawn* InPawn)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: ClientSpectate - %s"), *GetName(), *LoggingUtils::GetName(InPawn));

	// TODO: Do we need to wait for Spectate to propagate?
	// Allow the spectator pawn to take over the controls; otherwise, some of the bindings will be disabled

	DisableInput(this);
}

void AGolfPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME_CONDITION(AGolfPlayerController, ShotFinishedLocation, COND_OwnerOnly);
	DOREPLIFETIME(AGolfPlayerController, bScored);
}

void AGolfPlayerController::AddSpectatorPawn(APawn* PawnToSpectate)
{
	if (!HasAuthority())
	{
		// Can only change state to spectator on the server side
		return;
	}

	UE_VLOG_UELOG(this, LogPGPlayer, Display, TEXT("%s: Changing state to spectator"), *GetName());

	ChangeState(NAME_Spectating);
	ClientGotoState(NAME_Spectating);
	SetCameraToViewPawn(PawnToSpectate);
}

void AGolfPlayerController::SetCameraToViewPawn(APawn* InPawn)
{
	if (!IsValid(InPawn))
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Warning, TEXT("%s: SetCameraToViewPawn: NULL - skipping directly to spectator controls"),
			*GetName()
		);

		SetCameraOwnedBySpectatorPawn(nullptr);
		return;
	}

	UE_VLOG_UELOG(this, LogPGPlayer, Display, TEXT("%s: SetCameraToViewPawn: Tracking player pawn %s"),
		*GetName(),
		*InPawn->GetName()
	);

	// TODO: Consider using SetViewTargetWithBlend
	// Whatever the InPawn player is seeing, we will also see
	check(InPawn);
	SetViewTarget(InPawn);

	GetWorldTimerManager().SetTimer(SpectatorCameraDelayTimer,
		FTimerDelegate::CreateWeakLambda(this, [this, SpectatorPawn = MakeWeakObjectPtr(InPawn)]()
		{
			SetCameraOwnedBySpectatorPawn(SpectatorPawn.Get());
		}), SpectatorCameraControlsDelay, false);
}

// TODO: Don't think this does anything as AutoManageActiveCameraTarget needs to be true
void AGolfPlayerController::SetCameraOwnedBySpectatorPawn(APawn* InPawn)
{
	auto MySpectatorPawn = GetSpectatorPawn();

	if (!MySpectatorPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Warning, TEXT("%s: SetCameraOwnedBySpectatorPawn: InPawn=%s Spectator Pawn is NULL"),
			*GetName(), *LoggingUtils::GetName(InPawn));
		return;
	}

	UE_VLOG_UELOG(this, LogPGPlayer, Display, TEXT("%s: Managing camera target to InPawn=%s with SpectatorPawn=%s"),
		*GetName(),
		*LoggingUtils::GetName(InPawn),
		*LoggingUtils::GetName(MySpectatorPawn)
	);

	AutoManageActiveCameraTarget(InPawn);
}

#pragma endregion Turn and spectator logic

#pragma region Visual Logger

#if ENABLE_VISUAL_LOG

void AGolfPlayerController::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	FVisualLogStatusCategory Category;
	Category.Category = FString::Printf(TEXT("GolfPlayerController (%s)"), *GetName());

	Category.Add(TEXT("TurnActivated"), LoggingUtils::GetBoolString(bTurnActivated));
	Category.Add(TEXT("CanFlick"), LoggingUtils::GetBoolString(bCanFlick));
	Category.Add(TEXT("GolfInputEnabled"), LoggingUtils::GetBoolString(bInputEnabled));
	Category.Add(TEXT("ControllerInputEnabled"), LoggingUtils::GetBoolString(InputEnabled()));
	Category.Add(TEXT("OutOfBounds"), LoggingUtils::GetBoolString(bOutOfBounds));
	Category.Add(TEXT("Scored"), LoggingUtils::GetBoolString(bScored));
	Category.Add(TEXT("TotalRotation"), TotalRotation.ToCompactString());
	Category.Add(TEXT("FlickZ"), FString::Printf(TEXT("%.2f"), FlickZ));
	Category.Add(TEXT("ShotType"), LoggingUtils::GetName(ShotType));
	Category.Add(TEXT("NextShotTimer"), LoggingUtils::GetBoolString(NextShotTimerHandle.IsValid()));
	Category.Add(TEXT("SpectatorCameraDelayTimer"), LoggingUtils::GetBoolString(SpectatorCameraDelayTimer.IsValid()));

	Snapshot->Status.Add(Category);

	// TODO: Consider moving logic to PlayerState itself
	if (auto GolfPlayerState = GetPlayerState<AGolfPlayerState>(); GolfPlayerState)
	{
		FVisualLogStatusCategory PlayerStateCategory;
		PlayerStateCategory.Category = FString::Printf(TEXT("PlayerState"));

		PlayerStateCategory.Add(TEXT("Shots"), FString::Printf(TEXT("%d"), GolfPlayerState->GetShots()));

		Snapshot->Status.Last().AddChild(PlayerStateCategory);
	}

	// Only server seems to log the pawn - so grab a snapshot explicitly if locally controlled so we can compare to the server state
	if (!HasAuthority())
	{
		if (auto PaperGolfPawn = Cast<APaperGolfPawn>(GetPawn()))
		{
			PaperGolfPawn->GrabDebugSnapshot(Snapshot);
		}
	}
}

void AGolfPlayerController::InitDebugDraw()
{
	// Ensure that state logged regularly so we see the updates in the visual logger

	FTimerDelegate DebugDrawDelegate = FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UE_VLOG(this, LogPGPlayer, Log, TEXT("Get Player State"));
	});

	GetWorldTimerManager().SetTimer(VisualLoggerTimer, DebugDrawDelegate, 0.05f, true);
}

#else

void AGolfPlayerController::InitDebugDraw() {}

#endif

#pragma endregion Visual Logger