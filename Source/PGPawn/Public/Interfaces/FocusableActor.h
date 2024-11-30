// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

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
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Focus")
	bool IsPreferredFocus() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Focus")
	float GetMinDistance2D() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Focus")
	float GetMinCosAngle() const;

	/*
	* Overrides the default focus trace end offset.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Focus")
	float GetFocusTraceEndOffset() const;
};
