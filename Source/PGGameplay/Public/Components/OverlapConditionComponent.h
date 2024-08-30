// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "OverlapConditionComponent.generated.h"

class APaperGolfPawn;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PGGAMEPLAY_API UOverlapConditionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UOverlapConditionComponent();

	// Returns whether the overlap condition is met
	DECLARE_DELEGATE_RetVal_OneParam(bool, FOverlapConditionDelegate, const APaperGolfPawn& /*Overlapping Pawn */);
	DECLARE_DELEGATE_OneParam(FOverlapTriggerDelegate, APaperGolfPawn& /*Overlapping Pawn */);

	void Initialize(const FOverlapConditionDelegate& InOverlapConditionDelegate,
					const FOverlapTriggerDelegate& InOverlapTriggerDelegate);

	void BeginOverlap(APaperGolfPawn& Pawn);
	void EndOverlap(APaperGolfPawn& Pawn);

	void Reset();

#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void CheckOverlapCondition();

	void ClearTimer();
	void StartTimer();

private:
	FOverlapConditionDelegate OverlapConditionDelegate{};
	FOverlapTriggerDelegate OverlapTriggerDelegate{};
	TWeakObjectPtr<APaperGolfPawn> OverlappingPaperGolfPawn{};
	FTimerHandle OverlapTimerHandle{};

	UPROPERTY(EditAnywhere, Category = "Timer")
	float TimerInterval{ 0.1f };
};
