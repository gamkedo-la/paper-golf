// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/PaperGolfPawnAudioComponent.h"

#include "Audio/PGPawnAudioConfigAsset.h"

#include "Logging/LoggingUtils.h"
#include "PGPawnLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "Utils/PGAudioUtilities.h"

#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfPawnAudioComponent)

UPaperGolfPawnAudioComponent::UPaperGolfPawnAudioComponent()
{
	bEnableCollisionSounds = true;

	SetIsReplicatedByDefault(true);
}

void UPaperGolfPawnAudioComponent::PlayFlick()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: PlayFlick"), *LoggingUtils::GetName(GetOwner()), *GetName());

	if (!PawnAudioConfig)
	{
		return;
	}

	if (GetNetMode() != NM_DedicatedServer)
	{
		UPGAudioUtilities::PlaySfx2D(GetOwner(), PawnAudioConfig->FlickSfx);
	}

	// Flight sound is handled via replication in order to make sure that it is started and stopped reliably
	// It's a looping sound so better handled with replication than a reliable event for players that might join a game in progress
	if (GetOwnerRole() != ROLE_SimulatedProxy)
	{
		PlayFlight();
	}
}

void UPaperGolfPawnAudioComponent::PlayTurnStart()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: PlayTurnStart"), *LoggingUtils::GetName(GetOwner()), *GetName());

	if (!PawnAudioConfig)
	{
		return;
	}

	StopFlight();

	if (GetNetMode() != NM_DedicatedServer)
	{
		UPGAudioUtilities::PlaySfxAtActorLocation(GetOwner(), PawnAudioConfig->TurnStartSfx);
	}
}

void UPaperGolfPawnAudioComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bPlayFlightRequested);
}

void UPaperGolfPawnAudioComponent::BeginPlay()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: BeginPlay"), *LoggingUtils::GetName(GetOwner()), *GetName());

	Super::BeginPlay();

	PawnAudioConfig = Cast<UPGPawnAudioConfigAsset>(AudioConfigAsset);

	if(!ensureMsgf(PawnAudioConfig, TEXT("%s-%s: Missing AudioConfig=%s is not a UPGPawnAudioConfigAsset"), 
		*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(AudioConfigAsset)))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Error, TEXT("%s-%s: Missing AudioConfig=%s is not a UPGPawnAudioConfigAsset"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(AudioConfigAsset))
	}
}

void UPaperGolfPawnAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: EndPlay"), *LoggingUtils::GetName(GetOwner()), *GetName());

	StopFlight();

	Super::EndPlay(EndPlayReason);
}

void UPaperGolfPawnAudioComponent::RegisterCollisions()
{
	if (!bEnableCollisionSounds)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Display,
			TEXT("%s-%s: RegisterCollisions - bEnableCollisionSounds was false, skipping registration"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}

	if(auto Mesh = GetOwner()->FindComponentByClass<UStaticMeshComponent>(); ensureMsgf(Mesh, TEXT("Owner missing UStaticMeshComponent")))
	{
		RegisterComponent(Mesh);
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Error,
			TEXT("%s-%s: RegisterCollisions - Missing UStaticMeshComponent on Owner"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
	}
}

void UPaperGolfPawnAudioComponent::OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse)
{
	Super::OnNotifyRelevantCollision(HitComponent, Hit, NormalImpulse);

	StopFlight();
}

void UPaperGolfPawnAudioComponent::PlayFlight()
{
	if (IsValid(FlightAudioComponent))
	{
		return;
	}

	const auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: PlayFlight"), *LoggingUtils::GetName(GetOwner()), *GetName());

	if (!PawnAudioConfig)
	{
		return;
	}

	bPlayFlightRequested = true;

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const auto TimerDelegate = FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		// Check that stop hasn't been requested in the time it took for the timer to fire
		if (bPlayFlightRequested)
		{
			FlightAudioComponent = UPGAudioUtilities::PlaySfxAttached(GetOwner(), PawnAudioConfig->FlightSfx);
		}
	});

	// Only delay if we're on server or autonous proxy as simulated proxy only plays from network replication event
	if(GetOwnerRole() != ROLE_SimulatedProxy && PawnAudioConfig->FlightSfxDelayAfterFlick > 0)
	{
		World->GetTimerManager().SetTimer(FlightAudioTimerHandle, TimerDelegate, PawnAudioConfig->FlightSfxDelayAfterFlick, false);
	}
	else
	{
		TimerDelegate.Execute();
	}
}

void UPaperGolfPawnAudioComponent::StopFlight()
{
	bPlayFlightRequested = false;
	CancelFlightAudioTimer();

	if (GetNetMode() == NM_DedicatedServer)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: StopFlight"), *LoggingUtils::GetName(GetOwner()), *GetName());
	}

	if (!IsValid(FlightAudioComponent))
	{
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: StopFlight"), *LoggingUtils::GetName(GetOwner()), *GetName());

	check(PawnAudioConfig);

	const auto FadeOutTime = PawnAudioConfig->FlightSfxFadeOutTime;

	if (FadeOutTime > 0)
	{
		FlightAudioComponent->FadeOut(FadeOutTime, 0.0f);
	}
	else
	{
		FlightAudioComponent->Stop();
	}

	FlightAudioComponent = nullptr;
}

void UPaperGolfPawnAudioComponent::CancelFlightAudioTimer()
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(FlightAudioTimerHandle);
}

void UPaperGolfPawnAudioComponent::OnRep_PlayFlightRequested()
{
	UE_VLOG_UELOG(GetOwner(),
		LogPGPawn, Log, TEXT("%s-%s: OnRep_PlayFlightRequested - bPlayFlightRequested=%d"), *LoggingUtils::GetName(GetOwner()), *GetName(), bPlayFlightRequested);
	
	if (bPlayFlightRequested)
	{
		PlayFlight();
	}
	else
	{
		StopFlight();
	}
}
