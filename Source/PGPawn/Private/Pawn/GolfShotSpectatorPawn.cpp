// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Pawn/GolfShotSpectatorPawn.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PGPawnLogging.h"

#include "Pawn/PaperGolfPawn.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/PawnCameraLookComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfShotSpectatorPawn)

AGolfShotSpectatorPawn::AGolfShotSpectatorPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	bAddDefaultMovementBindings = false;

	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));

	CameraSpringArm->TargetArmLength = 1000.0;
	CameraSpringArm->bInheritYaw = true;
	CameraSpringArm->bInheritPitch = false;
	CameraSpringArm->bInheritRoll = false;

	// False by default so that it snaps to the target immediately
	SetCameraLag(false);

	CameraSpringArm->CameraLagSpeed = 2.0f;
	CameraSpringArm->CameraRotationLagSpeed = 2.0f;
	CameraSpringArm->TargetOffset = FVector(0, 0, 100);

	CameraSpringArm->SetupAttachment(RootComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraSpringArm);

	CameraLookComponent = CreateDefaultSubobject<UPawnCameraLookComponent>(TEXT("CameraLook"));
}

void AGolfShotSpectatorPawn::TrackPlayer(const APaperGolfPawn* PlayerPawn)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: TrackPlayer=%s"), *GetName(), *LoggingUtils::GetName(PlayerPawn));

	TrackedPlayerPawn = PlayerPawn;

	if (!PlayerPawn)
	{
		return;
	}

	SetCameraLag(false);
	ResetCameraRelativeRotation();

	SetActorTransform(PlayerPawn->GetActorTransform());

	if (auto PC = Cast<APlayerController>(GetController()); PC)
	{
		PC->SetViewTarget(this);
	}

	// reinitialize
	CameraLookComponent->Initialize(*CameraSpringArm);

	// Need to do on next tick to avoid camera lag on setting view target which is very nauseating to the player
	// We enable camera lag for the look controls
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::SetCameraLag, true));
}

void AGolfShotSpectatorPawn::AddCameraRelativeRotation(const FRotator& DeltaRotation)
{
	CameraLookComponent->AddCameraRelativeRotation(DeltaRotation);
}

void AGolfShotSpectatorPawn::ResetCameraRelativeRotation()
{
	CameraLookComponent->ResetCameraRelativeRotation();
}

void AGolfShotSpectatorPawn::AddCameraZoomDelta(float ZoomDelta)
{
	CameraLookComponent->AddCameraZoomDelta(ZoomDelta);
}

void AGolfShotSpectatorPawn::BeginPlay()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();

	CameraLookComponent->Initialize(*CameraSpringArm);
	SetCameraLag(false);
}

void AGolfShotSpectatorPawn::SetCameraLag(bool bEnableLag)
{
	CameraSpringArm->bEnableCameraRotationLag = CameraSpringArm->bEnableCameraLag = bEnableLag;
}
