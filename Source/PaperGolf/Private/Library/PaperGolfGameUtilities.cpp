// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Library/PaperGolfGameUtilities.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfGameUtilities)

APlayerController* UPaperGolfGameUtilities::GetLocalPlayerController(UObject* WorldContextObject)
{
	auto GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetFirstLocalPlayerController(WorldContextObject ? WorldContextObject->GetWorld() : nullptr);
}

APawn* UPaperGolfGameUtilities::GetLocalPlayer(UObject* WorldContextObject)
{
	auto PlayerController = GetLocalPlayerController(WorldContextObject);
	if (!PlayerController)
	{
		return nullptr;
	}

	return PlayerController->GetPawn();
}

APlayerCameraManager* UPaperGolfGameUtilities::GetLocalPlayerCameraManager(UObject* WorldContextObject)
{
	auto PlayerController = GetLocalPlayerController(WorldContextObject);
	if (!PlayerController)
	{
		return nullptr;
	}

	return PlayerController->PlayerCameraManager;
}

APlayerState* UPaperGolfGameUtilities::GetLocalPlayerState(UObject* WorldContextObject)
{
	auto PlayerController = GetLocalPlayerController(WorldContextObject);
	if (!PlayerController)
	{
		return nullptr;
	}

	return PlayerController->PlayerState;
}

FString UPaperGolfGameUtilities::GetCurrentMapName(UObject* WorldContextObject)
{
	if (!ensure(WorldContextObject))
	{
		return {};
	}

	const auto World = WorldContextObject->GetWorld();
	if (!ensure(World))
	{
		return {};
	}

	FString MapName = World->GetMapName();
	MapName.RemoveFromStart(World->StreamingLevelsPrefix); // Remove any prefix if necessary

	return MapName;
}
