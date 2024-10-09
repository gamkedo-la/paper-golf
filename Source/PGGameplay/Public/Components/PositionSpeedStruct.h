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

	FPositionSpeedStruct();
	~FPositionSpeedStruct();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int distanceIn{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Meta = (MakeEditWidget = true))
	FVector position = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USplineComponent* spline;
};
