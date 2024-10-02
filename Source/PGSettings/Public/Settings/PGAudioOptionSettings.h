// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PGAudioOptionSettings.generated.h"

class USoundMix;
class USoundClass;

/**
 * 
 */
UCLASS()
class PGSETTINGS_API UPGAudioOptionSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundMix> VolumeChangeMix{};

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundClass> MainSoundClass{};

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundClass> SfxSoundClass{};

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundClass> MusicSoundClass{};

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundClass> AmbienceSoundClass{};
	
};
