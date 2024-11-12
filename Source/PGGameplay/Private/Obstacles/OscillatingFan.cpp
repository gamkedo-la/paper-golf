// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Obstacles/OscillatingFan.h"

#include "VisualLogger/VisualLogger.h"
#include "Utils/VisualLoggerUtils.h"
#include "Logging/LoggingUtils.h"
#include "PGGameplayLogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OscillatingFan)

AOscillatingFan::AOscillatingFan()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
}

void AOscillatingFan::BeginPlay()
{
	Super::BeginPlay();

	// TODO: We should subscribe to game events to disable the force if an end overlap event doesn't happen before the end of the player's turn
	// or maybe even the end of the hole
}

void AOscillatingFan::SetInfluenceCollider(UPrimitiveComponent* InCollider)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: SetInfluenceCollider - %s"),
		*GetName(), *LoggingUtils::GetName(InCollider));

	if (!ensureMsgf(InCollider, TEXT("%s: SetCollider - Collider is null"), *GetName()))
	{
		return;
	}
}

void AOscillatingFan::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnComponentBeginOverlap - %s"), *GetName(), *LoggingUtils::GetName(OtherActor));

	SetForceActive(true);
}

void AOscillatingFan::OnComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnComponentEndOverlap - %s"), *GetName(), *LoggingUtils::GetName(OtherActor));

	SetForceActive(false);
}

void AOscillatingFan::SetForceActive(bool bActive)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: SetForceActive - %s"), *GetName(), LoggingUtils::GetBoolString(bActive));

	bForceIsActive = bActive;
	PrimaryActorTick.SetTickFunctionEnable(bActive);
}

void AOscillatingFan::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AOscillatingFan::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

