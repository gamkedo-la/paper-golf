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

	/*
	* Encodes the course options.
	*/
	UFUNCTION(BlueprintPure, Category = "Load Level")
	static FString CreateCourseOptionsUrl(const FString& Map, const FString& GameMode, int32 NumPlayers, int32 NumBots, int32 AllowBots = -1, int32 MaxPlayers = -1);
	
	/*
	* Encodes the next course options into the Options string.
	*/
	UFUNCTION(BlueprintPure, Category = "Load Level", meta = (DisplayName = "Encode Next Course Options (Ref)"))
	static void EncodeNextCourseOptionsInline(FString NextCourseOptions,  UPARAM(ref) FString& Options);

	/*
	* Encodes the next course options into the Options string and returns the new string.
	*/
	UFUNCTION(BlueprintPure, Category = "Load Level", meta = (DisplayName = "Encode Next Course Options"))
	static FString EncodeNextCourseOptions(FString NextCourseOptions, const FString& Options);

	/*
	* Decodes the next course options from the Options string.  The returned string can be passed to server travel.
	*/
	UFUNCTION(BlueprintPure, Category = "Load Level")
	static FString DecodeNextCourseOptions(const FString& Options);
};

#pragma region Inline Definitions

FORCEINLINE FString UPaperGolfGameUtilities::EncodeNextCourseOptions(FString NextCourseOptions, const FString& Options)
{
	FString UpdatedOptions = Options;
	EncodeNextCourseOptionsInline(NextCourseOptions, UpdatedOptions);

	return UpdatedOptions;
}

#pragma endregion Inline Definitions