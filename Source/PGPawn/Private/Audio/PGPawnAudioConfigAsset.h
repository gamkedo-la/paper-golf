// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Audio/PGAudioConfigAsset.h"
#include "PGPawnAudioConfigAsset.generated.h"

/**
 * 
 */
UCLASS()
class UPGPawnAudioConfigAsset : public UPGAudioConfigAsset
{
	GENERATED_BODY()
	
protected:
	virtual const TMap<UPhysicalMaterial*, USoundBase*>& SelectPhysicalMaterialSoundsForComponent(UPrimitiveComponent* Component) const override;
	virtual USoundBase* SelectDefaultHitSoundForComponent(UPrimitiveComponent* Component) const override;


private:
	enum class ESfxType
	{
		Bounce,
		Landing
	};

	ESfxType SelectSfxType(UPrimitiveComponent* Component) const;

private:
	UPROPERTY(Category = "Audio | Landing", EditDefaultsOnly)
	TMap<UPhysicalMaterial*, USoundBase*> PhysicalMaterialLandingToSfx{};

	UPROPERTY(Category = "Audio | Landing", EditDefaultsOnly)
	TObjectPtr<USoundBase> DefaultLandingSfx{};
};
