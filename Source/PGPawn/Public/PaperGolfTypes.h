// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

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
