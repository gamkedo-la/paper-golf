// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PGAudioUtilities.generated.h"


class USoundBase;

/**
 * 
 */
UCLASS()
class PGCORE_API UPGAudioUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Audio")
	static UAudioComponent* PlaySfxAtActorLocation(const AActor* Actor, USoundBase* Sound);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Audio")
	static UAudioComponent* PlaySfxAttached(const AActor* Actor, USoundBase* Sound);
	
};
