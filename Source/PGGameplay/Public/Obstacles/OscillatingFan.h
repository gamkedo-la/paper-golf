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

private:
	struct FAirflowData
	{
		FVector Origin { EForceInit::ForceInitToZero };
		FVector Direction { EForceInit::ForceInitToZero };
	};
	
	UFUNCTION()
	void OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void SetForceActive(bool bActive);

	bool ShouldApplyForceTo(const UPrimitiveComponent& Component, const FAirflowData& AirflowData) const;
	FVector CalculateAirflowForce(const UPrimitiveComponent& Component, const FAirflowData& AirflowData) const;
	void ApplyAirflowForce(UPrimitiveComponent& Component, const FVector& Force) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> InfluenceCollider{};

	// Only one active player at a time
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> OverlappedComponent{};

	/* Max force strength */
	UPROPERTY(Editanywhere, Category = "Fan")
	float MaxForceStrength{ 10000.0f };

	/* Max distance from airflow origin where max force in effect before inverse square falloff takes effect. */
	UPROPERTY(Editanywhere, Category = "Fan")
	float MaxForceDistance{ 100.0f };

	UPROPERTY(EditAnywhere, Category = "Fan")
	float ForceRadialFalloffConstantFactor{ 1.0f / (4 * PI) };

	UPROPERTY(EditAnywhere, Category = "Fan")
	float ForceRadialFalloffDistanceFactor { 100 * 5 };
};
