// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include "PaperGolfTypes.generated.h"

UENUM(BlueprintType)
enum class EShotType : uint8
{
	Default	UMETA(Hidden),
	Full,
	Medium,
	Close,
	MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EShotFocusType : uint8
{
	Hole,
	Focus
};

USTRUCT()
struct FShotFocusScores
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<AActor> FocusActor{};

	float Score{};
};
