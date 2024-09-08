// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"

#include "Engine/EngineTypes.h"
#include "Interfaces/PawnCameraLook.h"
#include "GolfShotSpectatorPawn.generated.h"

class APaperGolfPawn;
class UPawnCameraLookComponent;
class USpringArmComponent;
class UCameraComponent;

/**
 * 
 */
UCLASS()
class PGPAWN_API AGolfShotSpectatorPawn : public ASpectatorPawn, public IPawnCameraLook
{
	GENERATED_BODY()
	
public:
	AGolfShotSpectatorPawn();

	void TrackPlayer(const APaperGolfPawn* PlayerPawn);

	virtual void AddCameraRelativeRotation(const FRotator& DeltaRotation) override;
	virtual void ResetCameraRelativeRotation() override;
	virtual void AddCameraZoomDelta(float ZoomDelta) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SetCameraLag(bool bEnableLag);

	void OnPlayerTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

private:

	UPROPERTY(Category = "Camera", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraSpringArm{};

	UPROPERTY(Category = "Camera", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPawnCameraLookComponent> CameraLookComponent{};

	TWeakObjectPtr<const APaperGolfPawn> TrackedPlayerPawn{};
};
