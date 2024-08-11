// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Golf/GolfHole.h"

#include "Pawn/PaperGolfPawn.h"

#include "State/PaperGolfGameStateBase.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include "Kismet/GameplayStatics.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "Utils/ArrayUtils.h"
#include "PGGameplayLogging.h"

#include "Net/UnrealNetwork.h"


AGolfHole::AGolfHole()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	// Needs to be marked as always relevant; 
	// otherwise, even a ForceNetUpdate won't replicate the property if it is irrelevant which causes the wrong color to show when it comes into view
	// We make the actor dormant and only trigger the explicit changes
	bAlwaysRelevant = true;
	NetDormancy = ENetDormancy::DORM_DormantAll;
}

AGolfHole* AGolfHole::GetCurrentHole(const UObject* WorldContextObject)
{
	if(!ensureAlwaysMsgf(WorldContextObject, TEXT("GetCurrentHole: WorldContextObject is null")))
	{
		return nullptr;
	}

	auto World = WorldContextObject->GetWorld();
	if (!ensureAlways(World))
	{
		return nullptr;
	}

	auto GameState = World->GetGameState<APaperGolfGameStateBase>();
	if(!ensureAlwaysMsgf(GameState, TEXT("GetCurrentHole: WorldContextObject=%s; GameState=%s is not APaperGolfGameStateBase"),
		*LoggingUtils::GetName(WorldContextObject), *LoggingUtils::GetName(World->GetGameState())))
	{
		UE_VLOG_UELOG(WorldContextObject, LogPGGameplay, Error, TEXT("%s: GetCurrentHole: GameState=%s is not APaperGolfGameStateBase"),
			*LoggingUtils::GetName(WorldContextObject), *LoggingUtils::GetName(World->GetGameState()));
		return nullptr;
	}

	const auto GolfHoles = GetAllWorldHoles(WorldContextObject, false);

	check(GameState);

	const auto CurrentHoleNumber = GameState->GetCurrentHoleNumber();
	AGolfHole* MatchedGolfHole{};

	for (auto GolfHole : GolfHoles)
	{
		if(AGolfHole::Execute_GetHoleNumber(GolfHole) == CurrentHoleNumber)
		{
			if (!MatchedGolfHole)
			{
				MatchedGolfHole = GolfHole;
			}
			else
			{
				UE_VLOG_UELOG(WorldContextObject, LogPGGameplay, Error, TEXT("%s: GetCurrentHole: Found multiple golf holes for hole number %d: %s and %s"),
					*LoggingUtils::GetName(WorldContextObject), CurrentHoleNumber, *MatchedGolfHole->GetName(), *GolfHole->GetName());
			}
		}
	}
	
	if(!ensureAlwaysMsgf(MatchedGolfHole, TEXT("GetCurrentHole: No golf hole found for hole number %d"), CurrentHoleNumber))
	{
		UE_VLOG_UELOG(WorldContextObject, LogPGGameplay, Error, TEXT("%s: GetCurrentHole: No golf hole found for hole number %d"),
			*LoggingUtils::GetName(WorldContextObject), CurrentHoleNumber);
	}

	return MatchedGolfHole;
}

TArray<AGolfHole*> AGolfHole::GetAllWorldHoles(const UObject* WorldContextObject, bool bSort)
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(WorldContextObject, AGolfHole::StaticClass(), Actors);

	TArray<AGolfHole*> GolfHoles;
	GolfHoles.Reserve(Actors.Num());

	for (auto Actor : Actors)
	{
		auto GolfHole = Cast<AGolfHole>(Actor);
		if (GolfHole)
		{
			GolfHoles.Add(GolfHole);
		}
	}

	UE_VLOG_UELOG(WorldContextObject, LogPGGameplay, Log, TEXT("%s: GetAllWorldHoles: Found %d golf hole%s: %s"),
		*LoggingUtils::GetName(WorldContextObject), GolfHoles.Num(), LoggingUtils::Pluralize(GolfHoles.Num()), *PG::ToStringObjectElements(GolfHoles));

	if (!bSort)
	{
		return GolfHoles;
	}

	// Sort by hole number
	GolfHoles.Sort([](const AGolfHole& First, const AGolfHole& Second)
	{
		return AGolfHole::Execute_GetHoleNumber(&First) < AGolfHole::Execute_GetHoleNumber(&Second);
	});

	UE_VLOG_UELOG(WorldContextObject, LogPGGameplay, Log, TEXT("%s: GetAllWorldHoles: Sorted Golf holes=%s"),
		*LoggingUtils::GetName(WorldContextObject), *PG::ToStringObjectElements(GolfHoles));

	return GolfHoles;
}

void AGolfHole::Reset()
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: Reset"), *GetName());

	Super::Reset();

	UpdateColliderRegistration();
}

void AGolfHole::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGolfHole, bActiveHole);
}

void AGolfHole::SetCollider(UPrimitiveComponent* InCollider)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: SetCollider - %s"), *GetName(), *LoggingUtils::GetName(InCollider));

	if (!ensureMsgf(InCollider, TEXT("%s: SetCollider - Collider is null"), *GetName()))
	{
		return;
	}

	Collider = InCollider;

	UpdateColliderRegistration();

	bInitialized = true;
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
		UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnComponentEndOverlap - %s - Player has left hole."), *GetName(), *LoggingUtils::GetName(OtherActor));
		
		// Be sure to cancel the timer if the overlapping pawn leaves the hole
		ClearTimer();
	}
}

void AGolfHole::OnCheckScored()
{
	if (CheckedScored())
	{
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
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: CheckedScored - Player %s has scored"), *GetName(), *PaperGolfPawn->GetName());

	if(auto GolfEventsSubsystem = GetWorld()->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventsSubsystem))
	{
		GolfEventsSubsystem->OnPaperGolfPawnScored.Broadcast(PaperGolfPawn);
	}

	return true;
}

void AGolfHole::ClearTimer()
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: ClearTimer: CheckScoredTimerHandle=%s; OverlappingPaperGolfPawn=%s"),
		*GetName(), LoggingUtils::GetBoolString(CheckScoredTimerHandle.IsValid()), *LoggingUtils::GetName(OverlappingPaperGolfPawn));

	OverlappingPaperGolfPawn.Reset();

	if (auto World = GetWorld(); World)
	{
		World->GetTimerManager().ClearTimer(CheckScoredTimerHandle);
	}
}

void AGolfHole::UpdateColliderRegistration()
{
	// Only register ovents on server
	if (!HasAuthority())
	{
		return;
	}

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	auto GameState = World->GetGameState<APaperGolfGameStateBase>();

	if (!ensureAlwaysMsgf(GameState, TEXT("%s: UpdateColliderRegistration: GameState=%s is not APaperGolfGameStateBase"),
	   *GetName(), *LoggingUtils::GetName(World->GetGameState())))
	{
		UE_VLOG_UELOG(this, LogPGGameplay, Error, TEXT("%s: UpdateColliderRegistration: GameState=%s is not APaperGolfGameStateBase"),
			*GetName(), *LoggingUtils::GetName(World->GetGameState()));
		return;
	}

	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: UpdateColliderRegistration: CurrentHole=%d; MyHoleNumber=%d"), *GetName(), GameState->GetCurrentHoleNumber(), HoleNumber);
 
	const bool bNewActiveHoleStatus = GameState->GetCurrentHoleNumber() == HoleNumber;

	// If we haven't been initialized yet then we always need to call this
	if(bInitialized && bNewActiveHoleStatus == bActiveHole)
	{
		UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: UpdateColliderRegistration: No change in active hole status: bActiveHole=%s - skipping collider registration change"), *GetName(), LoggingUtils::GetBoolString(bActiveHole));
		return;
	}

	if (bNewActiveHoleStatus)
	{
		RegisterCollider();
	}
	else
	{
		UnregisterCollider();
	}

	bActiveHole = bNewActiveHoleStatus;
	ForceNetUpdate();

	// Call directly on server
	OnActiveHoleChanged();
}

void AGolfHole::RegisterCollider()
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: RegisterCollider: %s"), *GetName(), *LoggingUtils::GetName(Collider));

	if (!ensure(IsValid(Collider)))
	{
		return;
	}

	Collider->OnComponentBeginOverlap.AddUniqueDynamic(this, &AGolfHole::OnComponentBeginOverlap);
	Collider->OnComponentEndOverlap.AddUniqueDynamic(this, &AGolfHole::OnComponentEndOverlap);
}

void AGolfHole::UnregisterCollider()
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: UnregisterCollider: %s"), *GetName(), *LoggingUtils::GetName(Collider));

	ClearTimer();

	if (!IsValid(Collider))
	{
		return;
	}

	Collider->OnComponentBeginOverlap.RemoveDynamic(this, &AGolfHole::OnComponentBeginOverlap);
	Collider->OnComponentEndOverlap.RemoveDynamic(this, &AGolfHole::OnComponentEndOverlap);
}

void AGolfHole::OnRep_ActiveHole()
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnRep_ActiveHole - bActiveHole=%s"), *GetName(), LoggingUtils::GetBoolString(bActiveHole));

	OnActiveHoleChanged();
}

void AGolfHole::OnActiveHoleChanged()
{
	BlueprintOnActiveHoleChanged(bActiveHole);
}
