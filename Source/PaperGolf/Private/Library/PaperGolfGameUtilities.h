// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PaperGolfGameUtilities.generated.h"

class APlayerController;
class APawn;
class APlayerCameraManager;
class APlayerState;

/**
 * 
 */
UCLASS()
class UPaperGolfGameUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/*
	* Get the local player controller regardless of being a listen server or a client.  For dedicated servers this will return NULL.
	*/
	UFUNCTION(BlueprintPure, Category = "Paper Golf", meta = (DefaultToSelf = "WorldContextObject"))
	static APlayerController* GetLocalPlayerController(UObject* WorldContextObject);

	/*
	* Get the local player pawn regardless of being a listen server or a client.  For dedicated servers this will return NULL.
	*/
	UFUNCTION(BlueprintPure, Category = "Paper Golf", meta = (DefaultToSelf = "WorldContextObject"))
	static APawn* GetLocalPlayer(UObject* WorldContextObject);

	/*
	* Gets the local player camera manager regardless of being a listen server or a client.  For dedicated servers this will return NULL.
	*/
	UFUNCTION(BlueprintPure, Category = "Paper Golf", meta = (DefaultToSelf = "WorldContextObject"))
	static APlayerCameraManager* GetLocalPlayerCameraManager(UObject* WorldContextObject);

	/*
	* Gets the local player state regardless of being a listen server or a client.  For dedicated servers this will return NULL.
	*/
	UFUNCTION(BlueprintPure, Category = "Paper Golf", meta = (DefaultToSelf = "WorldContextObject"))
	static APlayerState* GetLocalPlayerState(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "World", meta = (WorldContext = "WorldContextObject"))
	static FString GetCurrentMapName(UObject* WorldContextObject);
};
