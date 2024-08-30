// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "BasePaperGolfVolume.h"
#include "OutOfBoundsVolume.generated.h"


/**
 * Volume for out of bounds which when triggered will result in player re-hitting with a one stroke penalty.
 */
UCLASS()
class PGGAMEPLAY_API AOutOfBoundsVolume : public ABasePaperGolfVolume
{
	GENERATED_BODY()

public:
	AOutOfBoundsVolume();

protected:
	virtual void OnConditionTriggered(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents) override;
	virtual bool CheckEndCondition_Implementation(const APaperGolfPawn* PaperGolfPawn) const override;
};
