// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "PGGameUserSettings.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameUserSettingsUpdated);

/**
 * 
 */
UCLASS(BlueprintType, config = GameUserSettings, configdonotcheckdefaults)
class PGSETTINGS_API UPGGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, DisplayName = "Get PG GameUserSettings", Category = "GameUserSettings")
	static UPGGameUserSettings* GetInstance();

	UFUNCTION(BlueprintPure, Category = "GameUserSettings")
	static float GetMaxVolume();

	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;

	UFUNCTION(BlueprintCallable, Category = "GameUserSettings")
	void SetMainVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "GameUserSettings")
	float GetMainVolume() const;

	UFUNCTION(BlueprintCallable, Category = "GameUserSettings")
	void SetSfxVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "GameUserSettings")
	float GetSfxVolume() const;

	UFUNCTION(BlueprintCallable, Category = "GameUserSettings")
	void SetMusicVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "GameUserSettings")
	float GetMusicVolume() const;

	UFUNCTION(BlueprintCallable, Category = "GameUserSettings")
	void SetAmbienceVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "GameUserSettings")
	float GetAmbienceVolume() const;


	virtual void SaveSettings() override;

	virtual void LoadSettings(bool bForceReload = false) override;

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnGameUserSettingsUpdated OnGameUserSettingsUpdated {};

private:

	static constexpr float MaxVolume = 2.0f;

	UPROPERTY(Config)
	float MainVolume{ 1.0f };

	UPROPERTY(Config)
	float SfxVolume{ 1.0f };

	UPROPERTY(Config)
	float MusicVolume{ 1.0f };

	UPROPERTY(Config)
	float AmbienceVolume{ 1.0f };

	bool bRequiresConfigCleanup{};
};

#pragma region Inline Definitions

FORCEINLINE float UPGGameUserSettings::GetMaxVolume()
{
	return MaxVolume;
}

FORCEINLINE UPGGameUserSettings* UPGGameUserSettings::GetInstance()
{
	return CastChecked<UPGGameUserSettings>(GetGameUserSettings());
}

FORCEINLINE void UPGGameUserSettings::SetMainVolume(float Value)
{
	MainVolume = Value;
}

FORCEINLINE float UPGGameUserSettings::GetMainVolume() const
{
	return MainVolume;
}

FORCEINLINE void UPGGameUserSettings::SetSfxVolume(float Value)
{
	SfxVolume = Value;
}

FORCEINLINE float UPGGameUserSettings::GetSfxVolume() const
{
	return SfxVolume;
}

FORCEINLINE void UPGGameUserSettings::SetMusicVolume(float Value)
{
	MusicVolume = Value;
}

FORCEINLINE float UPGGameUserSettings::GetMusicVolume() const
{
	return MusicVolume;
}

FORCEINLINE void UPGGameUserSettings::SetAmbienceVolume(float Value)
{
	AmbienceVolume = Value;
}

FORCEINLINE float UPGGameUserSettings::GetAmbienceVolume() const
{
	return AmbienceVolume;
}

#pragma endregion Inline Definitions
