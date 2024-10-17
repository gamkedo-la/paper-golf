// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TutorialAction.generated.h"

class APlayerController;
class APGHUD;

/**
 * 
 */
UCLASS(Abstract)
class UTutorialAction : public UObject
{
	GENERATED_BODY()

public:
	virtual bool IsRelevant() const { return !bIsCompleted; }
	virtual bool IsActive() const { return MessageTimerHandle.IsValid(); }

	virtual void Execute() PURE_VIRTUAL(UTutorialAction::Execute, );
	virtual void Abort();

protected:
	void ShowMessages(const TArray<FText>& Messages, float MessageDuration = -1.0f);
	APlayerController* GetPlayerController() const;
	APGHUD* GetHUD() const;
	// TODO: Save game state
	virtual void MarkCompleted() { bIsCompleted = true;  }

private:
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	float MaxMessageDuration{ 5.0f };

	FTimerHandle MessageTimerHandle{};
	int32 LastMessageIndex{};
	bool bIsCompleted{};
};
