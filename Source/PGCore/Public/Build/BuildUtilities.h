// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "BuildCharacteristics.h"

#include "BuildUtilities.generated.h"

/**
 * 
 */
UCLASS()
class PGCORE_API UBuildUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Build")
	static FBuildCharacteristics GetBuildCharacteristics() { return {}; }

	UFUNCTION(BlueprintPure, Category = "Build")
	static bool IsShipping() { return GetBuildCharacteristics().bShipping; }

	UFUNCTION(BlueprintPure, Category = "Build")
	static bool IsNotShipping() { return !IsShipping(); }
};
