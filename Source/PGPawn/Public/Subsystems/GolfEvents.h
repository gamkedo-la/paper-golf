// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include "GolfEvents.generated.h"

UENUM(BlueprintType)
enum class EHazardType : uint8
{
	OutOfBounds,
	Water
};

class APaperGolfPawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPaperGolfPawnEnteredHazard, APaperGolfPawn*, PaperGolfPawn, EHazardType, HazardType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaperGolfPawnClippedThroughWorld, APaperGolfPawn*, PaperGolfPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaperGolfPawnScored, APaperGolfPawn*, PaperGolfPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaperGolfShotFinished, APaperGolfPawn*, PaperGolfPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPaperGolfNextHole);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPaperGolfCourseComplete);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaperGolfStartHole, int32, HoleNumber);
