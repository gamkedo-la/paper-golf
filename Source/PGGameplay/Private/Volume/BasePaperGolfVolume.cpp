// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Volume/BasePaperGolfVolume.h"

#include "PGGameplayLogging.h"
#include "Pawn/PaperGolfPawn.h"

#include "Subsystems/GolfEventsSubsystem.h"
#include "Components/BrushComponent.h"

#include "Utils/CollisionUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasePaperGolfVolume)


ABasePaperGolfVolume::ABasePaperGolfVolume()
{
	GetBrushComponent()->SetCollisionProfileName(PG::CollisionProfile::OverlapOnlyPawn);
}

void ABasePaperGolfVolume::NotifyActorBeginOverlap(AActor* OtherActor)
{
	UE_LOG(LogPGGameplay, Log, TEXT("%s: NotifyActorBeginOverlap: %s"), *GetName(), OtherActor ? *OtherActor->GetName() : TEXT("NULL"));

	Super::NotifyActorBeginOverlap(OtherActor);

	auto PaperGolfPawn = Cast<APaperGolfPawn>(OtherActor);
	if (!PaperGolfPawn)
	{
		return;
	}

	auto World = GetWorld();
	if (!World)
	{
		UE_LOG(LogPGGameplay, Error, TEXT("%s: World is NULL"), *GetName());
		return;
	}

	auto GolfEvents = World->GetSubsystem<UGolfEventsSubsystem>();
	check(GolfEvents);

	OnPaperGolfPawnOverlap(*PaperGolfPawn, *GolfEvents);
	ReceiveOnPaperGolfPawnOverlap(PaperGolfPawn, GolfEvents);
}
