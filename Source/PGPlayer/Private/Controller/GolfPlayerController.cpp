// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Controller/GolfPlayerController.h"

#include "Pawn/PaperGolfPawn.h"

#include "Pawn/GolfShotSpectatorPawn.h"
#include "PlayerStart/GolfPlayerStart.h"

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

#include "Debug/PGConsoleVars.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfPlayerController)

AGolfPlayerController::AGolfPlayerController()
{
	bReplicates = true;
	bAlwaysRelevant = true;

	ShotArcPreviewComponent = CreateDefaultSubobject<UShotArcPreviewComponent>(TEXT("ShotArcPreview"));
	GolfControllerCommonComponent = CreateDefaultSubobject<UGolfControllerCommonComponent>(TEXT("GolfControllerCommon"));
}

void AGolfPlayerController::DoReset()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: DoReset"), *GetName());

	FlickZ = 0.f;
	ShotType = EShotType::Default;
	TotalRotation = FRotator::ZeroRotator;
	PlayerPawn = nullptr;
	PlayerStart = nullptr;
	CurrentSpectatorPlayerState = nullptr;
	SpectatingPawnPlayerStateMap.Reset();
	bSpectatorFlicked = false;

	bTurnActivated = false;
	bCanFlick = false;
	bOutOfBounds = false;
	bScored = false;
	bInputEnabled = true; // this is the default constructor value
	PreTurnState = EPlayerPreTurnState::None;

	check(GolfControllerCommonComponent);

	GolfControllerCommonComponent->Reset();

	if (IsLocalController())
	{
		if (auto HUD = GetHUD<APGHUD>(); ensure(HUD))
		{
			HUD->RemoveActiveMessageWidget();
		}
	}
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

	BlueprintResetShot();
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

	GolfControllerCommonComponent->OnScored();
	EndAnyActiveTurn();

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

	const auto RotationToApplyFunc = [&]()
	{
		return DeltaRotation * (RotationRate * GetWorld()->GetDeltaSeconds());
	};

	auto RotationToApply = RotationToApplyFunc();

	UPaperGolfPawnUtilities::ClampDeltaRotation(RotationMax, RotationToApply, TotalRotation);

	UE_VLOG_UELOG(this, LogPGPlayer, VeryVerbose,
		TEXT("%s: AddPaperGolfPawnRelativeRotation - DeltaRotation=%s; RotationToApply=%s; ClampedRotationToApply=%s; TotalRotation=%s"),
		*GetName(), *DeltaRotation.ToCompactString(), *RotationToApplyFunc().ToCompactString(), *RotationToApply.ToCompactString(), *TotalRotation.ToCompactString());

	PaperGolfPawn->AddDeltaRotation(RotationToApply);

	ServerSetPaperGolfPawnRotation(PaperGolfPawn->GetActorRotation());
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

	//SetPositionTo(Position, Rotation);
}

void AGolfPlayerController::DoAdditionalOnShotFinished()
{
	bTurnActivated = false;

	// invoke a client reliable function to say the next shot is ready unless the server is the client
	if (auto PaperGolfPawn = GetPaperGolfPawn(); IsRemoteServer() && PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log,
			TEXT("%s-%s: OnShotFinished - Setting final authoritative position for client pawn: %s"),
			*GetName(), *PaperGolfPawn->GetName(), *PaperGolfPawn->GetPaperGolfPosition().ToCompactString());

		ClientSetTransformTo(PaperGolfPawn->GetPaperGolfPosition(), PaperGolfPawn->GetPaperGolfRotation());
	}
}

void AGolfPlayerController::DoAdditionalFallThroughFloor()
{
	if (auto PaperGolfPawn = GetPaperGolfPawn(); IsRemoteServer() && PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log,
			TEXT("%s-%s: DoAdditionalFallThroughFloor - Setting final authoritative position for client pawn: %s"),
			*GetName(), *PaperGolfPawn->GetName(), *PaperGolfPawn->GetPaperGolfPosition().ToCompactString());

		ClientSetTransformTo(PaperGolfPawn->GetPaperGolfPosition(), PaperGolfPawn->GetPaperGolfRotation());
	}
}

void AGolfPlayerController::Reset()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: Reset"), *GetName());

	Super::Reset();

	DoReset();
}

void AGolfPlayerController::AddPitchInput(float Val)
{
	if (auto PawnCameraLook = GetPawnCameraLook(); PawnCameraLook)
	{
		PawnCameraLook->AddCameraRelativeRotation(FRotator(Val, 0.f, 0.f));
	}
}

void AGolfPlayerController::AddYawInput(float Val)
{
	if (auto PawnCameraLook = GetPawnCameraLook(); PawnCameraLook)
	{
		PawnCameraLook->AddCameraRelativeRotation(FRotator(0.f, Val, 0.f));
	}
}

bool AGolfPlayerController::IsSpectatingShotSetup() const
{
	return CurrentSpectatorPlayerState.IsValid() && !bSpectatorFlicked;
}

void AGolfPlayerController::ResetCameraRotation()
{
	if (auto PawnCameraLook = GetPawnCameraLook(); PawnCameraLook)
	{
		PawnCameraLook->ResetCameraRelativeRotation();
	}
}

void AGolfPlayerController::AddCameraZoomDelta(float ZoomDelta)
{
	if (auto PawnCameraLook = GetPawnCameraLook(); PawnCameraLook)
	{
		PawnCameraLook->AddCameraZoomDelta(ZoomDelta);
	}
}

void AGolfPlayerController::ClientReset_Implementation()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: ClientReset_Implementation"), *GetName());
	
	Super::ClientReset_Implementation();

	if (IsLocalServer())
	{
		return;
	}

	DoReset();
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

	HUD->BeginShot();

	UE_VLOG_UELOG(this, LogPGPlayer, Log,
		TEXT("%s: ProcessShootInput - Calling Flick"),
		*GetName());

	const auto Power = GolfWidget->GetMeterPower();
	const auto Accuracy = GetAdjustedAccuracy(GolfWidget->GetMeterAccuracy());

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

float AGolfPlayerController::GetAdjustedAccuracy(float Accuracy) const
{
	const float AdjustedAccuracy = FMath::Sign(Accuracy) * FMath::Min(MaxAccuracy, FMath::Pow(FMath::Abs(Accuracy), AccuracyAdjustmentExponent));

	UE_VLOG_UELOG(this, LogPGPlayer, Log,
		TEXT("%s: GetAdjustedAccuracy: %f -> %f"),
		*GetName(), Accuracy, AdjustedAccuracy);

	return AdjustedAccuracy;
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

void AGolfPlayerController::ServerSetPaperGolfPawnRotation_Implementation(const FRotator& InTotalRotation)
{
	if (IsLocalController())
	{
		return;
	}

	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: ServerSetPaperGolfPawnRotation - InTotalRotation=%s"), *GetName(), *InTotalRotation.ToCompactString());

	TotalRotation = InTotalRotation;

	if (auto PaperGolfPawn = GetPaperGolfPawn(); PaperGolfPawn)
	{
		PaperGolfPawn->SetActorRotation(InTotalRotation);
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
}

void AGolfPlayerController::Init()
{
	// turn is activated manually so we set this to false initially
	bCanFlick = false;

	InitFromConsoleVariables();

	if (auto World = GetWorld(); ensure(World))
	{
		if (auto GolfEventsSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventsSubsystem))
		{
			GolfEventsSubsystem->OnPaperGolfNextHole.AddUniqueDynamic(this, &ThisClass::OnHoleComplete);
		}
	}
}

void AGolfPlayerController::InitFromConsoleVariables()
{
#if PG_DEBUG_ENABLED
	if (const auto OverridePlayerAccuracyExponent = PG::CPlayerAccuracyExponent.GetValueOnGameThread(); OverridePlayerAccuracyExponent > 1.0f)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log,  TEXT("%s: InitFromConsoleVariables - Overriding PlayerAccuracyExponent %f -> %f"),
			*GetName(), AccuracyAdjustmentExponent, OverridePlayerAccuracyExponent);
		AccuracyAdjustmentExponent = OverridePlayerAccuracyExponent;
	}
	if(const auto OverridePlayerMaxAccuracy = PG::CPlayerMaxAccuracy.GetValueOnGameThread(); OverridePlayerMaxAccuracy >= 0.0f && OverridePlayerMaxAccuracy <= 1.0f)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log,  TEXT("%s: InitFromConsoleVariables - Overriding PlayerMaxAccuracy %f -> %f"),
			*GetName(), MaxAccuracy, OverridePlayerMaxAccuracy);
		MaxAccuracy = OverridePlayerMaxAccuracy;
	}
#endif
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

void AGolfPlayerController::PawnPendingDestroy(APawn* InPawn)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: PawnPendingDestroy - InPawn=%s"), *GetName(), *LoggingUtils::GetName(InPawn));

	Super::PawnPendingDestroy(InPawn);

	if (!HasAuthority())
	{
		// Pawn destructions come in as unposess on clients after scoring - be sure to end any active turn
		EndAnyActiveTurn();
		// spectate current golf hole until a new spectate target is assigned 
		SpectateCurrentGolfHole();
	}
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
		if (PreTurnState == EPlayerPreTurnState::CameraIntroductionRequested)
		{
			DoPlayerCameraIntroduction();
		}

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

#pragma region Start hole logic

void AGolfPlayerController::ReceivePlayerStart(AActor* InPlayerStart)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: ReceivePlayerStart - PlayerStart=%s"), *GetName(), *LoggingUtils::GetName(InPlayerStart));

	PlayerStart = InPlayerStart;
}

void AGolfPlayerController::StartHole()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: StartHole"), *GetName());

	ClientStartHole(PlayerStart);
}

void AGolfPlayerController::ClientStartHole_Implementation(AActor* InPlayerStart)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: ClientStartHole: PlayerStart=%s"), *GetName(), *LoggingUtils::GetName(InPlayerStart));

	PlayerStart = InPlayerStart;

	// This is where we need to look to grab the current hole, play the hole flyby if it hasn't been seen, and then do the camera introduction
	// We also need to sync when the current hole number replicates
	check(GolfControllerCommonComponent);

	GolfControllerCommonComponent->SyncHoleChanged(FSimpleDelegate::CreateUObject(this, &ThisClass::TriggerHoleFlybyAndPlayerCameraIntroduction));
}

#pragma endregion Start hole logic

void AGolfPlayerController::TriggerHoleFlybyAndPlayerCameraIntroduction()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: TriggerHoleFlybyAndPlayerCameraIntroduction"), *GetName());

	if (!IsHoleFlybySeen())
	{
		const auto GolfHole = AGolfHole::GetCurrentHole(this);
		if (ensure(GolfHole))
		{
			// TODO: Trigger the hole flyby sequence on the GolfHole and then 
			// have a callback for when it completes to trigger the player introduction
			MarkHoleFlybySeen();
			TriggerPlayerCameraIntroduction();
		}
		else
		{
			UE_VLOG_UELOG(this, LogPGPlayer, Error, TEXT("%s: TriggerHoleFlybyAndPlayerCameraIntroduction - GolfHole is NULL"), *GetName());

			MarkHoleFlybySeen();
			TriggerPlayerCameraIntroduction();
		}
	}
	else
	{
		TriggerPlayerCameraIntroduction();
	}
}

void AGolfPlayerController::OnHoleComplete()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: OnHoleComplete"), *GetName());

	if (!IsLocalController())
	{
		return;
	}

	SpectateCurrentGolfHole();
}

void AGolfPlayerController::SpectateCurrentGolfHole()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: SpectateCurrentGolfHole"), *GetName());

	// Fine to re-target the golf hole as the camera manager doesn't interrupt a transition to the same target
	if (auto GolfHole = AGolfHole::GetCurrentHole(this); GolfHole)
	{
		SetViewTargetWithBlend(GolfHole, HoleCameraCutTime, EViewTargetBlendFunction::VTBlend_EaseInOut, HoleCameraCutExponent);
	}
	else
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Warning, TEXT("%s: OnHoleComplete - GolfHole is NULL - skipping view target"), *GetName());
	}
}

void AGolfPlayerController::TriggerPlayerCameraIntroduction()
{
	const auto GolfPlayerStart = Cast<AGolfPlayerStart>(PlayerStart);

	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: TriggerPlayerCameraIntroduction: GolfPlayerStart=%s"), *GetName(), *LoggingUtils::GetName(GolfPlayerStart));

	if (!GolfPlayerStart)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Warning, TEXT("%s: TriggerPlayerCameraIntroduction - GolfPlayerStart is NULL - skipping"), *GetName());
		MarkFirstPlayerTurnReady();
	}
	else if(PlayerPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: TriggerPlayerCameraIntroduction - PlayerPawn is already valid - triggering immediately"), *GetName());
		DoPlayerCameraIntroduction();
	}
	else
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: TriggerPlayerCameraIntroduction - PlayerPawn is NULL - waiting for SetPawn"), *GetName());
		PreTurnState = EPlayerPreTurnState::CameraIntroductionRequested;
	}
}

void AGolfPlayerController::DoPlayerCameraIntroduction()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: DoPlayerCameraIntroduction"), *GetName());

	const auto GolfPlayerStart = Cast<AGolfPlayerStart>(PlayerStart);
	// function should not have been called if player start wasn't a golf player start
	if (!ensure(GolfPlayerStart))
	{
		PreTurnState = EPlayerPreTurnState::None;
		return;
	}

	SetInputEnabled(false);

	PreTurnState = EPlayerPreTurnState::CameraIntroductionPlaying;

	SetViewTarget(GolfPlayerStart);
	SetViewTargetWithBlend(PlayerPawn, PlayerCameraIntroductionTime, EViewTargetBlendFunction::VTBlend_EaseInOut, PlayerCameraIntroductionBlendExp, false);

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::MarkFirstPlayerTurnReady, PlayerCameraIntroductionTime);
}

void AGolfPlayerController::MarkFirstPlayerTurnReady()
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: MarkFirstPlayerTurnReady"), *GetName());

	PreTurnState = EPlayerPreTurnState::None;
	if (bTurnActivated)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: MarkFirstPlayerTurnReady - Setting input enabled"), *GetName());

		SetInputEnabled(true);
	}
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
		if (PlayerState)
		{
			PlayerState->SetIsSpectator(false);
		}

		Possess(PlayerPawn);
		// Turn activation will happen on SetPawn after possession has replicated
		// TODO: May need to avoid having Possess set camera target (or do that in SetPawn) if hole flyby is playing or we are doing player camera introduction
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

	CurrentSpectatorPlayerState = nullptr;
	bSpectatorFlicked = false;

	if (ShouldEnableInputForActivateTurn())
	{
		SetInputEnabled(true);
	}
	else
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: DoActivateTurn - Skipping input activation until after introductions completed"), *GetName());
		SetInputEnabled(false);
	}

	// TODO: If we just started the hole and need to see the hole flyby and then the player camera introduction then we'd want to defer the rest of these tasks until after they complete

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

	GolfControllerCommonComponent->BeginTurn();

	SetupNextShot(true);
	bTurnActivated = true;

	PaperGolfPawn->SetReadyForShot(true);

	BlueprintActivateTurn();

	if (IsLocalController())
	{
		if (auto HUD = GetHUD<APGHUD>(); ensure(HUD))
		{
			HUD->BeginTurn();
		}
	}
}

bool AGolfPlayerController::ShouldEnableInputForActivateTurn() const
{
	return IsHoleFlybySeen() && !CameraIntroductionInProgress();
}

bool AGolfPlayerController::IsHoleFlybySeen() const
{
	auto GameInstance = GetGameInstance();
	if (!ensure(GameInstance))
	{
		return false;
	}

	auto TutorialTrackingSubsystem = GameInstance->GetSubsystem<UTutorialTrackingSubsystem>();

	if (!ensure(TutorialTrackingSubsystem))
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

void AGolfPlayerController::MarkHoleFlybySeen()
{
	auto GameInstance = GetGameInstance();
	if (!ensure(GameInstance))
	{
		return;
	}

	auto TutorialTrackingSubsystem = GameInstance->GetSubsystem<UTutorialTrackingSubsystem>();

	if (!ensure(TutorialTrackingSubsystem))
	{
		return;
	}

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	auto GolfGameState = World->GetGameState<APaperGolfGameStateBase>();
	if (!ensure(GolfGameState))
	{
		return;
	}

	TutorialTrackingSubsystem->MarkHoleFlybySeen(GolfGameState->GetCurrentHoleNumber());
}

void AGolfPlayerController::Spectate(APaperGolfPawn* InPawn, AGolfPlayerState* InPlayerState)
{
	// This can only be called on server
	if(!ensureAlwaysMsgf(HasAuthority(), TEXT("%s: Spectate - InPawn=%s; InPlayerState=%s called on client!"), 
		*GetName(), *LoggingUtils::GetName(InPawn), InPlayerState ? *InPlayerState->GetPlayerName() : TEXT("NULL")))
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Error, TEXT("%s: Spectate - InPawn=%s; InPlayerState=%s called on client!"),
			*GetName(), *LoggingUtils::GetName(InPawn), InPlayerState ? *InPlayerState->GetPlayerName() : TEXT("NULL"));
		return;
	}

	UE_VLOG_UELOG(this, LogPGPlayer, Display, TEXT("%s: Spectate - InPawn=%s; InPlayerState=%s"), 
		*GetName(), *LoggingUtils::GetName(InPawn), InPlayerState ? *InPlayerState->GetPlayerName() : TEXT("NULL"));

	// TODO: Hide the player pawn and UI and switch to spectate the input player pawn
	// Need to account for possibly destroying the player pawn after scoring so the player pawn could be null
	// Should track the possessed paper golf pawn as need to switch back to it when activating the turn

	// Allow the spectator pawn to take over the controls; otherwise, some of the bindings will be disabled
	if (PlayerState)
	{
		PlayerState->SetIsSpectator(true);
	}

	CurrentSpectatorPlayerState = InPlayerState;
	bSpectatorFlicked = false;

	ClientSpectate(InPawn, InPlayerState);

	SpectatePawn(InPawn, InPlayerState);
}

void AGolfPlayerController::ClientSpectate_Implementation(APaperGolfPawn* InPawn, AGolfPlayerState* InPlayerState)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: ClientSpectate - InPawn=%s; InPlayerState=%s"),
		*GetName(), *LoggingUtils::GetName(InPawn), InPlayerState ? *InPlayerState->GetPlayerName() : TEXT("NULL"));

	CurrentSpectatorPlayerState = InPlayerState;
	bSpectatorFlicked = false;

	// Turn off collision on our own pawn
	if (IsValid(PlayerPawn))
	{
		PlayerPawn->SetCollisionEnabled(false);
	}

	EndAnyActiveTurn();

	BlueprintSpectate(InPawn, InPlayerState);

	if (auto HUD = GetHUD<APGHUD>(); ensure(HUD))
	{
		HUD->SpectatePlayer(InPawn, InPlayerState);

		if (InPawn)
		{
			OnHandleSpectatorShot(InPlayerState, InPawn);
		}
		else if(InPlayerState)
		{
			// On simulated proxies, the InPawn is null if it was first spawned on the server as the spawning may not have replicated yet,
			// so need to listen for when the player state is replicated on the pawn and then can trigger the OnFlick logic
			// There is a chance that the flick event could happen before the state change replicates, but the issue would be cosmetic
			InPlayerState->OnPawnSet.AddUniqueDynamic(this, &ThisClass::OnSpectatorShotPawnSet);
		}
	}
}

void AGolfPlayerController::OnSpectatorShotPawnSet(APlayerState* InPlayer, APawn* InNewPawn, APawn* InOldPawn)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: OnSpectatorShotPawnSet - InPlayer=%s; InNewPawn=%s; InOldPawn=%s"),
		*GetName(), *LoggingUtils::GetName(InPlayer), *LoggingUtils::GetName(InNewPawn), *LoggingUtils::GetName(InOldPawn));

	OnHandleSpectatorShot(Cast<AGolfPlayerState>(InPlayer), Cast<APaperGolfPawn>(InNewPawn));

	if (InPlayer)
	{
		InPlayer->OnPawnSet.RemoveDynamic(this, &ThisClass::OnSpectatorShotPawnSet);
	}
}

void AGolfPlayerController::OnSpectatedPawnDestroyed(AActor* InPawn)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: OnSpectatedPawnDestroyed - InPawn=%s"),
		*GetName(), *LoggingUtils::GetName(InPawn));

	const auto StrongCurrentSpectatorState = CurrentSpectatorPlayerState.Get();
	if (!StrongCurrentSpectatorState)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Verbose, TEXT("%s: OnSpectatedPawnDestroyed - InPawn=%s - CurrentSpectatorPlayerState is NULL - skipping"),
			*GetName(), *LoggingUtils::GetName(InPawn));
		return;
	}

	const auto MatchedPlayer = [&]()
	{
		const auto MatchedPlayerResult = SpectatingPawnPlayerStateMap.Find(InPawn);
		return MatchedPlayerResult ? MatchedPlayerResult->Get() : nullptr;
	}();

	if (!MatchedPlayer)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: OnSpectatedPawnDestroyed - InPawn=%s - No matching player state found"),
			*GetName(), *LoggingUtils::GetName(InPawn));
		return;
	}

	// If we are spectating the current player state and InNewPawn is NULL it likely means that pawn is destroyed so switch to the golf hole
	if (StrongCurrentSpectatorState == MatchedPlayer)
	{
		SpectateCurrentGolfHole();
	}
	else
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Verbose, TEXT("%s: OnSpectatedPawnDestroyed - InPawn=%s - Skipping as StrongCurrentSpectatorState=%s != MatchedPlayer=%s"),
			*GetName(), *LoggingUtils::GetName(InPawn), *StrongCurrentSpectatorState->GetPlayerName(), *MatchedPlayer->GetPlayerName());
	}
}

IPawnCameraLook* AGolfPlayerController::GetPawnCameraLook() const
{
	APawn* CameraLookPawn;
	if (IsSpectatingShotSetup())
	{
		CameraLookPawn = GetSpectatorPawn();
	}
	else
	{
		CameraLookPawn = GetPawn();
	}

	return Cast<IPawnCameraLook>(CameraLookPawn);
}

void AGolfPlayerController::OnHandleSpectatorShot(AGolfPlayerState* InPlayerState, APaperGolfPawn* InPawn)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: OnHandleSpectatorShot - InPlayerState=%s; InPawn=%s"),
		*GetName(), *LoggingUtils::GetName(InPlayerState), *LoggingUtils::GetName(InPawn));

	if (!InPawn)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Warning, TEXT("%s: OnHandleSpectatorShot - InPlayerState=%s - InPawn is NULL"), *GetName(), *LoggingUtils::GetName(InPlayerState));

		return;
	}

	SpectatingPawnPlayerStateMap.Add(InPawn, InPlayerState);
	InPawn->OnDestroyed.AddUniqueDynamic(this, &ThisClass::OnSpectatedPawnDestroyed);

	if (auto StrongCurrentSpectatorState = CurrentSpectatorPlayerState.Get(); !StrongCurrentSpectatorState || StrongCurrentSpectatorState != InPlayerState)
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Warning, TEXT("%s: OnHandleSpectatorShot - Skipping as spectator status has changed - CurrentSpectatorPlayerState=%s; InPlayerState=%s"),
			*GetName(), StrongCurrentSpectatorState ? *StrongCurrentSpectatorState->GetPlayerName() : TEXT("NULL"), InPlayerState ? *InPlayerState->GetPlayerName() : TEXT("NULL"));
		return;
	}

	// Track the player pawn for the spectator camera
	if (auto MySpectatorPawn = Cast<AGolfShotSpectatorPawn>(GetSpectatorPawn()); IsValid(MySpectatorPawn))
	{
		MySpectatorPawn->TrackPlayer(InPawn);
	}
	else
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: OnHandleSpectatorShot - SpectatorPawn=%s is not a AGolfShotSpectatorPawn"), *GetName(), *LoggingUtils::GetName(GetSpectatorPawn()));
	}

	OnFlickSpectateShotHandle = InPawn->OnFlick.AddWeakLambda(this,
		[this, InPawn = MakeWeakObjectPtr(InPawn), HUD = MakeWeakObjectPtr(GetHUD<APGHUD>()), InPlayerState = MakeWeakObjectPtr(InPlayerState)]()
		{
			bSpectatorFlicked = true;

			// Transition back to viewing pawn after flick
			SetViewTargetWithBlend(InPawn.Get(), SpectatorShotCameraCutTime, EViewTargetBlendFunction::VTBlend_EaseInOut, SpectatorShotCameraCutExponent);

			if (HUD.IsValid())
			{
				HUD->BeginSpectatorShot(InPawn.Get(), InPlayerState.Get());
			}

			// Removing the handle invalidates the delegate so we need to do it last
			const auto HandleToRemove = OnFlickSpectateShotHandle;
			OnFlickSpectateShotHandle.Reset();
			if (InPawn.IsValid())
			{
				InPawn->OnFlick.Remove(OnFlickSpectateShotHandle);
			}
		});
}

void AGolfPlayerController::EndAnyActiveTurn()
{
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

void AGolfPlayerController::SpectatePawn(APawn* PawnToSpectate, AGolfPlayerState* InPlayerState)
{
	if (!HasAuthority())
	{
		// Can only change state to spectator on the server side
		return;
	}

	UE_VLOG_UELOG(this, LogPGPlayer, Display, TEXT("%s: Changing state to spectator"), *GetName());

	ChangeState(NAME_Spectating);
	ClientGotoState(NAME_Spectating);
}

void AGolfPlayerController::SetSpectatorPawn(ASpectatorPawn* NewSpectatorPawn)
{
	UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: SetSpectatorPawn - NewSpectatorPawn=%s"),
		*GetName(), *LoggingUtils::GetName(NewSpectatorPawn));

	Super::SetSpectatorPawn(NewSpectatorPawn);

	if (auto GolfSpectatorPawn = Cast<AGolfShotSpectatorPawn>(NewSpectatorPawn); IsValid(GolfSpectatorPawn))
	{
		EnableInput(this);

		if (auto SpectatorPlayerState = CurrentSpectatorPlayerState.Get(); SpectatorPlayerState && SpectatorPlayerState->GetPawn())
		{
			GolfSpectatorPawn->TrackPlayer(Cast<APaperGolfPawn>(SpectatorPlayerState->GetPawn()));
		}
		else
		{
			UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: SetSpectatorPawn - Pawn has not yet replicated for SpectatorPlayerState=%s"),
				*GetName(), *LoggingUtils::GetName(SpectatorPlayerState));
			SetViewTarget(GolfSpectatorPawn);
		}
	}
	else
	{
		UE_VLOG_UELOG(this, LogPGPlayer, Log, TEXT("%s: SetSpectatorPawn - NewSpectatorPawn=%s is not a AGolfShotSpectatorPawn"),
			*GetName(), *LoggingUtils::GetName(NewSpectatorPawn));
	}
}


#pragma endregion Turn and spectator logic

#pragma region Visual Logger

#if ENABLE_VISUAL_LOG

void AGolfPlayerController::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	Super::GrabDebugSnapshot(Snapshot);

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

	if (auto GolfPlayerState = GetGolfPlayerState(); GolfPlayerState)
	{
		GolfPlayerState->GrabDebugSnapshot(Snapshot);
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

#endif

#pragma endregion Visual Logger
