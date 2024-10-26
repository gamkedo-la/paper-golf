// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/PGHitSfxComponent.h"
#include "PaperGolfPawnAudioComponent.generated.h"

class UPGPawnAudioConfigAsset;
class UAudioComponent;
struct FFlickParams;

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
	void PlayFlick(const FFlickParams& FlickParams, const FVector& FlickImpulse);

	UFUNCTION(BlueprintCallable)
	void PlayTurnStart();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty >& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void RegisterCollisions() override;

	virtual void OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse) override;

private:
	void CheckPlayFlight(const FVector& FlickImpulse);
	void PlayFlight();
	void StopFlight();
	void CancelFlightAudioTimer();

	UFUNCTION()
	void OnRep_PlayFlightRequested();

private:
	UPROPERTY(Transient)
	TObjectPtr<UPGPawnAudioConfigAsset> PawnAudioConfig{};

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> FlightAudioComponent{};

	FTimerHandle FlightAudioTimerHandle{};

	UPROPERTY(Transient, ReplicatedUsing = OnRep_PlayFlightRequested)
	bool bPlayFlightRequested{};
};

