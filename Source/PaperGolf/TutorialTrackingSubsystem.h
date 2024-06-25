// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TutorialTrackingSubsystem.generated.h"

// TODO: Refactor to UI module
/**
 * 
 */
UCLASS()
class PAPERGOLF_API UTutorialTrackingSubsystem : public UGameInstanceSubsystem
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
inline bool UTutorialTrackingSubsystem::IsHoleFlybySeen(int32 HoleNumber) const
{
	return HoleFlybySeen[HoleNumber];
}

inline bool UTutorialTrackingSubsystem::IsTutorialSeen() const
{
	return bTutorialSeen;
}

inline void UTutorialTrackingSubsystem::MarkTutorialSeen()
{
	bTutorialSeen = true;
}

inline void UTutorialTrackingSubsystem::MarkHoleFlybySeen(int32 HoleNumber)
{
	HoleFlybySeen[HoleNumber] = true;
}

#pragma endregion Inline Definitions
