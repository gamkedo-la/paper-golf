// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Volume/HazardBoundsVolume.h"
#include "WaterHazardBoundsVolume.generated.h"

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
	virtual void OnConditionTriggered(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents) override;

private:
	UPROPERTY(EditAnywhere, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> Mesh{};

	UPROPERTY(EditAnywhere, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> WaterTableMesh{};

	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<USoundBase> SplashSound{};
};
