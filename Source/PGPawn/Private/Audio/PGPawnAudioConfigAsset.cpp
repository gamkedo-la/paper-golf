// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Audio/PGPawnAudioConfigAsset.h"

#include "Logging/LoggingUtils.h"
#include "PGPawnLogging.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PGPawnAudioConfigAsset)

const TMap<UPhysicalMaterial*, USoundBase*>& UPGPawnAudioConfigAsset::SelectPhysicalMaterialSoundsForComponent(UPrimitiveComponent* Component) const
{
	const auto SfxType = SelectSfxType(Component);
	switch (SfxType)
	{
		case ESfxType::Bounce:
			return PhysicalMaterialHitToSfx;
		case ESfxType::Landing:
			return PhysicalMaterialLandingToSfx;
		default:
			ensureMsgf(false, TEXT("Unhandled SfxType=%d"), SfxType);
			return Super::SelectPhysicalMaterialSoundsForComponent(Component);
	}
}

USoundBase* UPGPawnAudioConfigAsset::SelectDefaultHitSoundForComponent(UPrimitiveComponent* Component) const
{
	const auto SfxType = SelectSfxType(Component);
	switch (SfxType)
	{
	case ESfxType::Bounce:
		return DefaultHitSfx;
	case ESfxType::Landing:
		return DefaultLandingSfx;
	default:
		ensureMsgf(false, TEXT("Unhandled SfxType=%d"), SfxType);
		return Super::SelectDefaultHitSoundForComponent(Component);
	}
}

UPGPawnAudioConfigAsset::ESfxType UPGPawnAudioConfigAsset::SelectSfxType(UPrimitiveComponent* Component) const
{
	// TODO: Implement by looking at the angular and linear velocity of component and if coming to rest then using the landing one
	// GetPhysicsAngularVelocityInDegrees and GetPhysicsLinearVelocity 
	return ESfxType::Bounce;
}
