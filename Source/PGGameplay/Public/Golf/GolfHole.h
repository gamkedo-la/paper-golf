// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GolfHole.generated.h"

class APaperGolfPawn;

UCLASS()
class PGGAMEPLAY_API AGolfHole : public AActor
{
	GENERATED_BODY()
	
public:	
	AGolfHole();

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
};
