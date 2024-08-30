// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/OverlapConditionComponent.h"

#include "PGGameplayLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Pawn/PaperGolfPawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OverlapConditionComponent)


UOverlapConditionComponent::UOverlapConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UOverlapConditionComponent::Initialize(const FOverlapConditionDelegate& InOverlapConditionDelegate,
	const FOverlapTriggerDelegate& InOverlapTriggerDelegate)
{
	UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Log, TEXT("%s-%s: Initialize - InOverlapConditionDelegateValid=%s; InOverlapTriggerDelegateValid=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		LoggingUtils::GetBoolString(InOverlapConditionDelegate.IsBound()),
		LoggingUtils::GetBoolString(InOverlapTriggerDelegate.IsBound()));

	OverlapConditionDelegate = InOverlapConditionDelegate;
	OverlapTriggerDelegate = InOverlapTriggerDelegate;
}

void UOverlapConditionComponent::BeginOverlap(APaperGolfPawn& Pawn)
{
	UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Log, TEXT("%s-%s: BeginOverlap: %s replacing existing overlap=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *Pawn.GetName(), *LoggingUtils::GetName(OverlappingPaperGolfPawn));

	OverlappingPaperGolfPawn = &Pawn;

	StartTimer();
}

void UOverlapConditionComponent::EndOverlap(APaperGolfPawn& Pawn)
{
	UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Log, TEXT("%s-%s: EndOverlap: %s"), *LoggingUtils::GetName(GetOwner()), *GetName(), *Pawn.GetName());

	const auto OverlappingPaperGolfPawnPtr = OverlappingPaperGolfPawn.Get();

	if (!OverlappingPaperGolfPawnPtr)
	{
		return;
	}

	if (&Pawn == OverlappingPaperGolfPawnPtr)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Log, TEXT("%s-%s: EndOverlap - matched begin condition pawn - resetting state"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *Pawn.GetName());

		ClearTimer();
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Warning, TEXT("%s-%s: EndOverlap - Ignoring - InPawn=%s does not match existing overlap=%s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *Pawn.GetName(), *LoggingUtils::GetName(OverlappingPaperGolfPawn));
	}
}

void UOverlapConditionComponent::Reset()
{
	UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Log, TEXT("%s-%s: Reset"), *LoggingUtils::GetName(GetOwner()), *GetName());

	ClearTimer();
}

void UOverlapConditionComponent::BeginPlay()
{
	UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Log, TEXT("%s-%s: BeginPlay"), *LoggingUtils::GetName(GetOwner()), *GetName());

	Super::BeginPlay();
}

void UOverlapConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Log, TEXT("%s-%s: EndPlay"), *LoggingUtils::GetName(GetOwner()), *GetName());

	Super::EndPlay(EndPlayReason);

	ClearTimer();
}

void UOverlapConditionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CheckOverlapCondition();
}

void UOverlapConditionComponent::CheckOverlapCondition()
{
	UE_VLOG_UELOG(GetOwner(), LogPGGameplay, VeryVerbose, TEXT("%s-%s: CheckOverlapCondition - OverlappingPaperGolfPawn=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		*LoggingUtils::GetName(OverlappingPaperGolfPawn));

	const auto PaperGolfPawn = OverlappingPaperGolfPawn.Get();
	if (!PaperGolfPawn)
	{
		// Ensure timer is turned off if the overlapping pawn has become invalid
		ClearTimer();

		return;
	}

	if (OverlapConditionDelegate.IsBound() && OverlapConditionDelegate.Execute(*PaperGolfPawn))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Log, TEXT("%s-%s: CheckOverlapCondition - Overlap condition met - triggering delegate"),
			*LoggingUtils::GetName(GetOwner()), *GetName());

		ClearTimer();
		OverlapTriggerDelegate.ExecuteIfBound(*PaperGolfPawn);
	}
}

void UOverlapConditionComponent::ClearTimer()
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s-%s: ClearTimer: TimerActive=%s; OverlappingPaperGolfPawn=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		*GetName(), LoggingUtils::GetBoolString(IsComponentTickEnabled()), *LoggingUtils::GetName(OverlappingPaperGolfPawn));

	OverlappingPaperGolfPawn.Reset();
	SetComponentTickEnabled(false);
}

void UOverlapConditionComponent::StartTimer()
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s-%s: StartTimer: TimerActive=%s; OverlappingPaperGolfPawn=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		*GetName(), LoggingUtils::GetBoolString(IsComponentTickEnabled()), *LoggingUtils::GetName(OverlappingPaperGolfPawn));

	// start timer to listen for end condition
	SetComponentTickEnabled(true);
}

#pragma region Visual Logger
#if ENABLE_VISUAL_LOG

void UOverlapConditionComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Overlap Condition Component");

	Category.Add(TEXT("OverlappingPaperGolfPawn"), LoggingUtils::GetName(OverlappingPaperGolfPawn));
	Category.Add(TEXT("Timer Active"), LoggingUtils::GetBoolString(IsComponentTickEnabled()));

	Snapshot->Status.Add(Category);
}

#endif
#pragma endregion Visual Logger