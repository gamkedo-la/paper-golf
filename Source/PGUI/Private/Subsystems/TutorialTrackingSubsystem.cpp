// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Subsystems/TutorialTrackingSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TutorialTrackingSubsystem)

UTutorialTrackingSubsystem::UTutorialTrackingSubsystem()
{
	HoleFlybySeen.SetNum(MaxHoles);
}

void UTutorialTrackingSubsystem::MarkAllHoleFlybysSeen(bool bSeen)
{
	for (auto& seen : HoleFlybySeen)
	{
		seen = bSeen;
	}
}
