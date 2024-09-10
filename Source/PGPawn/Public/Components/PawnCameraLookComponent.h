// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/PawnCameraLook.h"
#include "PawnCameraLookComponent.generated.h"

class USpringArmComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PGPAWN_API UPawnCameraLookComponent : public UActorComponent, public IPawnCameraLook
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPawnCameraLookComponent();

	virtual void AddCameraRelativeRotation(const FRotator& DeltaRotation) override;
	virtual void ResetCameraRelativeRotation() override;
	virtual void AddCameraZoomDelta(float ZoomDelta) override;

	void Initialize(USpringArmComponent& InCameraSpringArm);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Camera | Shot")
	FRotator MinCameraRotation{ -45.0f, -60.0f, 0 };

	UPROPERTY(EditDefaultsOnly, Category = "Camera | Shot")
	FRotator MaxCameraRotation{ 10.0f, 60.0f, 0 };

	UPROPERTY(EditDefaultsOnly, Category = "Camera | Shot", meta = (ClampMin = "0.0"))
	float MinCameraSpringArmLength{ 250.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Camera | Shot", meta = (ClampMin = "0.0"))
	float MaxCameraSpringArmLength{ 10000.0f };
		
	FRotator InitialSpringArmRotation{ EForceInit::ForceInitToZero };
	float InitialCameraSpringArmLength{};

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> CameraSpringArm{};
};
