// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "Subsystems/GolfEvents.h"

#include "PenaltyHazard.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UPenaltyHazard : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PGGAMEPLAY_API IPenaltyHazard
{
	GENERATED_BODY()

public:
	virtual EHazardType GetHazardType() const = 0;
	virtual bool IsPenaltyOnImpact() const = 0;
};
