// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/PGHitSfxComponent.h"
#include "PaperGolfPawnAudioComponent.generated.h"

class UPGPawnAudioConfigAsset;

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

protected:
	virtual void BeginPlay() override;

	virtual void RegisterCollisions() override;

private:
	bool ShouldPlayAudio() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPGPawnAudioConfigAsset> PawnAudioConfig{};
};

#pragma region Inline Definitions

FORCEINLINE bool UPaperGolfPawnAudioComponent::ShouldPlayAudio() const
{
	return GetNetMode() != NM_DedicatedServer;
}

#pragma endregion Inline Definitions
