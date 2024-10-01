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
	if (TrackedPlayerPawn.Get() == PlayerPawn)
	{
		UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: TrackPlayer=%s - Already tracking"), *GetName(), *LoggingUtils::GetName(PlayerPawn));
		return;
	}

	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: TrackPlayer=%s"), *GetName(), *LoggingUtils::GetName(PlayerPawn));

	// remove any previous tracking
	UnTrackPivotComponentTransformUpdates();

	TrackedPlayerPawn = PlayerPawn;

	if (!PlayerPawn)
	{
		return;
	}

	if (auto PlayerRootComponent = PlayerPawn->GetPivotComponent(); PlayerRootComponent)
	{
		UpdateSpectatorTransform(PlayerRootComponent);
	}
	else
	{
		UE_VLOG_UELOG(this, LogPGPawn, Warning, TEXT("%s: PlayerPawn=%s has no pivot component"), *GetName(), *LoggingUtils::GetName(PlayerPawn));
		UpdateSpectatorTransform(PlayerPawn->GetRootComponent());
	}

	TrackPivotComponentTransformUpdates();

	SetCameraLag(false);
	ResetCameraRelativeRotation();

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

void AGolfShotSpectatorPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: EndPlay"), *GetName());

	Super::EndPlay(EndPlayReason);

	UnTrackPivotComponentTransformUpdates();
}

void AGolfShotSpectatorPawn::SetCameraLag(bool bEnableLag)
{
	CameraSpringArm->bEnableCameraRotationLag = CameraSpringArm->bEnableCameraLag = bEnableLag;
}

void AGolfShotSpectatorPawn::OnPlayerTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	if (!ensure(UpdatedComponent))
	{
		return;
	}

	UE_VLOG_UELOG(this, LogPGPawn, VeryVerbose, TEXT("%s: OnPlayerTransformUpdated=%s - %s"), *GetName(), *UpdatedComponent->GetName(), *UpdatedComponent->GetComponentTransform().ToHumanReadableString());
	
	UpdateSpectatorTransform(UpdatedComponent);
}

void AGolfShotSpectatorPawn::UpdateSpectatorTransform(USceneComponent* TrackedPawnComponent)
{
	if (!ensure(TrackedPawnComponent))
	{
		return;
	}

	// Make sure the tracked pawn component is owned by the tracked player pawn
	if (TrackedPawnComponent->GetOwner() != TrackedPlayerPawn.Get())
	{
		UE_VLOG_UELOG(this, LogPGPawn, VeryVerbose, TEXT("%s: UpdateSpectatorTransform - TrackedPawnComponent=%s is owned by %s and not by TrackedPlayerPawn=%s"),
			*GetName(), *TrackedPawnComponent->GetName(), *LoggingUtils::GetName(TrackedPawnComponent->GetOwner()), *LoggingUtils::GetName(TrackedPlayerPawn));
		return;
	}

	UE_VLOG_UELOG(this, LogPGPawn, VeryVerbose, TEXT("%s: UpdateSpectatorTransform=%s->%s - %s"),
		*GetName(), *LoggingUtils::GetName(TrackedPawnComponent->GetOwner()), *TrackedPawnComponent->GetName(), *TrackedPawnComponent->GetComponentTransform().ToHumanReadableString());

	SetActorTransform(TrackedPawnComponent->GetComponentTransform());

	UE_VLOG_LOCATION(this, LogPGPawn, VeryVerbose, TrackedPawnComponent->GetComponentLocation(), 10.0f, FColor::Blue, TEXT("%s: Spectated - %s"),
		*LoggingUtils::GetName(TrackedPawnComponent->GetOwner()),
		*([&] { auto Pawn = Cast<APawn>(TrackedPawnComponent->GetOwner()); return Pawn ? LoggingUtils::GetName<APlayerState>(Pawn->GetPlayerState()) : FString{ TEXT("Unknown") }; }()));
}

void AGolfShotSpectatorPawn::TrackPivotComponentTransformUpdates()
{
	auto PlayerPawn = TrackedPlayerPawn.Get();
	if (!PlayerPawn)
	{
		return;
	}

	if (auto PlayerRootComponent = PlayerPawn->GetPivotComponent(); PlayerRootComponent)
	{
		UE_VLOG_UELOG(this, LogPGPawn, Verbose, TEXT("%s: TrackPivotComponentTransformUpdates=%s -> %s"), *GetName(), *LoggingUtils::GetName(PlayerPawn), *LoggingUtils::GetName(PlayerRootComponent));

		PlayerRootComponent->TransformUpdated.AddUObject(this, &ThisClass::OnPlayerTransformUpdated);
	}
}

void AGolfShotSpectatorPawn::UnTrackPivotComponentTransformUpdates()
{
	auto PlayerPawn = TrackedPlayerPawn.Get();
	if (!PlayerPawn)
	{
		return;
	}

	if (auto PlayerRootComponent = PlayerPawn->GetPivotComponent(); PlayerRootComponent)
	{
		UE_VLOG_UELOG(this, LogPGPawn, Verbose, TEXT("%s: UnTrackPivotComponentTransformUpdates=%s -> %s"), *GetName(), *LoggingUtils::GetName(PlayerPawn), *LoggingUtils::GetName(PlayerRootComponent));

		PlayerRootComponent->TransformUpdated.RemoveAll(this);
	}
}
