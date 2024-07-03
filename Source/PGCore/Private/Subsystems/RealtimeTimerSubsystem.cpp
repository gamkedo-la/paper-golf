// Copyright Game Salutes. All Rights Reserved.


#include "Subsystems/RealtimeTimerSubsystem.h"

#include "Logging/LoggingUtils.h"
#include "PGCoreLogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RealtimeTimerSubsystem)

void URealtimeTimerSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (RealTimeTimerDelegate.IsBound())
	{
		TickDelegates(DeltaTime);
	}
}

TStatId URealtimeTimerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(RealtimeTimerSubsystem, STATGROUP_Tickables);
}

void URealtimeTimerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
}

void URealtimeTimerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URealtimeTimerSubsystem::TickDelegates(float DeltaTime)
{
	UE_LOG(LogPGCore, VeryVerbose, TEXT("%s: TickDelegates=%fs"), *GetName(), DeltaTime);

	RealTimeTimerDelegate.Broadcast(DeltaTime);
}

