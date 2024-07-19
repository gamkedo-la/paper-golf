// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Volume/FellThroughWorldVolume.h"

#include "Pawn/PaperGolfPawn.h"
#include "Subsystems/GolfEventsSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FellThroughWorldVolume)

void AFellThroughWorldVolume::OnPaperGolfPawnOverlap(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents)
{
	GolfEvents.OnPaperGolfPawnClippedThroughWorld.Broadcast(&PaperGolfPawn);
}
