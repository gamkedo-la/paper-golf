// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "PositionSpeedStruct.h"
#include "MovingObstacle.generated.h"

UCLASS()
class PGGAMEPLAY_API AMovingObstacle : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMovingObstacle();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty >& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMeshComponent> staticMesh{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USplineComponent> movementPath{};

	UPROPERTY(EditAnywhere)
	float distance = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float speed = 1;

	virtual void Tick(float DeltaTime) override;

	void EvaluatePosition();

	void SetDistance();

protected:
	virtual void BeginPlay() override;

private:
	void EnableTick(bool bEnabled);
	void Init();

private:

	FVector InitialRelativeLocation{ EForceInit::ForceInitToZero };
	FQuat InitialRelativeRotation{ EForceInit::ForceInit };

	float valueToAdd = 1;

	UPROPERTY(EditAnywhere)
	bool bReverseDirection{};
};