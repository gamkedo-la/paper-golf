// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Settings/PGGameUserSettings.h"

#include "PGSettingsLogging.h"
#include "Logging/LoggingUtils.h"
#include "Logging/StructuredLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PGGameUserSettings)

namespace
{
	 constexpr const TCHAR* PGGameUserSettingsSectionName = TEXT("/Script/PGSettings.PGGameUserSettings");
	 constexpr const TCHAR* OldMainVolumeKeyName = TEXT("MasterVolume");
}

void UPGGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	UE_LOGFMT(LogPGSettings, Display, "{Name}: ApplySettings: bCheckForCommandLineOverrides={HasOverrides}",
		*GetName(), LoggingUtils::GetBoolString(bCheckForCommandLineOverrides));

	Super::ApplySettings(bCheckForCommandLineOverrides);

	OnGameUserSettingsUpdated.Broadcast();
}

void UPGGameUserSettings::SaveSettings()
{
	check(GConfig);

	// Example of how to make a breaking change to the file format but work with the previous version
	// Remove the previous value key so we don't overwrite the new values on the next load and also clean up the file
/*	if (bRequiresConfigCleanup)
	{
		GConfig->RemoveKey(PGGameUserSettingsSectionName, OldMainVolumeKeyName, *GGameUserSettingsIni);
		bRequiresConfigCleanup = false;
	}*/

	Super::SaveSettings();
}

void UPGGameUserSettings::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);

	check(GConfig);

	// Port over any renamed settings that still exist in the ini file
	// Note that property redirectors DO NOT work in this case, and we have to do it manually
	// Example of how to make a breaking change to the file format but work with the previous version
/*	if (GConfig->DoesSectionExist(PGGameUserSettingsSectionName, *GGameUserSettingsIni))
	{
		float OldMainVolumeValue;
		if (GConfig->GetFloat(PGGameUserSettingsSectionName, OldMainVolumeKeyName, OldMainVolumeValue, *GGameUserSettingsIni))
		{
			UE_LOGFMT(LogPGSettings, Display, "Found MasterVolume in ini file, replacing default MainVolume value with loaded value {Value}", OldMainVolumeValue);
			MainVolume = OldMainVolumeValue;
			bRequiresConfigCleanup = true;
		}
	}*/
}
