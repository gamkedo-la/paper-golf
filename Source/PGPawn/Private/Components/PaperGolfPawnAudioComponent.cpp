// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/PaperGolfPawnAudioComponent.h"

#include "Logging/LoggingUtils.h"
#include "PGPawnLogging.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfPawnAudioComponent)

UPaperGolfPawnAudioComponent::UPaperGolfPawnAudioComponent()
{
	// TODO: Enable once have the sounds ready and behavior defined
	bEnableCollisionSounds = false;
}

void UPaperGolfPawnAudioComponent::PlayFlick()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: PlayFlick"), *LoggingUtils::GetName(GetOwner()), *GetName());
}

void UPaperGolfPawnAudioComponent::PlayTurnStart()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: PlayTurnStart"), *LoggingUtils::GetName(GetOwner()), *GetName());
}
