// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/BaseCollisionRelevanceComponent.h"
#include "PGHitSfxComponent.generated.h"

class UAudioComponent;
class USoundBase;
class UPGAudioConfigAsset;

/*
* Component base class for handling audio interactions for an actor.
*/
UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PGCORE_API UPGHitSfxComponent : public UBaseCollisionRelevanceComponent
{
	GENERATED_BODY()

public:	
	UPGHitSfxComponent();

protected:
	virtual void BeginPlay() override;

#pragma region Collisions
	virtual void RegisterCollisions() override;

	UFUNCTION(BlueprintImplementableEvent)
	bool BlueprintRegisterCollisions();

	virtual void OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse) override;

	UFUNCTION(BlueprintNativeEvent)
	bool ShouldPlayHitSfx(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse) const;

	UFUNCTION(BlueprintNativeEvent)
	void OnPlayHitSfx(UPrimitiveComponent* HitComponent, const FHitResult& Hit, USoundBase* HitSfx);

	virtual void OnPlayHitSfx_Implementation(UPrimitiveComponent* HitComponent, const FHitResult& Hit, USoundBase* HitSfx);

private:
	float GetAudioVolume(const FVector& NormalImpulse) const;

#pragma endregion Collisions

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Collision")
	bool bEnableCollisionSounds{ true };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<UPGAudioConfigAsset> AudioConfigAsset{};

private:
	float LastHitPlayTimeSeconds{ -1.0f };
	int32 HitPlayCount{};
};
