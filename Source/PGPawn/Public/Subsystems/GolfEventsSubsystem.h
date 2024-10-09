// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Subsystems/GolfEvents.h"

#include "GolfEventsSubsystem.generated.h"



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

	/*
	* Called when a shot is finished.
	* Currently only broadcast on server and on locally controlled pawns.
	*/
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfShotFinished OnPaperGolfShotFinished;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfNextHole OnPaperGolfNextHole;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfStartHole OnPaperGolfStartHole;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfCourseComplete OnPaperGolfCourseComplete;
};
