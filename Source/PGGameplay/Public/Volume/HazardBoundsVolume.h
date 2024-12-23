// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "BasePaperGolfVolume.h"
#include "Subsystems/GolfEvents.h"

#include "Obstacles/PenaltyHazard.h"

#include "HazardBoundsVolume.generated.h"


/**
 * Volume for out of bounds which when triggered will result in player re-hitting with a one stroke penalty.
 */
UCLASS()
class PGGAMEPLAY_API AHazardBoundsVolume : public ABasePaperGolfVolume, public IPenaltyHazard
{
	GENERATED_BODY()

public:
	AHazardBoundsVolume();

	UFUNCTION(BlueprintPure)
	virtual EHazardType GetHazardType() const override { return HazardType; }

	virtual bool IsPenaltyOnImpact() const override { return Type == EPaperGolfVolumeOverlapType::Any; }

protected:
	virtual void OnConditionTriggered(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents) override;
	virtual bool CheckEndCondition_Implementation(const APaperGolfPawn* PaperGolfPawn) const override;

private:
	// Used to trigger SFX
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastConditionTriggered(const FVector_NetQuantize& Location);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Hazard")
	EHazardType HazardType{ EHazardType::OutOfBounds };

	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<USoundBase> HazardSound{};
};
