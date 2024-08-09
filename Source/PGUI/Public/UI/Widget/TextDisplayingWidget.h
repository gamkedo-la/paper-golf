// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TextDisplayingWidget.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable, BlueprintType)
class UTextDisplayingWidget : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PGUI_API ITextDisplayingWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "UI")
	void SetText(const FText& Text);
};
