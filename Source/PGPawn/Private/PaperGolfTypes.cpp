// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "PaperGolfTypes.h"

#include "Logging/LoggingUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfTypes)


void FFlickParams::Clamp()
{
	PowerFraction = FMath::Clamp(PowerFraction, 0.0f, 1.0f);
	Accuracy = FMath::Clamp(Accuracy, -1.0f, 1.0f);
}

FString FFlickParams::ToString() const
{
	return FString::Printf(TEXT("ShotType=%s; LocalZOffset=%f; PowerFraction=%f; Accuracy=%f"), *LoggingUtils::GetName(ShotType), LocalZOffset, PowerFraction, Accuracy);
}
