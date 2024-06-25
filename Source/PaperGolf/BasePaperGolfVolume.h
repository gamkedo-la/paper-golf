// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerVolume.h"
#include "BasePaperGolfVolume.generated.h"

class UGolfEventsSubsystem;
class APaperGolfPawn;

/**
 * 
 */
UCLASS(Abstract)
class PAPERGOLF_API ABasePaperGolfVolume : public ATriggerVolume
{
	GENERATED_BODY()

protected:

	virtual void OnPaperGolfPawnOverlap(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents) {}

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Paper Golf Pawn Overlap"))
	void ReceiveOnPaperGolfPawnOverlap(APaperGolfPawn* PaperGolfPawn, UGolfEventsSubsystem* GolfEvents);

private:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override final;
};
