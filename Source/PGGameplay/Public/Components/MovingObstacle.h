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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* staticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USplineComponent* movementPath;

	UPROPERTY(EditAnywhere)
	float distance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float speed = 1;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void EvaluatePosition();

	void SetDistance();

};