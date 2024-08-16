// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

struct FVisualLogEntry;
class UStaticMeshComponent;

namespace PG::VisualLoggerUtils
{
	#if ENABLE_VISUAL_LOG
		PGCORE_API void DrawStaticMeshComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, const UStaticMeshComponent& Component, const FColor& Color);
		PGCORE_API void StartAutomaticRecording(const UObject* Context);
		PGCORE_API void StopAutomaticRecording(const UObject* Context);
	#else
		inline void DrawStaticMeshComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, const UStaticMeshComponent& Component, const FColor& Color) {}
		inline void StartAutomaticRecording(const UObject* Context) {}
		inline void StopAutomaticRecording(const UObject* Context) {}
	#endif 
}
