// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "BuildCharacteristics.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct PAPERGOLF_API FBuildCharacteristics
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
