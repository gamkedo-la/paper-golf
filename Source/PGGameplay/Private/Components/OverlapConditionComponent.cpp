// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/OverlapConditionComponent.h"

#include "PGGameplayLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Pawn/PaperGolfPawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OverlapConditionComponent)


UOverlapConditionComponent::UOverlapConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
		UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Log, TEXT("%s-%s: EndOverlap: No active overlapping pawn"), *LoggingUtils::GetName(GetOwner()), *GetName());
		return;
	}

	// Check overlap end condition one more time as resetting the physics state will cause this event to trigger
	if (CheckOverlapCondition(true))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Log, TEXT("%s-%s: EndOverlap - overlap condition met"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		return;
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Log, TEXT("%s-%s: EndOverlap - overlap condition not met"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
	}

	if (&Pawn == OverlappingPaperGolfPawnPtr)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Log, TEXT("%s-%s: EndOverlap - matched begin condition pawn - resetting state"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *Pawn.GetName());

		// Do not clear the overlapping pawn as the condition may trigger again later
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

	ClearAll();
}

void UOverlapConditionComponent::ClearAll()
{
	UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Log, TEXT("%s-%s: ClearAll"), *LoggingUtils::GetName(GetOwner()), *GetName());
	ClearTimer();

	OverlappingPaperGolfPawn.Reset();
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

	ClearAll();
}

bool UOverlapConditionComponent::CheckOverlapCondition(bool bDeferTrigger)
{
	UE_VLOG_UELOG(GetOwner(), LogPGGameplay, VeryVerbose, TEXT("%s-%s: CheckOverlapCondition - OverlappingPaperGolfPawn=%s; bDeferTrigger=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		*LoggingUtils::GetName(OverlappingPaperGolfPawn),
		LoggingUtils::GetBoolString(bDeferTrigger)
	);

	const auto PaperGolfPawn = OverlappingPaperGolfPawn.Get();
	if (!PaperGolfPawn)
	{
		// Ensure timer is turned off if the overlapping pawn has become invalid
		ClearAll();

		return false;
	}

	if (OverlapConditionDelegate.IsBound() && OverlapConditionDelegate.Execute(*PaperGolfPawn))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGGameplay, Log, TEXT("%s-%s: CheckOverlapCondition - Overlap condition met - triggering delegate - bDeferTrigger=%s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), LoggingUtils::GetBoolString(bDeferTrigger));

		ClearAll();

		if (bDeferTrigger)
		{
			if (auto World = GetWorld(); ensure(World))
			{
				World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, TriggerPawnWeak = MakeWeakObjectPtr(PaperGolfPawn)]
				{
					if (auto TriggerPawn = TriggerPawnWeak.Get(); TriggerPawn)
					{
						OverlapTriggerDelegate.ExecuteIfBound(*TriggerPawn);
					}
				}));
			}
		}
		else
		{
			OverlapTriggerDelegate.ExecuteIfBound(*PaperGolfPawn);
		}

		return true;
	}

	return false;
}

void UOverlapConditionComponent::ClearTimer()
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s-%s: ClearTimer: TimerActive=%s; OverlappingPaperGolfPawn=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		*GetName(), LoggingUtils::GetBoolString(IsComponentTickEnabled()), *LoggingUtils::GetName(OverlappingPaperGolfPawn));

	// Do not reset the overlapping pawn as sometimes the end overlap triggers prematurely and then it will trigger again and we want to be able to test the overlap condition
	// Only reset the overlapping pawn if the condition succeeds

	if (auto World = GetWorld(); OverlapTimerHandle.IsValid() && World)
	{
		World->GetTimerManager().ClearTimer(OverlapTimerHandle);
	}
}

void UOverlapConditionComponent::StartTimer()
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s-%s: StartTimer: TimerActive=%s; OverlappingPaperGolfPawn=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		*GetName(), LoggingUtils::GetBoolString(IsComponentTickEnabled()), *LoggingUtils::GetName(OverlappingPaperGolfPawn));

	// start timer to listen for end condition
	if (auto World = GetWorld(); !OverlapTimerHandle.IsValid() && ensure(World))
	{
		auto TimerDelegate = FTimerDelegate::CreateWeakLambda(this, [this]() { CheckOverlapCondition(); });
		World->GetTimerManager().SetTimer(OverlapTimerHandle, TimerDelegate, TimerInterval, true);
	}
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