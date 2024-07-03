// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FVisualLogEntry;
class UStaticMeshComponent;

namespace PG::VisualLoggerUtils
{
	#if ENABLE_VISUAL_LOG
		PGCORE_API void DrawStaticMeshComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, UStaticMeshComponent& Component);
	#else
		inline void DrawStaticMeshComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, UStaticMeshComponent& Component) {}
	#endif 
}
