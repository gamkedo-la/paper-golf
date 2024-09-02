// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "PGAITypes.h"

#include "Logging/LoggingUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PGAITypes)


FString FAIShotSetupResult::ToString() const
{
	return FString::Printf(TEXT("FlickParams=[%s]; FocusActor=%s; ShotPitch=%.1f; ShotYaw=%.1f"),
		*FlickParams.ToString(), *LoggingUtils::GetName(FocusActor), ShotPitch, ShotYaw);
}
