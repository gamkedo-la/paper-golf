// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Volume/OutOfBoundsVolume.h"

#include "Pawn/PaperGolfPawn.h"
#include "Subsystems/GolfEventsSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OutOfBoundsVolume)

void AOutOfBoundsVolume::OnPaperGolfPawnOverlap(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents)
{
	GolfEvents.OnPaperGolfPawnEnteredHazard.Broadcast(&PaperGolfPawn, EHazardType::OutOfBounds);
}
