// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Audio/PGAudioConfigAsset.h"
#include "PGPawnAudioConfigAsset.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class UPGPawnAudioConfigAsset : public UPGAudioConfigAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(Category = "Audio", EditDefaultsOnly)
	TObjectPtr<USoundBase> FlickSfx{};

	UPROPERTY(Category = "Audio", EditDefaultsOnly)
	TObjectPtr<USoundBase> TurnStartSfx{};

protected:
	virtual const TMap<UPhysicalMaterial*, USoundBase*>& SelectPhysicalMaterialSoundsForComponent(UPrimitiveComponent* OwnerComponent, UPrimitiveComponent* HitComponent) const override;
	virtual USoundBase* SelectDefaultHitSoundForComponent(UPrimitiveComponent* OwnerComponent, UPrimitiveComponent* HitComponent) const override;


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

	UPROPERTY(Category = "Audio | Landing", EditDefaultsOnly)
	float LandingMaxLinearVelocity{ 100.0f };

	UPROPERTY(Category = "Audio | Landing", EditDefaultsOnly)
	float LandingMaxAngularVelocityDegrees{ 30.0f };
};
