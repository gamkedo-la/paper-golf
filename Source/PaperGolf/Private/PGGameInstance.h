// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "Input/InputCharacteristics.h"

#include "PGGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class UPGGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintPure, Category = "Controls")
	bool IsGamepadAvailable() const;

private:
	void InitLoadingScreen();

	UFUNCTION()
	void BeginLoadingScreen(const FString& MapName);

	UFUNCTION()
	void EndLoadingScreen(UWorld* InLoadedWorld);

	void DoLoadingScreen();

	void InitGamepadAvailable();
	void HandleControllerConnectionChange(EInputDeviceConnectionState InputDeviceConnectionState, FPlatformUserId UserId, FInputDeviceId ControllerId);
	void HandleControllerPairingChanged(FInputDeviceId ControllerId, FPlatformUserId NewUserId, FPlatformUserId OldUserId);
};

#pragma region Inline Definitions

FORCEINLINE bool UPGGameInstance::IsGamepadAvailable() const
{
	return PG::FInputCharacteristics::IsGamepadAvailable();
}

#pragma endregion Inline Definitions
