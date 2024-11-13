// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "OscillatingFan.generated.h"

UCLASS()
class PGGAMEPLAY_API AOscillatingFan : public AActor, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_BODY()
	
public:	
	AOscillatingFan();

	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty >& OutLifetimeProps) const override;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetInfluenceCollider(UPrimitiveComponent* InCollider);

	UFUNCTION(BlueprintImplementableEvent)
	void GetAirflowOriginAndDirection(FVector& OutOrigin, FVector& OutDirection) const;

	UFUNCTION(BlueprintImplementableEvent)
	UStaticMeshComponent* GetFanHeadMesh() const;

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
	float ForceRadialFalloffDistance{ 5000.0f };

#if ENABLE_VISUAL_LOG
	FTimerHandle VisualLoggerTimer{};
#endif
};
