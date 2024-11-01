// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "EnhancedInputComponent.h"

#include "TutorialAction.generated.h"

class APlayerController;
class APGHUD;
class APaperGolfPawn;
class IGolfController;
class UTutorialConfigDataAsset;
class UInputAction;
class UEnhancedInputComponent;

/**
 * 
 */
UCLASS(Abstract)
class UTutorialAction : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(UTutorialConfigDataAsset* ConfigAsset);

	virtual bool IsRelevant() const { return !bIsCompleted; }
	virtual bool IsActive() const { return MessageTimerHandle.IsValid(); }

	bool IsCompleted() const { return bIsCompleted; }

	virtual void Execute();
	virtual void Abort();

	virtual FString GetActionName() const { return GetClass()->GetName(); }

protected:
	void ShowMessages(const TArray<FText>& Messages, float MessageDuration = -1.0f);
	
	APlayerController* GetPlayerController() const;
	IGolfController* GetGolfController() const;
	APaperGolfPawn* GetPlayerPawn() const;
	APGHUD* GetHUD() const;
	UEnhancedInputComponent* GetInputComponent() const;

	virtual void RegisterInputBindings();
	virtual void UnregisterInputBindings();

	// TODO: Save game state

	virtual void OnMessageShown(int32 Index, int32 NumMessages) {};
	virtual void OnPlayerSkipped();

	virtual void MarkCompleted();
	virtual bool ShouldMarkCompletedOnLastMessageDismissed() const { return true; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	float MaxMessageDuration{ 5.0f };

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SkipTutorialAction{};

	uint32 SkipTutorialBindingHandle{};

	FTimerHandle MessageTimerHandle{};
	int32 LastMessageIndex{};
	bool bIsCompleted{};
};
