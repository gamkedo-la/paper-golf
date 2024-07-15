// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FocusableActor.generated.h"

UINTERFACE(MinimalAPI)
class UFocusableActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PGPAWN_API IFocusableActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Focus")
	int32 GetHoleNumber() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Focus")
	bool IsHole() const;
};
