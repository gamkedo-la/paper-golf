// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include "PaperGolfTypes.h"

#include "PGAITypes.generated.h"

class AGolfPlayerState;
class APaperGolfPawn;

USTRUCT()
struct FAIShotContext
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<APaperGolfPawn> PlayerPawn{};

	UPROPERTY(Transient)
	TObjectPtr<AGolfPlayerState> PlayerState{};

	UPROPERTY(Transient)
	AActor* GolfHole{};

	UPROPERTY(Transient)
	TArray<FShotFocusScores> FocusActorScores{};

	// TODO: May want to pass in the full array of focus actors to select another target

	EShotType ShotType{ EShotType::Default };

	FString ToString() const;
};

USTRUCT()
struct FAIShotSetupResult
{
	GENERATED_BODY()

	FFlickParams FlickParams{};

	UPROPERTY(Transient)
	TObjectPtr<AActor> FocusActor{};

	float ShotPitch{};
	float ShotYaw{};

	FString ToString() const;
};
