// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Volume/WaterHazardBoundsVolume.h"

#include "Components/StaticMeshComponent.h"

#include "Utils/PGAudioUtilities.h"

#include "Pawn/PaperGolfPawn.h"

#include "Utils/CollisionUtils.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PGGameplayLogging.h"

#include "GameFramework/GameUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WaterHazardBoundsVolume)

AWaterHazardBoundsVolume::AWaterHazardBoundsVolume()
{
	HazardType = EHazardType::Water;
	Type = EPaperGolfVolumeOverlapType::Any;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetMobility(EComponentMobility::Static);
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionProfileName(PG::CollisionProfile::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);

	WaterTableMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Water Table"));
	WaterTableMesh->SetMobility(EComponentMobility::Static);
	WaterTableMesh->SetupAttachment(RootComponent);
	WaterTableMesh->SetCollisionProfileName("BlockAll");
	Mesh->SetGenerateOverlapEvents(false);
}

void AWaterHazardBoundsVolume::BeginPlay()
{
	Super::BeginPlay();

	if (!ensureMsgf(HighQualityWaterMaterial, TEXT("HighQualityWaterMaterial is NULL")))
	{
		UE_VLOG_UELOG(this, LogPGGameplay, Error, TEXT("%s: BeginPlay: HighQualityWaterMaterial is NULL"), *GetName());
	}
	if (!ensureMsgf(LowQualityWaterMaterial, TEXT("LowQualityWaterMaterial is NULL")))
	{
		UE_VLOG_UELOG(this, LogPGGameplay, Error, TEXT("%s: BeginPlay: LowQualityWaterMaterial is NULL"), *GetName());
	}

	SetWaterMaterial(true);

	FTimerHandle WaterMaterialTimerHandle;
	GetWorldTimerManager().SetTimer(WaterMaterialTimerHandle, this, &AWaterHazardBoundsVolume::PollWaterMaterial, 1.0f, true);
}

void AWaterHazardBoundsVolume::SetWaterMaterial(bool bForceUpdate)
{
	if (!Mesh)
	{
		return;
	}

	const bool bCalculatedWaterQuality = ShouldUseHighQualityWater();
	if (!bForceUpdate && bCalculatedWaterQuality == bUsingHighQualityWater)
	{
		return;
	}

	bUsingHighQualityWater = bCalculatedWaterQuality;
	UMaterialInterface* const WaterQualityMaterial = bCalculatedWaterQuality ? HighQualityWaterMaterial : LowQualityWaterMaterial;

	if (WaterQualityMaterial)
	{
		Mesh->SetMaterial(0, WaterQualityMaterial);
		UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: SetWaterMaterial : bForceUpdate=%s; bUsingHighQualityWater=%s; Material=%s"),
			*GetName(), LoggingUtils::GetBoolString(bForceUpdate), LoggingUtils::GetBoolString(bUsingHighQualityWater), *WaterQualityMaterial->GetName());
	}
}

bool AWaterHazardBoundsVolume::ShouldUseHighQualityWater() const
{
	const auto GameUserSettings = UGameUserSettings::GetGameUserSettings();
	if (!ensure(GameUserSettings))
	{
		return false;
	}

	return GameUserSettings->GetShadingQuality() >= WaterQualityThreshold;
}
