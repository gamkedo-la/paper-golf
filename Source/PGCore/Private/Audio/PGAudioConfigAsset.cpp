// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Audio/PGAudioConfigAsset.h"

#include "Logging/LoggingUtils.h"
#include "PGCoreLogging.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PGAudioConfigAsset)


USoundBase* UPGAudioConfigAsset::GetHitSfx(UPrimitiveComponent* OwnerComponent, UPrimitiveComponent* HitComponent, UPhysicalMaterial* PhysicalMaterial) const
{
	if (PhysicalMaterial)
	{
		const auto& PhysicalMaterialToSfx = SelectPhysicalMaterialSoundsForComponent(OwnerComponent, HitComponent);

		auto MatchedPhysicalMaterialSfx = PhysicalMaterialToSfx.Find(PhysicalMaterial);
		if (MatchedPhysicalMaterialSfx)
		{
			const auto HitSurfaceSound = *MatchedPhysicalMaterialSfx;
			UE_VLOG_UELOG(this, LogPGCore, Log, TEXT("%s: GetHitSound: Matched PhysicalMaterial=%s to HitSFX=%s"),
				*GetName(), *PhysicalMaterial->GetName(), *LoggingUtils::GetName(HitSurfaceSound));

			return HitSurfaceSound;
		}
	}

	const auto SelectedDefaultHitSfx = SelectDefaultHitSoundForComponent(OwnerComponent, HitComponent);

	UE_VLOG_UELOG(this, LogPGCore, Log, TEXT("%s: GetHitSound: Using No match to PhysicalMaterial=%s; using DefaultHitSfx=%s"),
		*GetName(), *LoggingUtils::GetName(PhysicalMaterial), *LoggingUtils::GetName(SelectedDefaultHitSfx));

	return SelectedDefaultHitSfx;
}
