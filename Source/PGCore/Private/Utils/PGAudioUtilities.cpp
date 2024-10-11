// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Utils/PGAudioUtilities.h"

#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

#include "Logging/LoggingUtils.h"
#include "PGCoreLogging.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PGAudioUtilities)


#pragma region General Audio
UAudioComponent* UPGAudioUtilities::PlaySfxAtActorLocation(const AActor* Actor, USoundBase* Sound)
{
	if (!ensure(Sound))
	{
		return nullptr;
	}

	if (!ensure(Actor))
	{
		return nullptr;
	}

	if (Actor->GetNetMode() == NM_DedicatedServer)
	{
		UE_VLOG_UELOG(Actor, LogPGCore, Log,
			TEXT("%s-PGAudioUtilities: PlaySfxAttached - Not playing sfx=%s on dedicated server"),
			*Actor->GetName(), *Sound->GetName());
		return nullptr;
	}

	// The owner of the audio component is derived from the world context object and this will control the sound concurrency
	auto SpawnedAudioComponent = UGameplayStatics::SpawnSoundAtLocation(
		Actor,
		Sound,
		Actor->GetActorLocation(), Actor->GetActorRotation()
	);

	if (!SpawnedAudioComponent)
	{
		// This is not an error condition as the component may not spawn if the sound is not audible, for example it attenuates below a threshold based on distance
		UE_VLOG_UELOG(Actor, LogPGCore, Log,
			TEXT("%s-PGAudioUtilities: PlaySfxAtActorLocation - Unable to spawn audio component for sfx=%s"),
			*Actor->GetName(), *Sound->GetName());
		return nullptr;
	}

	UE_VLOG_UELOG(Actor, LogPGCore, Log,
		TEXT("%s-PGAudioUtilities: PlaySfxAtActorLocation - Playing sfx=%s"),
		*Actor->GetName(), *Sound->GetName());

	SpawnedAudioComponent->bAutoDestroy = true;

	return SpawnedAudioComponent;
}

UAudioComponent* UPGAudioUtilities::PlaySfxAtLocation(const AActor* ContextObject, const FVector& Location, USoundBase* Sound)
{
	if (!ensure(Sound))
	{
		return nullptr;
	}

	if (!ensure(ContextObject))
	{
		return nullptr;
	}

	if (ContextObject->GetNetMode() == NM_DedicatedServer)
	{
		UE_VLOG_UELOG(ContextObject, LogPGCore, Log,
			TEXT("%s-PGAudioUtilities: PlaySfxAtLocation - Not playing sfx=%s on dedicated server"),
			*ContextObject->GetName(), *Sound->GetName());
		return nullptr;
	}

	// The owner of the audio component is derived from the world context object and this will control the sound concurrency
	auto SpawnedAudioComponent = UGameplayStatics::SpawnSoundAtLocation(
		ContextObject,
		Sound,
		Location
	);

	if (!SpawnedAudioComponent)
	{
		// This is not an error condition as the component may not spawn if the sound is not audible, for example it attenuates below a threshold based on distance
		UE_VLOG_UELOG(ContextObject, LogPGCore, Log,
			TEXT("%s-PGAudioUtilities: PlaySfxAtLocation - Unable to spawn audio component for sfx=%s"),
			*ContextObject->GetName(), *Sound->GetName());
		return nullptr;
	}

	UE_VLOG_UELOG(ContextObject, LogPGCore, Log,
		TEXT("%s-PGAudioUtilities: PlaySfxAtLocation - Playing sfx=%s"),
		*ContextObject->GetName(), *Sound->GetName());

	SpawnedAudioComponent->bAutoDestroy = true;

	return SpawnedAudioComponent;
}

UAudioComponent* UPGAudioUtilities::PlaySfxAttached(const AActor* Actor, USoundBase* Sound)
{
	if (!ensure(Sound))
	{
		return nullptr;
	}

	if (!ensure(Actor))
	{
		return nullptr;
	}

	if (Actor->GetNetMode() == NM_DedicatedServer)
	{
		UE_VLOG_UELOG(Actor, LogPGCore, Log,
			TEXT("%s-PGAudioUtilities: PlaySfxAttached - Not playing sfx=%s on dedicated server"),
			*Actor->GetName(), *Sound->GetName());
		return nullptr;
	}

	// The owner of the audio component is derived from the world context object and this will control the sound concurrency
	auto SpawnedAudioComponent = UGameplayStatics::SpawnSoundAttached(
		Sound,
		Actor->GetRootComponent(),
		NAME_None,
		FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset,
		true
	);

	if (!SpawnedAudioComponent)
	{
		// This is not an error condition as the component may not spawn if the sound is not audible, for example it attenuates below a threshold based on distance
		UE_VLOG_UELOG(Actor, LogPGCore, Log,
			TEXT("%s-PGAudioUtilities: PlaySfxAttached - Unable to spawn audio component for sfx=%s"),
			*Actor->GetName(), *Sound->GetName());
		return nullptr;
	}

	UE_VLOG_UELOG(Actor, LogPGCore, Log,
		TEXT("%s-PGAudioUtilities: PlaySfxAttached - Playing sfx=%s"),
		*Actor->GetName(), *Sound->GetName());

	SpawnedAudioComponent->bAutoDestroy = true;

	return SpawnedAudioComponent;
}

void UPGAudioUtilities::PlaySfx2D(const AActor* Owner, USoundBase* Sound)
{
	if (!ensure(Owner))
	{
		return;
	}

	if (!ensure(Sound))
	{
		return;
	}

	UE_VLOG_UELOG(Owner, LogPGCore, Log,
		TEXT("%s-PGAudioUtilities: PlaySfx2D - Playing sfx=%s"),
		*Owner->GetName(), *Sound->GetName());

	if (Owner->GetNetMode() != NM_DedicatedServer)
	{
		UGameplayStatics::PlaySound2D(Owner, Sound);
	}
	else
	{
		UE_VLOG_UELOG(Owner, LogPGCore, Log,
			TEXT("%s-PGAudioUtilities: PlaySfx2D - Not playing sfx=%s on dedicated server"),
			*Owner->GetName(), *Sound->GetName());
	}
}

#pragma endregion General Audio