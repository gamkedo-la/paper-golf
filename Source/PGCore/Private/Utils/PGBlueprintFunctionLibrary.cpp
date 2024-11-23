// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Utils/PGBlueprintFunctionLibrary.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Misc/ConfigCacheIni.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PGBlueprintFunctionLibrary)

bool UPGBlueprintFunctionLibrary::IsRunningInEditor(const UObject* WorldContextObject)
{
	if(!WorldContextObject)
	{
		return false;
	}

	auto World = WorldContextObject->GetWorld();
	if (!World)
	{
		return false;
	}

	return !World->IsGameWorld();
}

FString UPGBlueprintFunctionLibrary::GetProjectVersion()
{
	FString GameVersion;
	GConfig->GetString(
		TEXT("/Script/EngineSettings.GeneralProjectSettings"),
		TEXT("ProjectVersion"),
		GameVersion,
		GGameIni
	);

	return FString::Printf(TEXT("v %s"), *GameVersion);
}

bool UPGBlueprintFunctionLibrary::FileLoadString(const FString& FileName, FString& Contents)
{
	// Check if file exists
	// Look first in content directory then saved directory
	for (const auto& Dir : { FPaths::ProjectContentDir(), FPaths::ProjectSavedDir() })
	{
		const auto FullPath = Dir + FileName;
		if (FPaths::FileExists(FullPath))
		{
			return FFileHelper::LoadFileToString(Contents, *FullPath);
		}
	}

	return false;
}

bool UPGBlueprintFunctionLibrary::FileSaveString(const FString& File, const FString& Contents)
{
	// Want this to save in the saved game directory
	return FFileHelper::SaveStringToFile(Contents, *(FPaths::ProjectSavedDir() + File));
}
