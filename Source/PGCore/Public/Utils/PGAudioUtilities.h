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

	UFUNCTION(BlueprintCallable, Category = "Audio")
	static UAudioComponent* PlaySfxAtActorLocation(const AActor* Actor, USoundBase* Sound);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	static UAudioComponent* PlaySfxAtLocation(const AActor* ContextObject, const FVector& Location, USoundBase* Sound);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	static UAudioComponent* PlaySfxAttached(const AActor* Actor, USoundBase* Sound);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DefaultToSelf = "WorldContextObject"))
	static void PlaySfx2D(const AActor* Owner, USoundBase* Sound);
};
