// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GolfHole.generated.h"

class APaperGolfPawn;
class APaperGolfGameStateBase;

UCLASS()
class PGGAMEPLAY_API AGolfHole : public AActor
{
	GENERATED_BODY()
	
public:	
	AGolfHole();

	UFUNCTION(BlueprintPure, Category = "Hole", meta = (DefaultToSelf = "WorldContextObject"))
	static AGolfHole* GetCurrentHole(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure)
	int32 GetHoleNumber() const;

protected:
	UFUNCTION(BlueprintCallable)
	void SetCollider(UPrimitiveComponent* Collider);

private:
	UFUNCTION()
	void OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void OnCheckScored();

	bool CheckedScored() const;

	void ClearTimer();

private:
	FTimerHandle CheckScoredTimerHandle{};

	TWeakObjectPtr<APaperGolfPawn> OverlappingPaperGolfPawn{};

	UPROPERTY(EditAnywhere, Category = "Config")
	int32 HoleNumber{};
};

#pragma region Inline Definitions

FORCEINLINE int32 AGolfHole::GetHoleNumber() const
{
	ensureAlwaysMsgf(HoleNumber > 0, TEXT("%s: HoleNumber is not set"), *GetName());

	return HoleNumber;
}

#pragma endregion Inline Definitions
