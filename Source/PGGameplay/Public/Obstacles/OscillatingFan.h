// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OscillatingFan.generated.h"

UCLASS()
class PGGAMEPLAY_API AOscillatingFan : public AActor
{
	GENERATED_BODY()
	
public:	
	AOscillatingFan();

	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty >& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

protected:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetInfluenceCollider(UPrimitiveComponent* InCollider);

	UFUNCTION(BlueprintImplementableEvent)
	void GetAirflowOriginAndDirection(FVector& OutOrigin, FVector& OutDirection) const;

private:
	UFUNCTION()
	void OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void SetForceActive(bool bActive);

private:
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> InfluenceCollider{};

	// We can use a bool since there is only one active player at a time
	bool bForceIsActive{};

	/* Max force strength */
	UPROPERTY(Editanywhere, Category = "Fan")
	float MaxForceStrength{ 1000.0f };

	/* Max distance from airflow origin where max force in effect before inverse square falloff takes effect. */
	UPROPERTY(Editanywhere, Category = "Fan")
	float MaxForceDistance{ 100.0f };

	UPROPERTY(EditAnywhere, Category = "Fan")
	float ForceRadialFalloffFactor{ 1.0f / (4 * PI) };
};
