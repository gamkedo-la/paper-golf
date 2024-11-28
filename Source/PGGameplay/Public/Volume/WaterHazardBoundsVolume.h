// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Volume/HazardBoundsVolume.h"
#include "WaterHazardBoundsVolume.generated.h"

class UMaterialInterface;

/**
 * 
 */
UCLASS()
class PGGAMEPLAY_API AWaterHazardBoundsVolume : public AHazardBoundsVolume
{
	GENERATED_BODY()
	
public:
	AWaterHazardBoundsVolume();

protected:
	virtual void BeginPlay() override;

private:
	void SetWaterMaterial(bool bForceUpdate = false);
	void PollWaterMaterial() { SetWaterMaterial(false); }

	bool ShouldUseHighQualityWater() const;

private:
	UPROPERTY(EditAnywhere, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> Mesh{};

	UPROPERTY(EditAnywhere, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> WaterTableMesh{};

	UPROPERTY(EditAnywhere, Category = "Material")
	TObjectPtr<UMaterialInterface> HighQualityWaterMaterial{};

	UPROPERTY(EditAnywhere, Category = "Material")
	TObjectPtr<UMaterialInterface> LowQualityWaterMaterial{};

	UPROPERTY(EditAnywhere, Category = "Material", meta = (ClampMin = "0", ClampMax="3"))
	int32 WaterQualityThreshold{ 2 };

	bool bUsingHighQualityWater{};
};
