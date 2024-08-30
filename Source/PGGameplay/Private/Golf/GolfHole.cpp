// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Golf/GolfHole.h"

#include "Pawn/PaperGolfPawn.h"

#include "State/PaperGolfGameStateBase.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include "Components/OverlapConditionComponent.h"

#include "Kismet/GameplayStatics.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "Utils/ArrayUtils.h"
#include "PGGameplayLogging.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfHole)


AGolfHole::AGolfHole()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	// Needs to be marked as always relevant; 
	// otherwise, even a ForceNetUpdate won't replicate the property if it is irrelevant which causes the wrong color to show when it comes into view
	// We make the actor dormant and only trigger the explicit changes
	bAlwaysRelevant = true;
	NetDormancy = ENetDormancy::DORM_DormantAll;

	OverlapConditionComponent = CreateDefaultSubobject<UOverlapConditionComponent>(TEXT("Overlap Condition"));
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

	OverlapConditionComponent->Reset();

	UpdateColliderRegistration();
}

void AGolfHole::PostInitializeComponents()
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: PostInitializeComponents"), *GetName());

	Super::PostInitializeComponents();

	OverlapConditionComponent->Initialize(
		UOverlapConditionComponent::FOverlapConditionDelegate::CreateUObject(this, &AGolfHole::CheckedScored),
		UOverlapConditionComponent::FOverlapTriggerDelegate::CreateUObject(this, &AGolfHole::OnScored));
}

void AGolfHole::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGolfHole, GolfHoleState);
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
}

void AGolfHole::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnComponentBeginOverlap - %s"), *GetName(), *LoggingUtils::GetName(OtherActor));

	auto PaperGolfPawn = Cast<APaperGolfPawn>(OtherActor);

	if (!PaperGolfPawn)
	{
		return;
	}

	OverlapConditionComponent->BeginOverlap(*PaperGolfPawn);
}

void AGolfHole::OnComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnComponentEndOverlap - %s"), *GetName(), *LoggingUtils::GetName(OtherActor));

	auto PaperGolfPawn = Cast<APaperGolfPawn>(OtherActor);

	if (!PaperGolfPawn)
	{
		return;
	}

	OverlapConditionComponent->EndOverlap(*PaperGolfPawn);
}

void AGolfHole::OnScored(APaperGolfPawn& PaperGolfPawn)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnScored - Player %s has scored"), 
		*GetName(), *PaperGolfPawn.GetName());

	// Fire scored event
	if (auto GolfEventsSubsystem = GetWorld()->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventsSubsystem))
	{
		GolfEventsSubsystem->OnPaperGolfPawnScored.Broadcast(&PaperGolfPawn);
	}
}

bool AGolfHole::CheckedScored(const APaperGolfPawn& Pawn) const
{
	return Pawn.IsAtRest();
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
 
	const EGolfHoleState NewState = GameState->GetCurrentHoleNumber() == HoleNumber ? EGolfHoleState::Active : EGolfHoleState::Inactive;

	// If we haven't been initialized yet then we always need to call this
	if(GolfHoleState == NewState)
	{
		UE_VLOG_UELOG(this, LogPGGameplay, Log, 
			TEXT("%s: UpdateColliderRegistration: No change in active hole status: GolfHoleState=%s - skipping collider registration change"),
			*GetName(), *LoggingUtils::GetName(GolfHoleState));
		return;
	}

	if (NewState == EGolfHoleState::Active)
	{
		RegisterCollider();
	}
	else
	{
		UnregisterCollider();
	}

	GolfHoleState = NewState;
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

	OverlapConditionComponent->Reset();

	if (!IsValid(Collider))
	{
		return;
	}

	Collider->OnComponentBeginOverlap.RemoveDynamic(this, &AGolfHole::OnComponentBeginOverlap);
	Collider->OnComponentEndOverlap.RemoveDynamic(this, &AGolfHole::OnComponentEndOverlap);
}

void AGolfHole::OnRep_GolfHoleState()
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnRep_GolfHoleState - HoleNumber=%d; GolfHoleState=%s"), *GetName(), HoleNumber, *LoggingUtils::GetName(GolfHoleState));

	OnActiveHoleChanged();
}

void AGolfHole::OnActiveHoleChanged()
{
	if (GolfHoleState == EGolfHoleState::None)
	{
		UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnActiveHoleChanged - HoleNumber=%d - Skipping GolfHoleState=None"), *GetName(), HoleNumber);
		return;
	}

	BlueprintOnActiveHoleChanged(GolfHoleState == EGolfHoleState::Active);
}

#pragma region Visual Logger
#if ENABLE_VISUAL_LOG

void AGolfHole::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = FString::Printf(TEXT("Hole %d"), HoleNumber);

	Category.Add(TEXT("Hole Number"), FString::Printf(TEXT("%d"), HoleNumber));
	Category.Add(TEXT("State"), LoggingUtils::GetName(GolfHoleState));

	Snapshot->Status.Add(Category);

	if (HasAuthority())
	{
		OverlapConditionComponent->DescribeSelfToVisLog(Snapshot);
	}
}

#endif
#pragma endregion Visual Logger
