// Copyright Game Salutes. All Rights Reserved.


#include "Library/PaperGolfGameUtilities.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/PlayerController.h"

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
