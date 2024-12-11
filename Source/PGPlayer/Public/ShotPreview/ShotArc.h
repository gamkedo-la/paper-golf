// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Components/SplineMeshComponent.h"

#include "ShotArc.generated.h"

struct FPredictProjectilePathResult;
class USplineComponent;
class UStaticMesh;
class UStaticMeshComponent;
class USplineMeshComponent;

UCLASS()
class PGPLAYER_API AShotArc : public AActor
{
	GENERATED_BODY()
	
public:	
	AShotArc();

	void SetData(const FPredictProjectilePathResult& Path, bool bDrawHit);

	// Show and Hide with actor visibility

protected:
	virtual void BeginPlay() override;

private:
	void CalculateArcPoints(const FPredictProjectilePathResult& Path, bool bDrawHit);

	void BuildSpline();
	void BuildMeshAlongSpline();
	void UpdateLandingMesh();

	void SetStaticMeshProperties(UStaticMeshComponent& MeshComponent);

	void ClearSplineMeshes();

	void ClearPreviousState();

private:

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	TObjectPtr<USplineComponent> SplineComponent{};

	UPROPERTY(EditDefaultsOnly, Category = "Mesh")
	TObjectPtr<UStaticMesh> SplineMesh{};

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LandingMeshComponent{};

	UPROPERTY(EditDefaultsOnly, Category = "Mesh")
	TEnumAsByte<ESplineMeshAxis::Type> MeshForwardAxis{ ESplineMeshAxis::Type::X };

	UPROPERTY(Transient)
	TArray<USplineMeshComponent*> SplineMeshes{};

	UPROPERTY(Category = "Shot Arc", EditDefaultsOnly)
	float HitRadiusSize{ 100.0f };

	TArray<FVector> ArcPoints;
	TOptional<FVector> HitLocation{};
};
