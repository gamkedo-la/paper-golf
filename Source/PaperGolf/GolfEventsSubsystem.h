// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GolfEventsSubsystem.generated.h"


class APaperGolfPawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaperGolfPawnOutOfBounds, APaperGolfPawn*, PaperGolfPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPaperGolfPawnClippedThroughWorld, APaperGolfPawn*, PaperGolfPawn);

/**
 * 
 */
UCLASS()
class PAPERGOLF_API UGolfEventsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfPawnOutOfBounds OnPaperGolfPawnOutBounds;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPaperGolfPawnOutOfBounds OnPaperGolfPawnClippedThroughWorld;
};
