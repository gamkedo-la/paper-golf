// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

class APaperGolfGameModeBase;

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

private:
	FString GetPathForMap(const TSoftObjectPtr<UWorld>& World) const;
	FString GetPathForGameMode(const TSoftClassPtr<APaperGolfGameModeBase>& GameMode) const;

	TSoftObjectPtr<UWorld> GetRandomMap() const;

	void ValidateMaps();
	void ValidateGameModes();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Modes")
	TMap<FString, TSoftClassPtr<APaperGolfGameModeBase>> MatchTypesToModes;

	UPROPERTY(EditDefaultsOnly, Category = "Maps")
	TArray<TSoftObjectPtr<UWorld>> Maps;


};
