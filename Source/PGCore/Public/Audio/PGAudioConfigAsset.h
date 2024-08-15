// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PGAudioConfigAsset.generated.h"

class UPhysicalMaterial;
class USoundBase;

/**
 * 
 */
UCLASS()
class PGCORE_API UPGAudioConfigAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	USoundBase* GetHitSfx(UPrimitiveComponent* Component, UPhysicalMaterial* PhysicalMaterial) const;

protected:
	virtual const TMap<UPhysicalMaterial*, USoundBase*>& SelectPhysicalMaterialSoundsForComponent(UPrimitiveComponent* Component) const { return PhysicalMaterialHitToSfx; }
	virtual USoundBase* SelectDefaultHitSoundForComponent(UPrimitiveComponent* Component) const { return DefaultHitSfx; }

public:

	UPROPERTY(EditAnywhere, Category = "Audio | Hit")
	float MinAudioVolume{ 0.0f };

	UPROPERTY(EditAnywhere, Category = "Audio | Hit")
	float MaxAudioVolume{ 5.0f };

	UPROPERTY(EditAnywhere, Category = "Audio | Hit", Meta = (ClampMin = "1.0"))
	float VolumeEaseFactor{ 1.25f };

	UPROPERTY(EditAnywhere, Category = "Audio | Hit")
	float MinVolumeImpulse{ };

	UPROPERTY(EditAnywhere, Category = "Audio | Hit")
	float MaxVolumeImpulse{ 10000 };

	/* Minimum amount of time to wait in between triggering of the sfx */
	UPROPERTY(EditAnywhere, Category = "Audio | Hit")
	float MinPlayInterval{ 0.0f };

	/* Minimum game time elapsed before the sound will start playing */
	UPROPERTY(EditAnywhere, Category = "Audio | Hit")
	float MinPlayTimeSeconds{ 0.0f };

	/* Set to value > 0 to limit total number of triggers. */
	UPROPERTY(EditAnywhere, Category = "Audio")
	int32 MaxPlayCount{};

protected:
	UPROPERTY(Category = "Audio | Hit", EditDefaultsOnly)
	TMap<UPhysicalMaterial*, USoundBase*> PhysicalMaterialHitToSfx{};

	UPROPERTY(Category = "Audio | Hit", EditDefaultsOnly)
	TObjectPtr<USoundBase> DefaultHitSfx{};
};
