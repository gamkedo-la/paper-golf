// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GolfEventsSubsystem.generated.h"


class APaperGolfPawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaperGolfPawnOutOfBounds, APaperGolfPawn*, PaperGolfPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaperGolfPawnClippedThroughWorld, APaperGolfPawn*, PaperGolfPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaperGolfPawnScored, APaperGolfPawn*, PaperGolfPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaperGolfShotFinished, APaperGolfPawn*, PaperGolfPawn);

/**
 * 
 */
UCLASS()
class PGPAWN_API UGolfEventsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfPawnOutOfBounds OnPaperGolfPawnOutBounds;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfPawnOutOfBounds OnPaperGolfPawnClippedThroughWorld;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfPawnOutOfBounds OnPaperGolfPawnScored;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfShotFinished OnPaperGolfShotFinished;
};
