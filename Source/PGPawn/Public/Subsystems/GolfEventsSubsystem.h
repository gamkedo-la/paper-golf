// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GolfEventsSubsystem.generated.h"

UENUM(BlueprintType)
enum class EHazardType : uint8
{
	OutOfBounds,
	Water,
	Glue
};

class APaperGolfPawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPaperGolfPawnEnteredHazard, APaperGolfPawn*, PaperGolfPawn, EHazardType, HazardType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaperGolfPawnClippedThroughWorld, APaperGolfPawn*, PaperGolfPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaperGolfPawnScored, APaperGolfPawn*, PaperGolfPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaperGolfShotFinished, APaperGolfPawn*, PaperGolfPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPaperGolfNextHole);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPaperGolfCourseComplete);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaperGolfStartHole, int32, HoleNumber);

/**
 * 
 */
UCLASS()
class PGPAWN_API UGolfEventsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfPawnEnteredHazard OnPaperGolfPawnEnteredHazard;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfPawnClippedThroughWorld OnPaperGolfPawnClippedThroughWorld;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfPawnScored OnPaperGolfPawnScored;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfShotFinished OnPaperGolfShotFinished;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfNextHole OnPaperGolfNextHole;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfStartHole OnPaperGolfStartHole;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfCourseComplete OnPaperGolfCourseComplete;
};
