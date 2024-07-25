// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TutorialTrackingSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class PGUI_API UTutorialTrackingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	UTutorialTrackingSubsystem();
	

public:
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

private:

	inline static constexpr int32 MaxHoles = 18;

	bool bTutorialSeen{};
	TArray<bool> HoleFlybySeen{};
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
	return bTutorialSeen;
}

FORCEINLINE void UTutorialTrackingSubsystem::MarkTutorialSeen()
{
	bTutorialSeen = true;
}

FORCEINLINE void UTutorialTrackingSubsystem::MarkHoleFlybySeen(int32 HoleNumber, bool bSeen)
{
	if (!ensureAlwaysMsgf(HoleNumber > 0 && HoleNumber <= MaxHoles, TEXT("HoleNumber=%d must be between 1 and 18"), HoleNumber))
	{
		HoleNumber = FMath::Clamp(HoleNumber, 1, MaxHoles);
	}

	HoleFlybySeen[HoleNumber - 1] = bSeen;
}

#pragma endregion Inline Definitions
