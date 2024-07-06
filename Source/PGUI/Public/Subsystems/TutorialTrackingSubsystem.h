// Copyright Game Salutes. All Rights Reserved.

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
	void MarkHoleFlybySeen(int32 HoleNumber);

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

FORCEINLINE void UTutorialTrackingSubsystem::MarkHoleFlybySeen(int32 HoleNumber)
{
	if (!ensureAlwaysMsgf(HoleNumber > 0 && HoleNumber <= MaxHoles, TEXT("HoleNumber=%d must be between 1 and 18"), HoleNumber))
	{
		HoleNumber = FMath::Clamp(HoleNumber, 1, MaxHoles);
	}

	HoleFlybySeen[HoleNumber - 1] = true;
}

#pragma endregion Inline Definitions
