// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/PGTurnBasedGameMode.h"
#include "PGTutorialGameMode.generated.h"

/**
 * 
 */
UCLASS()
class APGTutorialGameMode : public APGTurnBasedGameMode
{
	GENERATED_BODY()

public:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;

protected:
	virtual void CourseCompletionNextAction() override;

private:
	void MarkTutorialComplete();
	
private:
	FString NextCourseOptions{};
};
