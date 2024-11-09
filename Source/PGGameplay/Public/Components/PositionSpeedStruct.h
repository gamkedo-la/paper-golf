// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "PositionSpeedStruct.generated.h"

USTRUCT(BlueprintType)
struct PGGAMEPLAY_API FPositionSpeedStruct
{
public:
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 distanceIn{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Meta = (MakeEditWidget = true))
	FVector position = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USplineComponent> spline{};
};
