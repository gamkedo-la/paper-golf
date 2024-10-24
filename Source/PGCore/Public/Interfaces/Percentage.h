// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Percentage.generated.h"

// This class does not need to be modified.
// Using NotBlueprintable causes the type to not show up at all in blueprints so use the meta = (CannotImplementInterfaceInBlueprint)
// as we can't allow this to be implemented by a blueprint class but want to expose the type to blueprints
UINTERFACE(MinimalAPI, BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UPercentage : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PGCORE_API IPercentage
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable)
	virtual float GetCurrentValue() const = 0;

	UFUNCTION(BlueprintCallable)
	virtual float GetMaxValue() const = 0;

	/*
	* Returns fraction of max value [0,1].
	*/
	UFUNCTION(BlueprintCallable)
	virtual float GetValuePercent() const;
};


#pragma region Inline Definitions

inline float IPercentage::GetValuePercent() const
{
	const auto Max = GetMaxValue();
	return Max > 0 ? GetCurrentValue() / Max : 0.0f;
}
#pragma endregion Inline Definitions

