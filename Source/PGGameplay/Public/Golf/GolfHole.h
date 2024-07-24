// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include "Interfaces/FocusableActor.h"
#include "GameFramework/Actor.h"
#include "GolfHole.generated.h"

class APaperGolfPawn;
class APaperGolfGameStateBase;

UCLASS()
class PGGAMEPLAY_API AGolfHole : public AActor, public IFocusableActor
{
	GENERATED_BODY()
	
public:	
	AGolfHole();

	UFUNCTION(BlueprintPure, Category = "Hole", meta = (DefaultToSelf = "WorldContextObject"))
	static AGolfHole* GetCurrentHole(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Hole", meta = (DefaultToSelf = "WorldContextObject"))
	static TArray<AGolfHole*> GetAllWorldHoles(const UObject* WorldContextObject, bool bSort = true);

protected:
	UFUNCTION(BlueprintCallable)
	void SetCollider(UPrimitiveComponent* Collider);

private:

	int32 GetHoleNumber_Implementation() const;
	bool IsHole_Implementation() const { return true; }

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

FORCEINLINE int32 AGolfHole::GetHoleNumber_Implementation() const
{
	ensureAlwaysMsgf(HoleNumber > 0, TEXT("%s: HoleNumber is not set"), *GetName());

	return HoleNumber;
}

#pragma endregion Inline Definitions
