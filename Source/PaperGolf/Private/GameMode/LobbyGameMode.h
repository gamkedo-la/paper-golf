// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"

#include "Config/GameSessionConfig.h"

#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;

	UFUNCTION(BlueprintCallable)
	bool HostStartMatch();

	UFUNCTION(BlueprintPure)
	bool CanStartMatch() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	class UMultiplayerSessionsSubsystem* GetMultiplayerSessionsSubsystem() const;

	int32 GetNumBots() const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Game Session Config")
	TObjectPtr<UGameSessionConfig> GameSessionConfig{};

	bool bCanStartMatch{};
	bool bMatchStarted{};
};

#pragma region Inline Definitions


FORCEINLINE bool ALobbyGameMode::CanStartMatch() const
{
	return !bMatchStarted && bCanStartMatch;
}

#pragma endregion Inline Definitions
