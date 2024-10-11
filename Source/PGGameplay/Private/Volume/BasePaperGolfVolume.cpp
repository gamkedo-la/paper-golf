// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Volume/BasePaperGolfVolume.h"

#include "Components/OverlapConditionComponent.h"
#include "PGGameplayLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "Pawn/PaperGolfPawn.h"

#include "Subsystems/GolfEventsSubsystem.h"
#include "Components/BrushComponent.h"

#include "Utils/CollisionUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasePaperGolfVolume)


ABasePaperGolfVolume::ABasePaperGolfVolume()
{
	auto OverlapComponent = GetBrushComponent();
	OverlapComponent->SetCollisionProfileName(PG::CollisionProfile::OverlapOnlyPawn);
	OverlapComponent->SetGenerateOverlapEvents(true);

	OverlapConditionComponent = CreateDefaultSubobject<UOverlapConditionComponent>(TEXT("Overlap Condition"));
}

void ABasePaperGolfVolume::PostInitializeComponents()
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: PostInitializeComponents"), *GetName());

	Super::PostInitializeComponents();

	// Events only fire on server and the overlaps are server authoritative
	if (!HasAuthority())
	{
		UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: PostInitializeComponents - Disable collision and overlaps on client"), *GetName());

		if (auto OverlapComponent = GetBrushComponent(); ensure(OverlapComponent))
		{
			OverlapComponent->SetCollisionProfileName(PG::CollisionProfile::NoCollision);
			OverlapComponent->SetGenerateOverlapEvents(false);
		}

		return;
	}

	if (Type == EPaperGolfVolumeOverlapType::End)
	{
		UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: PostInitializeComponents - Initializing OverlapConditionComponent"), *GetName());

		OverlapConditionComponent->Initialize(
			UOverlapConditionComponent::FOverlapConditionDelegate::CreateUObject(this, &ABasePaperGolfVolume::IsConditionTriggered),
			UOverlapConditionComponent::FOverlapTriggerDelegate::CreateUObject(this, &ABasePaperGolfVolume::NotifyConditionTriggered)
		);
	}
}

void ABasePaperGolfVolume::NotifyActorBeginOverlap(AActor* OtherActor)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Verbose, TEXT("%s: NotifyActorBeginOverlap: %s"), *GetName(), *LoggingUtils::GetName(OtherActor));

	Super::NotifyActorBeginOverlap(OtherActor);

	auto PaperGolfPawn = Cast<APaperGolfPawn>(OtherActor);
	if (!PaperGolfPawn)
	{
		return;
	}

	if (Type == EPaperGolfVolumeOverlapType::Any)
	{
		NotifyConditionTriggered(*PaperGolfPawn);
	}
	else
	{
		OverlapConditionComponent->BeginOverlap(*PaperGolfPawn);
	}
}

void ABasePaperGolfVolume::NotifyActorEndOverlap(AActor* OtherActor)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Verbose, TEXT("%s: NotifyActorEndOverlap: %s"), *GetName(), *LoggingUtils::GetName(OtherActor));

	Super::NotifyActorEndOverlap(OtherActor);

	if (Type == EPaperGolfVolumeOverlapType::Any)
	{
		UE_VLOG_UELOG(this, LogPGGameplay, Verbose, TEXT("%s: NotifyActorEndOverlap: %s - Skipping for Type=Any"),
			*GetName(), *LoggingUtils::GetName(OtherActor));

		return;
	}

	auto PaperGolfPawn = Cast<APaperGolfPawn>(OtherActor);
	if (!PaperGolfPawn)
	{
		return;
	}

	OverlapConditionComponent->EndOverlap(*PaperGolfPawn);
}

void ABasePaperGolfVolume::NotifyConditionTriggered(APaperGolfPawn& PaperGolfPawn)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: NotifyConditionTriggered: %s"), *GetName(), *PaperGolfPawn.GetName());

	auto World = GetWorld();
	if (!World)
	{
		UE_LOG(LogPGGameplay, Error, TEXT("%s: World is NULL"), *GetName());
		return;
	}

	auto GolfEvents = World->GetSubsystem<UGolfEventsSubsystem>();
	check(GolfEvents);

	OnConditionTriggered(PaperGolfPawn, *GolfEvents);
	ReceiveConditionTriggered(&PaperGolfPawn, GolfEvents);
}
