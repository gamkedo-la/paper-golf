// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include "BuildCharacteristics.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct PGCORE_API FBuildCharacteristics
{
public:
	GENERATED_BODY()

	FBuildCharacteristics();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bShipping{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bDevelopment{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bTest{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bEditor{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bGame{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bDrawDebug{};
};
