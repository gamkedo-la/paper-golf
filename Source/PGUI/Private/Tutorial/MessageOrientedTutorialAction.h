// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Tutorial/TutorialAction.h"
#include "MessageOrientedTutorialAction.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class UMessageOrientedTutorialAction : public UTutorialAction
{
	GENERATED_BODY()

public:
	virtual void Execute() override;
	virtual void Abort() override;

protected:
	virtual void OnMessageShown(int32 Index, int32 NumMessages) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	TArray<FText> Messages;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	float MessageDuration{ 1.5f };

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	int32 MaxMessagePresentations{ 3 };

	int32 MessagePresentationCount{};
	
};
