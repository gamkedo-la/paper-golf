// Copyright Game Salutes. All Rights Reserved.


#include "BasePaperGolfVolume.h"

#include "PaperGolfLogging.h"
#include "PaperGolfPawn.h"

#include "GolfEventsSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasePaperGolfVolume)


void ABasePaperGolfVolume::NotifyActorBeginOverlap(AActor* OtherActor)
{
	UE_LOG(LogPaperGolfGame, Log, TEXT("%s: NotifyActorBeginOverlap: %s"), *GetName(), OtherActor ? *OtherActor->GetName() : TEXT("NULL"));

	Super::NotifyActorBeginOverlap(OtherActor);

	auto PaperGolfPawn = Cast<APaperGolfPawn>(OtherActor);
	if (!PaperGolfPawn)
	{
		return;
	}

	auto World = GetWorld();
	if (!World)
	{
		UE_LOG(LogPaperGolfGame, Error, TEXT("%s: World is NULL"), *GetName());
		return;
	}

	auto GolfEvents = World->GetSubsystem<UGolfEventsSubsystem>();
	check(GolfEvents);

	OnPaperGolfPawnOverlap(*PaperGolfPawn, *GolfEvents);
	ReceiveOnPaperGolfPawnOverlap(PaperGolfPawn, GolfEvents);
}
