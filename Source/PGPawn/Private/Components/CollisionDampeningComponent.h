// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/BaseCollisionRelevanceComponent.h"
#include "CollisionDampeningComponent.generated.h"

class UCurveFloat;
class UStaticMeshComponent;

/**
 * 
 */
UCLASS()
class UCollisionDampeningComponent : public UBaseCollisionRelevanceComponent
{
	GENERATED_BODY()

public:
	UCollisionDampeningComponent();

	void OnFlick();
	void OnShotFinished();

protected:
	virtual void BeginPlay() override;

#pragma region Collisions
	virtual void RegisterCollisions() override;
	virtual void OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse) override;


private:
	void ResetState();
	bool ValidateConfig() const;

	void InitInitialDampeningValues();
	void UpdateDampeningValues();

	float GetDampeningValue(float InitialValue, const UCurveFloat* Curve) const;

	void DisableCollisionDampening();
	bool IsCollisionDampeningActive() const;
	bool ShouldEnableCollisionDampening() const;

private:

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	bool bEnableCollisionDampening{};

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float FlickTimeDelay{ 0.2f };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float CollisionTimeRecordDelay{ 1.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<UCurveFloat> AngularDampeningCurve{};

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<UCurveFloat> LinearDampeningCurve{};

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> OwnerStaticMeshComponent{};

	float InitialAngularDampening{};
	float InitialLinearDampening{};

	float FlickTime{ -1.0f };
	float LastHitTime{};
	int32 HitCount{};

	// Don't need to store as weak object or UPROPERTY as just using it for comparison and not dereferencing
	// Make it a void* to make that apparent and prevent misuse
	const void* LastHitComponent{};
};
