// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "BasePaperGolfVolume.h"
#include "FellThroughWorldVolume.generated.h"

/**
 * Volume for falling through world.  This is an addition to the "Kill-Z" defined in world that is handled in the pawn.
 * Overlap Type is hidden as "End" doesn't make sense for this volume type and should always be "Any".
 */
UCLASS(HideCategories = ("Overlap Type"))
class PGGAMEPLAY_API AFellThroughWorldVolume : public ABasePaperGolfVolume
{
	GENERATED_BODY()
	
protected:
	virtual void OnConditionTriggered(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents) override;
};
