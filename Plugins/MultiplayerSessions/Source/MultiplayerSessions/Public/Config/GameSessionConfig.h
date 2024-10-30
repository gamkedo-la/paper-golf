// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameSessionConfig.generated.h"

class AGameModeBase;

USTRUCT(BlueprintType)
struct FGameModeInfo
{
	GENERATED_BODY()

public:
	bool IsValid() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString Name{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<AGameModeBase> GameMode{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 MinPlayers{};
};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class MULTIPLAYERSESSIONS_API UGameSessionConfig : public UDataAsset
{
	GENERATED_BODY()

public:

	UFUNCTION(Category = "Game Session Config", BlueprintCallable, meta = (DevelopmentOnly))
	void Validate();

	UFUNCTION(Category = "Game Session Config", BlueprintPure)
	bool IsValid() const;

	UFUNCTION(BlueprintPure, Category = "Game Session Config")
	static const FString& GetRandomMapName() { return RandomMapName; }

	UFUNCTION(BLueprintPure, Category = "Maps")
	static bool IsRandomMap(const FString& MapName);

	UFUNCTION(BlueprintPure, Category = "Maps")
	TSoftObjectPtr<UWorld> GetMap(const FString& MapName) const;

	UFUNCTION(BlueprintPure, Category = "Maps")
	TSoftObjectPtr<UWorld> GetRandomMap() const;

	UFUNCTION(BlueprintPure, Category = "Maps")
	FString GetRandomizedMapName() const;

	UFUNCTION(BlueprintPure, Category = "Maps")
	static FString GetPathForMap(const TSoftObjectPtr<UWorld>& World);

	UFUNCTION(BlueprintPure, Category = "Maps")
	FString GetPathForMapName(const FString& MapName) const;

	UFUNCTION(BlueprintPure, Category = "Maps")
	TArray<FString> GetAllMapNames() const;

	// Have to define it this way as cannot return pointers to USTRUCT for blueprint accessible interfaces
	UFUNCTION(BlueprintPure, Category = "Modes")
	const FGameModeInfo& GetGameModeInfo(const FString& MatchType, bool& bOutValid) const;

	UFUNCTION(BlueprintPure, Category = "Modes")
	static FString GetGameModePath(const TSoftClassPtr<AGameModeBase>& GameMode);

	UFUNCTION(BlueprintPure, Category = "Modes")
	TArray<FGameModeInfo> GetAllGameModes() const;

	UFUNCTION(BlueprintPure, Category = "Modes")
	TMap<FString, FString> GetMatchTypesToModeNames() const;

	UFUNCTION(BlueprintPure, Category = "Modes")
	TArray<FString> GetAllMatchTypeKeys() const;

	UFUNCTION(BlueprintPure, Category = "Modes")
	TArray<FString> GetAllGameModeNames() const;

private:
	void ValidateMaps();
	void ValidateGameModes();

private:

	static const FString RandomMapName;

	UPROPERTY(EditDefaultsOnly, Category = "Modes", meta = (TitleProperty = "GameMode"))
	TMap<FString, FGameModeInfo> MatchTypesToModes;

	UPROPERTY(EditDefaultsOnly, Category = "Maps")
	TMap<FString, TSoftObjectPtr<UWorld>> Maps;

	UPROPERTY(EditDefaultsOnly, Category = "Maps")
	bool bEnableRandomMap{ true };
};

#pragma region Inline Definitions

FORCEINLINE FString UGameSessionConfig::GetPathForMapName(const FString& MapName) const
{
	return GetPathForMap(GetMap(MapName));
}

FORCEINLINE bool UGameSessionConfig::IsRandomMap(const FString& MapName)
{
	return MapName == RandomMapName;
}

#pragma endregion Inline Definitions