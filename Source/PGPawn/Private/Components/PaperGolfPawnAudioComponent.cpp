// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/PaperGolfPawnAudioComponent.h"

#include "Audio/PGPawnAudioConfigAsset.h"

#include "Logging/LoggingUtils.h"
#include "PGPawnLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "Utils/PGAudioUtilities.h"

#include "Components/StaticMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfPawnAudioComponent)

UPaperGolfPawnAudioComponent::UPaperGolfPawnAudioComponent()
{
	bEnableCollisionSounds = true;
}

void UPaperGolfPawnAudioComponent::PlayFlick()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: PlayFlick"), *LoggingUtils::GetName(GetOwner()), *GetName());

	if (!PawnAudioConfig)
	{
		return;
	}

	// TODO: Play at flick location
	UPGAudioUtilities::PlaySfxAtActorLocation(GetOwner(), PawnAudioConfig->FlickSfx);
}

void UPaperGolfPawnAudioComponent::PlayTurnStart()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: PlayTurnStart"), *LoggingUtils::GetName(GetOwner()), *GetName());

	if (!PawnAudioConfig)
	{
		return;
	}

	UPGAudioUtilities::PlaySfxAtActorLocation(GetOwner(), PawnAudioConfig->TurnStartSfx);
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
