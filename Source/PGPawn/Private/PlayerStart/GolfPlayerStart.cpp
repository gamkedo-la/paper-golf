// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "PlayerStart/GolfPlayerStart.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfPlayerStart)

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AGolfPlayerStart::AGolfPlayerStart(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));

	CameraSpringArm->TargetArmLength = 1000.0;
	CameraSpringArm->SetRelativeRotation(FRotator(-30, 120, 0));
	CameraSpringArm->bEnableCameraLag = true;
	CameraSpringArm->bEnableCameraRotationLag = true;
	CameraSpringArm->CameraLagSpeed = 2.0f;
	CameraSpringArm->CameraRotationLagSpeed = 2.0f;
	CameraSpringArm->TargetOffset = FVector(0, 0, 100);

	CameraSpringArm->SetupAttachment(RootComponent);

	// Only enable conditionally
	CameraSpringArm->PrimaryComponentTick.bStartWithTickEnabled = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraSpringArm);
}
