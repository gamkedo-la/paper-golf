// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/PGHitSfxComponent.h"

#include "Logging/LoggingUtils.h"
#include "PGCoreLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

#include "Audio/PGAudioConfigAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PGHitSfxComponent)

UPGHitSfxComponent::UPGHitSfxComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPGHitSfxComponent::BeginPlay()
{
	Super::BeginPlay();
}

#pragma region Collisions
bool UPGHitSfxComponent::ShouldPlayHitSfx_Implementation(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse) const
{
	if (!ensure(AudioConfigAsset))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGCore, Error,
			TEXT("%s-%s: ShouldPlayHitSfx_Implementation - AudioConfigAsset was NULL"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		return false;
	}

	if (AudioConfigAsset->MaxPlayCount > 0 && HitPlayCount >= AudioConfigAsset->MaxPlayCount)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGCore, Log,
			TEXT("%s-%s: ShouldPlaySfx - Not playing HitSfx as PlayCount=%d >= MaxPlayCount=%d"),
			*LoggingUtils::GetName(GetOwner()), *GetName(),
			HitPlayCount, AudioConfigAsset->MaxPlayCount);

		return false;
	}

	return true;
}

void UPGHitSfxComponent::RegisterCollisions()
{
	if(!bEnableCollisionSounds)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGCore, Display,
			TEXT("%s-%s: RegisterCollisions - bEnableCollisionSounds was false, skipping registration"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}

	if (BlueprintRegisterCollisions())
	{
		UE_VLOG_UELOG(GetOwner(), LogPGCore, Warning,
			TEXT("%s-%s: RegisterCollisions - BlueprintRegisterCollisions consumed registration, preventing default behavior"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}

	auto MyOwner = GetOwner();
	if (!ensureMsgf(MyOwner, TEXT("%s: Owner was NULL"), *GetName()))
	{
		return;
	}

	int32 RegisteredComponents{};

	MyOwner->ForEachComponent<UPrimitiveComponent>(false, [&](auto Component)
		{
			++RegisteredComponents;
			RegisterComponent(Component);
		});

	if (!RegisteredComponents)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGCore, Warning,
			TEXT("%s-%s: RegisterCollisions - Could not find any PrimitiveComponents to register for hit callbacks!"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
	}
}

void UPGHitSfxComponent::OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse)
{
	if (!ensure(AudioConfigAsset))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGCore, Error,
			TEXT("%s-%s: OnNotifyRelevantCollision - AudioConfigAsset was NULL"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		return;
	}

	if (!ShouldPlayHitSfx(HitComponent, Hit, NormalImpulse))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGCore, Log,
			TEXT("%s-%s: OnNotifyRelevantCollision - Not playing HitSfx as ShouldPlayHitSfx() returned false"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		return;
	}

	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	const auto TimeSeconds = World->GetTimeSeconds();

	if ((AudioConfigAsset->MinPlayInterval > 0 && LastHitPlayTimeSeconds >= 0 && TimeSeconds - LastHitPlayTimeSeconds < AudioConfigAsset->MinPlayInterval) || TimeSeconds < AudioConfigAsset->MinPlayTimeSeconds)
	{
		if (TimeSeconds < AudioConfigAsset->MinPlayTimeSeconds)
		{
			UE_VLOG_UELOG(GetOwner(), LogPGCore, Log,
				TEXT("%s-%s: OnNotifyRelevantCollision - Not playing HitSfx as TimeSeconds=%fs < MinPlayTimeSeconds=%fs"),
				*LoggingUtils::GetName(GetOwner()), *GetName(),
				TimeSeconds, AudioConfigAsset->MinPlayTimeSeconds);
		}
		else
		{
			UE_VLOG_UELOG(GetOwner(), LogPGCore, Log,
				TEXT("%s-%s: OnNotifyRelevantCollision - Not playing HitSfx as DeltaTime=%fs < MinPlayInterval=%fs"),
				*LoggingUtils::GetName(GetOwner()), *GetName(),
				TimeSeconds - LastHitPlayTimeSeconds, AudioConfigAsset->MinPlayInterval);
		}

		return;
	}

	const auto Volume = GetAudioVolume(NormalImpulse);

	UE_VLOG_UELOG(GetOwner(), LogPGCore, Verbose,
		TEXT("%s-%s: OnNotifyRelevantCollision - NormalImpulse=%.3e resulted in Volume=%.2f; Range = [%.1f,%1f]; ImpulseRange=[%.1e,%.1e]"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), NormalImpulse.Size(),
		Volume, AudioConfigAsset->MinAudioVolume, AudioConfigAsset->MaxAudioVolume,
		AudioConfigAsset->MinVolumeImpulse, AudioConfigAsset->MaxVolumeImpulse);

	if (FMath::IsNearlyZero(Volume, 1e-3))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGCore, Log,
			TEXT("%s-%s: OnNotifyRelevantCollision - Not playing HitSfx as NormalImpulse=%.3e resulted in 0 Volume"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), NormalImpulse.Size());

		return;
	}

	const auto HitSfx = AudioConfigAsset->GetHitSfx(HitComponent, Hit.GetComponent(), Hit.PhysMaterial.Get());
	if (!HitSfx)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGCore, Warning,
			TEXT("%s-%s: DoPlayHitSfx - No HitSfx found for PhysMaterial=%s and no DefaultHitSfx"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(Hit.PhysMaterial));
		return;
	}

	PlayHitSfx({ HitSfx, HitComponent, Hit, NormalImpulse, Volume }, TimeSeconds);
}

void UPGHitSfxComponent::OnPlayHitSfx_Implementation(UPrimitiveComponent* HitComponent, const FHitResult& Hit, USoundBase* HitSfx)
{
	// TODO: There should be some way to reset this
	++HitPlayCount;

	UE_VLOG_UELOG(GetOwner(), LogPGCore, Log,
		TEXT("%s-%s: OnPlaySfx - %s: PlayCount=%d"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(HitSfx),
		HitPlayCount)
}

float UPGHitSfxComponent::GetAudioVolume(const FVector& NormalImpulse) const
{
	if(!ensure(AudioConfigAsset))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGCore, Error,
			TEXT("%s-%s: GetAudioVolume - AudioConfigAsset was NULL - returning 1.0"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		return 1.0f;
	}

	const double Size = NormalImpulse.Size();

	if (Size < AudioConfigAsset->MinVolumeImpulse)
	{
		return 0.0f;
	}
	if (Size >= AudioConfigAsset->MaxVolumeImpulse)
	{
		return AudioConfigAsset->MaxAudioVolume;
	}

	// Interpolate size between min and max and scale to volume
	const float Alpha = FMath::Min((Size - AudioConfigAsset->MinVolumeImpulse) / (static_cast<double>(AudioConfigAsset->MaxVolumeImpulse) - AudioConfigAsset->MinVolumeImpulse), 1.0);

	return FMath::InterpEaseInOut(AudioConfigAsset->MinAudioVolume, AudioConfigAsset->MaxAudioVolume, Alpha, AudioConfigAsset->VolumeEaseFactor);
}

void UPGHitSfxComponent::PlayHitSfx(const FHitSfxContext& HitSfxContext, float TimeSeconds)
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		DoPlayHitSfx(HitSfxContext, TimeSeconds);
	}

	if (GetOwnerRole() == ROLE_Authority && bMulticastHitSfx)
	{
		MulticastPlayHitSfx(HitSfxContext);
	}
}

void UPGHitSfxComponent::DoPlayHitSfx(const FHitSfxContext& HitSfxContext, float TimeSeconds)
{
	if (!ensure(HitSfxContext.HitSfx))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGCore, Error,
			TEXT("%s-%s: DoPlayHitSfx - HitSfx was NULL"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		return;
	}

	auto SpawnedAudioComponent = UGameplayStatics::SpawnSoundAtLocation(
		GetOwner(),
		HitSfxContext.HitSfx,
		HitSfxContext.Hit.Location, HitSfxContext.NormalImpulse.Rotation(),
		HitSfxContext.Volume
	);

	if (!SpawnedAudioComponent)
	{
		// This is not an error condition as the component may not spawn if the sound is not audible,
		// for example it attenuates below a threshold based on distance or is filtered by sound concurrency
		UE_VLOG_UELOG(GetOwner(), LogPGCore, Log,
			TEXT("%s-%s: DoPlayHitSfx - Unable to spawn audio component for sfx=%s; volume=%.3f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(HitSfxContext.HitSfx), HitSfxContext.Volume);
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGCore, Log,
		TEXT("%s-%s: DoPlayHitSfx -  Playing sfx=%s at volume=%.3f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *HitSfxContext.HitSfx->GetName(), HitSfxContext.Volume);

	SpawnedAudioComponent->bAutoDestroy = true;
	SpawnedAudioComponent->bReverb = true;

	LastHitPlayTimeSeconds = TimeSeconds;

	OnPlayHitSfx(HitSfxContext.HitComponent, HitSfxContext.Hit, HitSfxContext.HitSfx);
}

void UPGHitSfxComponent::MulticastPlayHitSfx_Implementation(const FHitSfxContext& HitSfxContext)
{
	if (GetOwnerRole() != ROLE_SimulatedProxy)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGCore, Log,
			TEXT("%s-%s: MulticastPlayHitSfx - Skipping role=%s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(GetOwnerRole()));

		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGCore, Log,
		TEXT("%s-%s: MulticastPlayHitSfx"),
		*LoggingUtils::GetName(GetOwner()), *GetName());

	if (auto World = GetWorld(); ensure(World))
	{
		DoPlayHitSfx(HitSfxContext, World->GetTimeSeconds());

	}
}

#pragma endregion Collisions