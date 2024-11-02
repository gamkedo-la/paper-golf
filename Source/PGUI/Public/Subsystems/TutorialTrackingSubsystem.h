// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include <concepts>

#include "TutorialTrackingSubsystem.generated.h"

class UTutorialAction;
class APlayerController;
class UTutorialConfigDataAsset;
class UTutorialSaveGame;

/* Callback for when the hole fly by is complete or canceled.
   Should be bound to the triggering code and then executed from blueprints where the flyby is executed */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHoleFlybyComplete);

/**
 * 
 */
UCLASS()
class PGUI_API UTutorialTrackingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()	

public:

	UTutorialTrackingSubsystem();

	/** Implement this for initialization of instances of the system */
	virtual void Initialize(FSubsystemCollectionBase& Collection);
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnHoleFlybyComplete OnHoleFlybyComplete{};

	void InitializeTutorialActions(UTutorialConfigDataAsset* TutorialConfig, APlayerController* PlayerController);
	void DisplayNextTutorial(APlayerController* PlayerController);
	void HideActiveTutorial();
	void DestroyTutorialActions();

	void ResetTutorialState();

	UFUNCTION(BlueprintPure)
	bool IsOnHoleFlybyCompleteBound() const;

	UFUNCTION(BlueprintPure)
	bool IsHoleFlybySeen(int32 HoleNumber) const;

	UFUNCTION(BlueprintPure)
	bool IsTutorialSeen() const;

	UFUNCTION(BlueprintCallable)
	void MarkTutorialSeen();

	UFUNCTION(BlueprintCallable)
	void MarkHoleFlybySeen(int32 HoleNumber, bool bSeen = true);

	UFUNCTION(BlueprintCallable)
	void MarkAllHoleFlybysSeen(bool bSeen);

private:

	friend class UTutorialSaveGame;

	template<std::derived_from<UTutorialAction> T>
	void RegisterTutorialAction();

	void SaveTutorialState();
	void RestoreTutorialState();
	void RegisterResetTutorialConsoleCommand();

private:

	inline static constexpr int32 MaxHoles = 18;

	bool bTutorialHoleSeen{};
	TArray<bool> HoleFlybySeen{};

	UPROPERTY(Transient)
	TArray<UTutorialAction*> TutorialActions;

	UPROPERTY(Transient)
	TObjectPtr<UTutorialAction> CurrentTutorialAction{};

	UPROPERTY(Transient)
	TObjectPtr<UTutorialSaveGame> TutorialSaveGame{};

	UPROPERTY(Transient)
	TObjectPtr<UTutorialConfigDataAsset> TutorialConfig{};
	
	TWeakObjectPtr<APlayerController> LastPlayerController{};
};

#pragma region Inline Definitions
FORCEINLINE bool UTutorialTrackingSubsystem::IsHoleFlybySeen(int32 HoleNumber) const
{
	if (!ensureAlwaysMsgf(HoleNumber > 0 && HoleNumber <= MaxHoles, TEXT("HoleNumber=%d must be between 1 and 18"), HoleNumber))
	{
		HoleNumber = FMath::Clamp(HoleNumber, 1, MaxHoles);
	}

	return HoleFlybySeen[HoleNumber - 1];
}

FORCEINLINE bool UTutorialTrackingSubsystem::IsTutorialSeen() const
{
	return bTutorialHoleSeen;
}

FORCEINLINE void UTutorialTrackingSubsystem::MarkHoleFlybySeen(int32 HoleNumber, bool bSeen)
{
	if (!ensureAlwaysMsgf(HoleNumber > 0 && HoleNumber <= MaxHoles, TEXT("HoleNumber=%d must be between 1 and 18"), HoleNumber))
	{
		HoleNumber = FMath::Clamp(HoleNumber, 1, MaxHoles);
	}

	HoleFlybySeen[HoleNumber - 1] = bSeen;
}

FORCEINLINE bool UTutorialTrackingSubsystem::IsOnHoleFlybyCompleteBound() const
{
	return OnHoleFlybyComplete.IsBound();
}

#pragma endregion Inline Definitions
