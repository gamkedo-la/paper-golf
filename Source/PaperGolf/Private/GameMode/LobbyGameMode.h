// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

class APaperGolfGameModeBase;

USTRUCT(BlueprintType)
struct FGameModeInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString Name{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<APaperGolfGameModeBase> GameMode{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 MinPlayers{};
};

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
	FString GetPathForMap(const TSoftObjectPtr<UWorld>& World) const;
	FString GetPathForGameMode(const TSoftClassPtr<APaperGolfGameModeBase>& GameMode) const;

	TSoftObjectPtr<UWorld> GetRandomMap() const;

	void ValidateMaps();
	void ValidateGameModes();

	class UMultiplayerSessionsSubsystem* GetMultiplayerSessionsSubsystem() const;

	int32 GetNumBots() const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Modes", meta = (TitleProperty = "GameMode"))
	TMap<FString, FGameModeInfo> MatchTypesToModes;

	UPROPERTY(EditDefaultsOnly, Category = "Maps")
	TArray<TSoftObjectPtr<UWorld>> Maps;

	bool bCanStartMatch{};
	bool bMatchStarted{};
};

#pragma region Inline Definitions


FORCEINLINE bool ALobbyGameMode::CanStartMatch() const
{
	return !bMatchStarted && bCanStartMatch;
}

#pragma endregion Inline Definitions
