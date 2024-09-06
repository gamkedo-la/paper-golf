// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/PawnCameraLookComponent.h"

#include "GameFramework/SpringArmComponent.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PGPawnLogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PawnCameraLookComponent)

UPawnCameraLookComponent::UPawnCameraLookComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPawnCameraLookComponent::AddCameraRelativeRotation(const FRotator& DeltaRotation)
{
	if (!ensure(CameraSpringArm))
	{
		return;
	}

	FRotator RelativeRotation = CameraSpringArm->GetRelativeRotation();
	RelativeRotation.Pitch = FMath::ClampAngle(RelativeRotation.Pitch + DeltaRotation.Pitch, MinCameraRotation.Pitch, MaxCameraRotation.Pitch);
	RelativeRotation.Yaw = FMath::ClampAngle(RelativeRotation.Yaw + DeltaRotation.Yaw, MinCameraRotation.Yaw, MaxCameraRotation.Yaw);

	CameraSpringArm->SetRelativeRotation(RelativeRotation);

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, VeryVerbose, TEXT("%s-%s: AddCameraRelativeRotation: %s -> %s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		*DeltaRotation.ToCompactString(), *CameraSpringArm->GetRelativeRotation().ToCompactString());
}

void UPawnCameraLookComponent::ResetCameraRelativeRotation()
{
	if (!ensure(CameraSpringArm))
	{
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, VeryVerbose, TEXT("%s-%s: ResetCameraRelativeRotation: %s -> %s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		*CameraSpringArm->GetRelativeRotation().ToCompactString(), *InitialSpringArmRotation.ToCompactString());

	CameraSpringArm->SetRelativeRotation(InitialSpringArmRotation);
	CameraSpringArm->TargetArmLength = InitialCameraSpringArmLength;
}

void UPawnCameraLookComponent::AddCameraZoomDelta(float ZoomDelta)
{
	if (!ensure(CameraSpringArm))
	{
		return;
	}

	const auto NewTargetArmLength = FMath::Clamp(CameraSpringArm->TargetArmLength + ZoomDelta, MinCameraSpringArmLength, MaxCameraSpringArmLength);

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, VeryVerbose, TEXT("%s-%s: AddCameraZoomDelta: %f -> %f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), CameraSpringArm->TargetArmLength, NewTargetArmLength);

	CameraSpringArm->TargetArmLength = NewTargetArmLength;
}

void UPawnCameraLookComponent::Initialize(USpringArmComponent& InCameraSpringArm)
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: Initialize: InCameraSpringArm=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), 
		*InCameraSpringArm.GetName());

	CameraSpringArm = &InCameraSpringArm;

	InitialSpringArmRotation = CameraSpringArm->GetRelativeRotation();
	InitialCameraSpringArmLength = CameraSpringArm->TargetArmLength;
}

void UPawnCameraLookComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: BeginPlay"),
		*LoggingUtils::GetName(GetOwner()), *GetName());
}

