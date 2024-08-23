// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/PGHitSfxComponent.h"
#include "PaperGolfPawnAudioComponent.generated.h"

class UPGPawnAudioConfigAsset;
class UAudioComponent;

/**
 * 
 */
UCLASS()
class UPaperGolfPawnAudioComponent : public UPGHitSfxComponent
{
	GENERATED_BODY()
	
public:
	UPaperGolfPawnAudioComponent();

	UFUNCTION(BlueprintCallable)
	void PlayFlick();

	UFUNCTION(BlueprintCallable)
	void PlayTurnStart();

	// Ensure replication always disabled
	virtual ELifetimeCondition GetReplicationCondition() const override { return ELifetimeCondition::COND_Never; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void RegisterCollisions() override;

	virtual void OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse) override;

private:
	bool ShouldPlayAudio() const;

	void PlayFlight();
	void StopFlight();
	void CancelFlightAudioTimer();

private:
	UPROPERTY(Transient)
	TObjectPtr<UPGPawnAudioConfigAsset> PawnAudioConfig{};

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> FlightAudioComponent{};

	FTimerHandle FlightAudioTimerHandle{};
	bool bPlayFlightRequested{};
};

#pragma region Inline Definitions

FORCEINLINE bool UPaperGolfPawnAudioComponent::ShouldPlayAudio() const
{
	return GetNetMode() != NM_DedicatedServer;
}

#pragma endregion Inline Definitions
