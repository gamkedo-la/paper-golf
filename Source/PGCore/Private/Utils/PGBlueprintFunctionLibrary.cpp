// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Utils/PGBlueprintFunctionLibrary.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Misc/ConfigCacheIni.h"

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
