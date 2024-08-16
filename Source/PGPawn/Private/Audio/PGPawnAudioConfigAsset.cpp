// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Audio/PGPawnAudioConfigAsset.h"

#include "Logging/LoggingUtils.h"
#include "PGPawnLogging.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PGPawnAudioConfigAsset)

const TMap<UPhysicalMaterial*, USoundBase*>& UPGPawnAudioConfigAsset::SelectPhysicalMaterialSoundsForComponent(UPrimitiveComponent* OwnerComponent, UPrimitiveComponent* HitComponent) const
{
	const auto SfxType = SelectSfxType(OwnerComponent);
	switch (SfxType)
	{
		case ESfxType::Bounce:
			return PhysicalMaterialHitToSfx;
		case ESfxType::Landing:
			return PhysicalMaterialLandingToSfx;
		default:
			ensureMsgf(false, TEXT("Unhandled SfxType=%d"), SfxType);
			return Super::SelectPhysicalMaterialSoundsForComponent(OwnerComponent, HitComponent);
	}
}

USoundBase* UPGPawnAudioConfigAsset::SelectDefaultHitSoundForComponent(UPrimitiveComponent* OwnerComponent, UPrimitiveComponent* HitComponent) const
{
	const auto SfxType = SelectSfxType(OwnerComponent);
	switch (SfxType)
	{
	case ESfxType::Bounce:
		return DefaultHitSfx;
	case ESfxType::Landing:
		return DefaultLandingSfx;
	default:
		ensureMsgf(false, TEXT("Unhandled SfxType=%d"), SfxType);
		return Super::SelectDefaultHitSoundForComponent(OwnerComponent, HitComponent);
	}
}

UPGPawnAudioConfigAsset::ESfxType UPGPawnAudioConfigAsset::SelectSfxType(UPrimitiveComponent* Component) const
{
	if(!ensure(Component))
	{
		return ESfxType::Bounce;
	}

	const auto LinearVelocity = Component->GetPhysicsLinearVelocity();
	const auto AngularVelocity = Component->GetPhysicsAngularVelocityInDegrees();

	const auto bIsLanding = LinearVelocity.Size() <= LandingMaxLinearVelocity && AngularVelocity.Size() <= LandingMaxAngularVelocityDegrees;
	const auto SfxType = bIsLanding ? ESfxType::Landing : ESfxType::Bounce;

	UE_VLOG_UELOG(Component->GetOwner(), LogPGPawn, Verbose, TEXT("%s-%s: SelectSfxType - LinearVelocity=%s->%f m/s; AngularVelocity=%s -> %f deg/s; bIsLanding=%s"),
		*LoggingUtils::GetName(Component->GetOwner()), *GetName(),
		*LinearVelocity.ToCompactString(), LinearVelocity.Size() / 100,
		*AngularVelocity.ToCompactString(), AngularVelocity.Size(),
		LoggingUtils::GetBoolString(bIsLanding));

	return SfxType;
}
