// Copyright Game Salutes. All Rights Reserved.


#include "Controller/GolfPlayerController.h"

#include "Pawn/PaperGolfPawn.h"

#include "Interfaces/FocusableActor.h"

#include "Library/PaperGolfPawnUtilities.h"

#include "Kismet/GameplayStatics.h"

#include "GameFramework/SpectatorPawn.h"

#include "PGPlayerLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Utils/StringUtils.h"

#include "UI/PGHUD.h"
#include "UI/Widget/GolfUserWidget.h"

#include "Components/ShotArcPreviewComponent.h"
#include "Components/GolfControllerCommonComponent.h"

#include "PaperGolfTypes.h"

#include "Subsystems/TutorialTrackingSubsystem.h"

#include "State/GolfPlayerState.h"
#include "State/PaperGolfGameStateBase.h"

#include "Golf/GolfHole.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include <limits>
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfPlayerController)

AGolfPlayerController::AGolfPlayerController()
{
	bReplicates = true;
	bAlwaysRelevant = true;

	ShotArcPreviewComponent = CreateDefaultSubobject<UShotArcPreviewComponent>(TEXT("ShotArcPreview"));
	GolfControllerCommonComponent = CreateDefaultSubobject<UGolfControllerCommonComponent>(TEXT("GolfControllerCommon"));
}

void AGolfPlayerController::ResetForCamera()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: ResetForCamera"), *GetName());

	// Ignore if we haven't been possessed yet
	const auto PaperGolfPawn = Cast<APaperGolfPawn>(GetPawn());

	if (!PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: ResetForCamera - Skipping as Pawn=%s is not a APaperGolfPawn"), *GetName(), *LoggingUtils::GetName(GetPawn()));
		return;
	}

	PaperGolfPawn->SnapToGround();
	SetViewTargetWithBlend(PaperGolfPawn);
}

void AGolfPlayerController::ResetShot()
{
	ShotType = EShotType::Default;
	TotalRotation = FRotator::ZeroRotator;

	GolfControllerCommonComponent->ResetShot();

	ResetFlickZ();
	DetermineShotType();
}


void AGolfPlayerController::DetermineShotType()
{
	const auto NewShotType = GolfControllerCommonComponent->DetermineShotType();

	if (NewShotType != EShotType::Default)
	{
		SetShotType(NewShotType);
	}
}

void AGolfPlayerController::MarkScored()
{
	bScored = true;

	// Rep notifies are not called on the server so we need to invoke the function manually if the server is also a client
	if (HasAuthority())
	{
		if (IsLocalController())
		{
			OnScored();
		}

		DestroyPawn();
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

	auto GolfPlayerState = GetGolfPlayerState();
	if (!ensure(GolfPlayerState))
	{
		return;
	}

	if (HasAuthority())
	{
		GolfPlayerState->SetReadyForShot(false);
	}

	// GetShots may not necessary be accurate if the Shots variable replication happens after the score replication;
	// however, it is only for logging purposes
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

void AGolfPlayerController::SetupNextShot(bool bSetCanFlick)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: SetupNextShot: bSetCanFlick=%s"), *GetName(), LoggingUtils::GetBoolString(bSetCanFlick));

	if(!GolfControllerCommonComponent->SetupNextShot(bSetCanFlick))
	{
		return;
	}

	ResetShot();
	GolfControllerCommonComponent->SetPaperGolfPawnAimFocus();
	ResetForCamera();

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

	if (bSetCanFlick)
	{
		bCanFlick = true;
	}
}

void AGolfPlayerController::SetPositionTo(const FVector& Position, const TOptional<FRotator>& OptionalRotation)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log,
		TEXT("%s: SetPositionTo - Position=%s; Rotation=%s"),
		*GetName(), *Position.ToCompactString(), *PG::StringUtils::ToString(OptionalRotation));

	GolfControllerCommonComponent->SetPositionTo(Position, OptionalRotation);

	if (HasAuthority())
	{
		SetupNextShot(false);
	}
}

void AGolfPlayerController::ClientSetTransformTo_Implementation(const FVector_NetQuantize& Position, const FRotator& Rotation)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log,
		TEXT("%s: ClientSetTransformTo_Implementation - Position=%s; Rotation=%s"),
		*GetName(), *Position.ToCompactString(), *Rotation.ToCompactString());

	bTurnActivated = false;
	// Disable any timers so we don't overwrite the position
	GolfControllerCommonComponent->EndTurn();

	SetPositionTo(Position, Rotation);
}

void AGolfPlayerController::DoAdditionalOnShotFinished()
{
	bTurnActivated = false;

	// invoke a client reliable function to say the next shot is ready unless the server is the client
	if (auto PaperGolfPawn = GetPaperGolfPawn(); IsRemoteServer() && PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log,
			TEXT("%s-%s: OnShotFinished - Setting final authoritative position for client pawn: %s"),
			*GetName(), *PaperGolfPawn->GetName(), *PaperGolfPawn->GetActorLocation().ToCompactString());

		ClientSetTransformTo(PaperGolfPawn->GetActorLocation(), PaperGolfPawn->GetActorRotation());
	}
}

void AGolfPlayerController::DoAdditionalFallThroughFloor()
{
	if (auto PaperGolfPawn = GetPaperGolfPawn(); IsRemoteServer() && PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log,
			TEXT("%s-%s: DoAdditionalFallThroughFloor - Setting final authoritative position for client pawn: %s"),
			*GetName(), *PaperGolfPawn->GetName(), *PaperGolfPawn->GetActorLocation().ToCompactString());

		ClientSetTransformTo(PaperGolfPawn->GetActorLocation(), PaperGolfPawn->GetActorRotation());
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

	PaperGolfPawn->Flick(FFlickParams
	{
		.ShotType = GetShotType(),
		.LocalZOffset = FlickZ,
		.PowerFraction = Power,
		.Accuracy = Accuracy
	});

	bCanFlick = false;

	ServerProcessShootInput(TotalRotation);
}

void AGolfPlayerController::ServerProcessShootInput_Implementation(const FRotator& InTotalRotation)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log,
		TEXT("%s: ServerProcessShootInput: TotalRotation=%s"), *GetName(), *TotalRotation.ToCompactString());

	AddStroke();
	bCanFlick = false;

	if (!IsLocalController())
	{
		TotalRotation = InTotalRotation;
	}

	if (auto GolfPlayerState = GetGolfPlayerState(); ensure(GolfPlayerState))
	{
		GolfPlayerState->SetReadyForShot(false);
	}
}

bool AGolfPlayerController::HandleOutOfBounds()
{
	// only called on server
	check(HasAuthority());

	if (bOutOfBounds)
	{
		return false;
	}

	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: HandleOutOfBounds"), *GetName());

	bOutOfBounds = true;

	// Make sure we don't process this is as a normal shot 
	GolfControllerCommonComponent->EndTurn();

	ClientHandleOutOfBounds();

	if(auto World = GetWorld(); ensure(World))
	{
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, this, &ThisClass::ResetShotAfterOutOfBounds, OutOfBoundsDelayTime);
	}

	return true;
}

void AGolfPlayerController::ClientHandleOutOfBounds_Implementation()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: ClientHandleOutOfBounds_Implementation"), *GetName());

	bOutOfBounds = true;
	if (auto HUD = GetHUD<APGHUD>(); ensure(HUD))
	{
		HUD->DisplayMessageWidget(EMessageWidgetType::OutOfBounds);
	}
}

void AGolfPlayerController::ResetShotAfterOutOfBounds()
{
	// Only called on server
	check(HasAuthority());

	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: ResetShotAfterOutOfBounds"), *GetName());

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
	if(ensureMsgf(LastShotOptional, TEXT("%s-%s: ResetShotAfterOutOfBounds - ShotHistory is empty"),
		*GetName(), *PaperGolfPawn->GetName()))
	{
		const auto& ResetPosition = LastShotOptional->Position;
		if (!IsLocalController())
		{
			SetPositionTo(ResetPosition);
			DoActivateTurn();
		}

		ClientResetShotAfterOutOfBounds(ResetPosition);
		PaperGolfPawn->MulticastReliableSetTransform(ResetPosition, true, PaperGolfPawn->GetActorRotation());
	}
	// TODO: What to do if we bail out early as user will still have the HUD message for out of bounds displaying
}

void AGolfPlayerController::ClientResetShotAfterOutOfBounds_Implementation(const FVector_NetQuantize& Position)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: ClientResetShotAfterOutOfBounds - Position=%s"), *GetName(), *Position.ToCompactString());

	bOutOfBounds = false;
	bTurnActivated = false;

	SetPositionTo(Position);

	if (auto HUD = GetHUD<APGHUD>(); ensure(HUD))
	{
		HUD->RemoveActiveMessageWidget();
	}

	DoActivateTurn();
}

APaperGolfPawn* AGolfPlayerController::GetPaperGolfPawn()
{
	if (IsValid(PlayerPawn))
	{
		return PlayerPawn;
	}

	const auto PaperGolfPawn = Cast<APaperGolfPawn>(GetPawn());

	if (!ensureMsgf(PaperGolfPawn, TEXT("%s: GetPaperGolfPawn - %s is not a APaperGolfPawn"), *GetName(), *LoggingUtils::GetName(GetPawn())))
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Error, TEXT("%s: GetPaperGolfPawn - %s is not a APaperGolfPawn"), *GetName(), *LoggingUtils::GetName(GetPawn()));
		return nullptr;
	}

	return PaperGolfPawn;
}

void AGolfPlayerController::BeginPlay()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();

	DoBeginPlay([this](auto& GolfSubsystem)
	{
		GolfSubsystem.OnPaperGolfPawnClippedThroughWorld.AddDynamic(this, &ThisClass::OnFellThroughFloor);
	});

	Init();
}

void AGolfPlayerController::OnFellThroughFloor(APaperGolfPawn* InPaperGolfPawn)
{
	DoOnFellThroughFloor(InPaperGolfPawn);
}

void AGolfPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: EndPlay - %s"), *GetName(), *LoggingUtils::GetName(EndPlayReason));

	Super::EndPlay(EndPlayReason);

	CleanupDebugDraw();
}

void AGolfPlayerController::Init()
{	
	// turn is activated manually so we set this to false initially
	bCanFlick = false;

	InitDebugDraw();
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

	const auto PaperGolfPawn = Cast<APaperGolfPawn>(InPawn);

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
	const EShotType NewShotType = static_cast<EShotType>((static_cast<int32>(GetShotType()) + 1) % static_cast<int32>(EShotType::MAX));

	if (NewShotType != EShotType::Default)
	{
		SetShotType(NewShotType);
	}
	else
	{
		SetShotType(EShotType::Full);
	}
}

void AGolfPlayerController::SetShotType(EShotType InShotType)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: SetShotType: %s"), *GetName(), *LoggingUtils::GetName(InShotType));

	ShotType = InShotType;
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
		// Make sure that we execute on the server if this isn't a listen server client
		if (!IsLocalController())
		{
			DoActivateTurn();
		}

		ClientActivateTurn();
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

	if (ShouldEnableInputForActivateTurn())
	{
		SetInputEnabled(true);
	}

	GolfControllerCommonComponent->BeginTurn();

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

	if (!PaperGolfPawn->IsAtRest())
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: DoActivateTurn - Resetting shot state as paper golf pawn is not at rest"),
			*GetName(), *LoggingUtils::GetName(PaperGolfPawn));
		// Force reset of physics state to avoid triggering assertion
		PaperGolfPawn->SetUpForNextShot();
	}

	SetupNextShot(true);
	bTurnActivated = true;

	PaperGolfPawn->SetReadyForShot(true);
}

bool AGolfPlayerController::ShouldEnableInputForActivateTurn() const
{
	auto GameInstance = GetGameInstance();
	if (!ensure(GameInstance))
	{
		return false;
	}

	auto TutorialTrackingSubsystem = GameInstance->GetSubsystem<UTutorialTrackingSubsystem>();

	if(!ensure(TutorialTrackingSubsystem))
	{
		return true;
	}

	auto World = GetWorld();
	if (!ensure(World))
	{
		return false;
	}

	auto GolfGameState = World->GetGameState<APaperGolfGameStateBase>();
	if (!ensure(GolfGameState))
	{
		return false;
	}

	return TutorialTrackingSubsystem->IsHoleFlybySeen(GolfGameState->GetCurrentHoleNumber());
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

	//DisableInput(this);

	// Turn off collision on our own pawn
	if (IsValid(PlayerPawn))
	{
		PlayerPawn->SetCollisionEnabled(false);
	}

	SetInputEnabled(false);
	bTurnActivated = false;
	GolfControllerCommonComponent->EndTurn();
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
		UE_VLOG_UELOG(this, LogPGPlayer, Warning, TEXT("%s: SetCameraToViewPawn: NULL - using default player controller view target"),
			*GetName()
		);
	}
	else
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Display, TEXT("%s: SetCameraToViewPawn: Tracking player pawn %s"),
			*GetName(),
			*InPawn->GetName()
		);
	}

	// TODO: Consider using SetViewTargetWithBlend
	// Whatever the InPawn player is seeing, we will also see
	SetViewTarget(InPawn);
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

	Snapshot->Status.Add(Category);

	// TODO: Consider moving logic to PlayerState itself
	if (auto GolfPlayerState = GetGolfPlayerState(); GolfPlayerState)
	{
		FVisualLogStatusCategory PlayerStateCategory;
		PlayerStateCategory.Category = FString::Printf(TEXT("PlayerState"));

		PlayerStateCategory.Add(TEXT("Shots"), FString::Printf(TEXT("%d"), GolfPlayerState->GetShots()));
		PlayerStateCategory.Add(TEXT("TotalShots"), FString::Printf(TEXT("%d"), GolfPlayerState->GetTotalShots()));
		PlayerStateCategory.Add(TEXT("IsReadyForShot"), LoggingUtils::GetBoolString(GolfPlayerState->IsReadyForShot()));

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

void AGolfPlayerController::CleanupDebugDraw()
{
	GetWorldTimerManager().ClearTimer(VisualLoggerTimer);
}

void AGolfPlayerController::DestroyPawn()
{
	if (!HasAuthority())
	{
		return;
	}
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: DestroyPawn: %s"), *GetName(), *LoggingUtils::GetName(PlayerPawn));

	if (!IsValid(PlayerPawn))
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Warning, TEXT("%s: DestroyPawn - PlayerPawn is NULL"), *GetName());
		return;
	}

	GolfControllerCommonComponent->DestroyPawn();

	PlayerPawn = nullptr;
}

#else

void AGolfPlayerController::InitDebugDraw() {}
void AGolfPlayerController::CleanupDebugDraw() {}

#endif

#pragma endregion Visual Logger