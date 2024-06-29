// Copyright Game Salutes. All Rights Reserved.


#include "Golf/GolfHole.h"

#include "Pawn/PaperGolfPawn.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGGameplayLogging.h"

AGolfHole::AGolfHole()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGolfHole::SetCollider(UPrimitiveComponent* Collider)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: SetCollider - %s"), *GetName(), *LoggingUtils::GetName(Collider));

	if (!ensureMsgf(Collider, TEXT("%s: SetCollider - Collider is null"), *GetName()))
	{
		return;
	}

	Collider->OnComponentBeginOverlap.AddDynamic(this, &AGolfHole::OnComponentBeginOverlap);
	Collider->OnComponentEndOverlap.AddDynamic(this, &AGolfHole::OnComponentEndOverlap);
}

void AGolfHole::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnComponentBeginOverlap - %s"), *GetName(), *LoggingUtils::GetName(OtherActor));

	auto PaperGolfPawn = Cast<APaperGolfPawn>(OtherActor);

	if (!PaperGolfPawn)
	{
		return;
	}

	// Players take turns so there can only be one concurrent overlapping pawn

	OverlappingPaperGolfPawn = PaperGolfPawn;

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	World->GetTimerManager().SetTimer(CheckScoredTimerHandle, this, &AGolfHole::OnCheckScored, 0.1f, true);
}

void AGolfHole::OnComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnComponentEndOverlap - %s"), *GetName(), *LoggingUtils::GetName(OtherActor));

	if (OtherActor == OverlappingPaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnComponentEndOverlap - %s - Player has left hole. Clearing timer."), *GetName(), *LoggingUtils::GetName(OtherActor));
		
		// Be sure to cancel the timer if the overlapping pawn leaves the hole
		ClearTimer();
	}
}

void AGolfHole::OnCheckScored()
{
	if (CheckedScored())
	{
		UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnCheckScored - Player %s has scored. Clearing timer."), *GetName(), *LoggingUtils::GetName(OverlappingPaperGolfPawn.Get()));

		ClearTimer();
	}
}

bool AGolfHole::CheckedScored() const
{
	auto PaperGolfPawn = OverlappingPaperGolfPawn.Get();
	if (!PaperGolfPawn)
	{
		return false;
	}

	// If we come to rest inside the hole, then we have scored
	if (!PaperGolfPawn->IsAtRest())
	{
		return false;
	}

	// Fire scored event

	if(auto GolfEventsSubsystem = GetWorld()->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventsSubsystem))
	{
		GolfEventsSubsystem->OnPaperGolfPawnScored.Broadcast(PaperGolfPawn);
	}

	return true;
}

void AGolfHole::ClearTimer()
{
	OverlappingPaperGolfPawn.Reset();

	if (auto World = GetWorld(); World)
	{
		World->GetTimerManager().ClearTimer(CheckScoredTimerHandle);
	}
}
