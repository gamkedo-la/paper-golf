// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

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
class PGGAMEPLAY_API ABasePaperGolfVolume : public ATriggerVolume
{
	GENERATED_BODY()

public:
	ABasePaperGolfVolume();

protected:

	virtual void OnPaperGolfPawnOverlap(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents) {}

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Paper Golf Pawn Overlap"))
	void ReceiveOnPaperGolfPawnOverlap(APaperGolfPawn* PaperGolfPawn, UGolfEventsSubsystem* GolfEvents);

private:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override final;
};
