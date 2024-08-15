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

#pragma endregion General Audio