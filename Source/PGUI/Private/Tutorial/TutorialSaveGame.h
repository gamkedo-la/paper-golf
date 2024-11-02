// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TutorialSaveGame.generated.h"

class UTutorialTrackingSubsystem;
class UTutorialAction;

/**
 * 
 */
UCLASS()
class UTutorialSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	static inline constexpr uint32 CurrentVersion = 1;
	static const FString SlotName;

	uint32 GetVersion() const { return Version; }
	void Save(const UObject* WorldContextObject);
	void RestoreState(const UObject* WorldContextObject) const;

	bool IsTutorialCompleted(const UTutorialAction& Tutorial) const;

private:
	UTutorialTrackingSubsystem* GetTutorialTrackingSubsystem(const UObject* WorldContextObject) const;

private:
	UPROPERTY(SaveGame)
	uint32 Version{};

	UPROPERTY(SaveGame)
	TSet<FString> CompletedTutorials{};

	UPROPERTY(SaveGame)
	bool bTutorialHoleSeen{};
};
