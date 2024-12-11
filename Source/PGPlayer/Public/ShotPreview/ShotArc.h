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


USTRUCT()
struct FShotArcData
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<USplineMeshComponent> SplineMesh{};
};

UCLASS()
class PGPLAYER_API AShotArc : public AActor
{
	GENERATED_BODY()
	
public:	
	AShotArc();

	void SetData(const FPredictProjectilePathResult& Path, bool bDrawHit);

protected:
	virtual void BeginPlay() override;

private:
	void CalculateArcPoints(const FPredictProjectilePathResult& Path, bool bDrawHit);

	void BuildSpline();
	void BuildMeshAlongSpline();
	void UpdateLandingMesh();

	static void SetStaticMeshProperties(UStaticMeshComponent& MeshComponent);

	void ClearSplineMeshes();

	void ClearPreviousState();

	float CalculateMeshSize() const;

	USplineMeshComponent* GetOrCreateSplineComponent(int32 Index);

private:
	struct FLandingInfo
	{
		FVector Location;
		FVector Normal;
	};

private:

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	TObjectPtr<USplineComponent> SplineComponent{};

	UPROPERTY(EditDefaultsOnly, Category = "Mesh")
	TObjectPtr<UStaticMesh> SplineMesh{};

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LandingMeshComponent{};

	UPROPERTY(EditDefaultsOnly, Category = "Mesh")
	TEnumAsByte<ESplineMeshAxis::Type> MeshForwardAxis{ ESplineMeshAxis::Type::X };

	UPROPERTY(EditDefaultsOnly, Category = "Mesh")
	FVector LandingMeshUpVector { FVector::ZAxisVector };
	
	UPROPERTY(EditDefaultsOnly, Category = "Shot Arc")
	int32 MaxSplineMeshes { 20 };
	
	UPROPERTY(Transient)
	TArray<FShotArcData> ArcElements{};

	UPROPERTY(Category = "Shot Arc", EditDefaultsOnly)
	float HitRadiusSize{ 100.0f };

	float SplineMeshSize{ -1.0f };
	float LandingMeshSize { -1.0f };

	TArray<FVector> ArcPoints;
	TOptional<FLandingInfo> LandingData{};
};
