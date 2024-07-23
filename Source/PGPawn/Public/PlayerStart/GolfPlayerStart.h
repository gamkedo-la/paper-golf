// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "GolfPlayerStart.generated.h"

/**
 * 
 */
UCLASS()
class PGPAWN_API AGolfPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Golf")
	int32 GetHoleNumber() const { return HoleNumber; }

private:
	UPROPERTY(EditAnywhere, Category = "Golf")
	int32 HoleNumber{};
};
