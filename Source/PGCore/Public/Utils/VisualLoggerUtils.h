// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FVisualLogEntry;
class UStaticMeshComponent;

namespace PG::VisualLoggerUtils
{
	#if ENABLE_VISUAL_LOG
		PGCORE_API void DrawStaticMeshComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, const UStaticMeshComponent& Component, const FColor& Color);
	#else
		inline void DrawStaticMeshComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, const UStaticMeshComponent& Component, const FColor& Color) {}
	#endif 
}
