// Copyright Game Salutes. All Rights Reserved.


#include "Controller/GolfPlayerController.h"

#include "Pawn/PaperGolfPawn.h"

#include "Library/PaperGolfUtilities.h"

#include "Kismet/GameplayStatics.h"

#include "PGPlayerLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "UI/PGHUD.h"
#include "UI/Widget/GolfUserWidget.h"

#include "State/GolfPlayerState.h"

#include <limits>

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfPlayerController)

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

void AGolfPlayerController::OnScored()
{
	bScored = true;
	bCanFlick = false;

	auto GolfPlayerState = GetPlayerState<AGolfPlayerState>();
	if(!ensure(GolfPlayerState))
	{
		return;
	}

	UE_VLOG_UELOG(this, LogPGPlayer, Display, TEXT("%s-%s: OnScored: NumStrokes=%d"),
		*GetName(), *LoggingUtils::GetName(GetPawn()), GolfPlayerState->GetShots());

	if (IsLocalController())
	{
		if (auto HUD = GetHUD<APGHUD>(); ensure(HUD))
		{
			// Won't be present on non-locally controlled player
			HUD->DisplayMessageWidget(EMessageWidgetType::HoleFinished);
		}
	}

	NextHole();
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
		return false;
	}

	const auto PaperGolfPawn = GetPaperGolfPawn();

	if (!PaperGolfPawn)
	{
		return false;
	}

	return PaperGolfPawn->IsAtRest();
}

void AGolfPlayerController::DrawFlickLocation()
{
	const auto PaperGolfPawn = GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	UPaperGolfUtilities::DrawPoint(GetWorld(), PaperGolfPawn->GetFlickLocation(FlickZ, 1.f, 1.f), FlickReticuleColor);
}

void AGolfPlayerController::ProcessFlickZInput(float FlickZInput)
{
	if (!bCanFlick)
	{
		return;
	}

	FlickZ += FlickZInput;

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

	UPaperGolfUtilities::ClampDeltaRotation(RotationMax, RotationToApply, TotalRotation);

	if (FMath::IsNearlyZero(RotationToApply.Yaw))
	{
		PaperGolfPawn->AddActorLocalRotation(RotationToApply);
	}
	else
	{
		PaperGolfPawn->AddActorWorldRotation(RotationToApply);
	}

}

// TODO: Will need to disable once score but that might be handled by transitioning game mode, etc
void AGolfPlayerController::CheckAndSetupNextShot()
{
	if (!IsFlickedAtRest())
	{
		UE_VLOG_UELOG(this, LogPGPlayer, VeryVerbose,
			TEXT("%s: CheckAndSetupNextShot - Skip - Not at rest"),
			*GetName());
		return;
	}

	if (bOutOfBounds)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, VeryVerbose,
			TEXT("%s: CheckAndSetupNextShot - Skip - Out of Bounds"),
			*GetName());
		return;
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

	CheckAndSetupNextShot();
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

void AGolfPlayerController::NextHole()
{
	// TODO: Adjust once have multiple holes and a way to transition between them
	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, this, &ThisClass::RestartLevel, NextHoleDelay);
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
		SetPositionTo(ShotHistory.Last().Position);
	}
}

APaperGolfPawn* AGolfPlayerController::GetPaperGolfPawn()
{
	const auto PaperGolfPawn = Cast<APaperGolfPawn>(GetPawn());

	if (!ensureMsgf(PaperGolfPawn, TEXT("%s: GetPaperGolfPawn - %s is not a APaperGolfPawn"), *GetName(), *LoggingUtils::GetName(GetPawn())))
	{
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

	bCanFlick = true;
}
