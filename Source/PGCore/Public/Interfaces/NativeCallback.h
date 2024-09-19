// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NativeCallback.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, NotBlueprintable)
class UNativeCallback : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PGCORE_API INativeCallback
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	virtual void ExecuteCallback() = 0;
};
