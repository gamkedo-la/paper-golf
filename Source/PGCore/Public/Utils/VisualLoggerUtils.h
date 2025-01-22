// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "VisualLoggerUtils.generated.h"

struct FVisualLogEntry;
class UStaticMeshComponent;

UCLASS()
class PGCORE_API UVisualLoggerUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Visual Logger", meta = (DevelopmentOnly, DisplayName = "Is VisLog Recording"))
	static bool IsRecording();

	/*
	* Starts the visual logger recording if it isn't already recording.  Equivalent to console command "vislog record"
	*/
	UFUNCTION(BlueprintCallable, Category = "Visual Logger", meta = (DevelopmentOnly, DisplayName="Start VisLog Recording"))
	static void StartRecording();

	/*
	* Stops the visual logger recording.  Equivalent to console command "vislog stop"
	*/
	UFUNCTION(BlueprintCallable, Category = "Visual Logger", meta = (DevelopmentOnly, DisplayName = "Stop VisLog recording"))
	static void StopRecording();
};

namespace PG::VisualLoggerUtils
{
	#if ENABLE_VISUAL_LOG
		PGCORE_API void DrawPrimitiveComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, const UPrimitiveComponent& Component, const FColor& Color, bool bUseWires = false);
		PGCORE_API void DrawStaticMeshComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, const UStaticMeshComponent& Component, const FColor& Color, bool bUseWires = false);
		PGCORE_API void StartAutomaticRecording(const UObject* Context);
		PGCORE_API void RecheckAutomaticRecording(const UObject* Context);
		PGCORE_API void StopAutomaticRecording(const UObject* Context);
	#else
		inline void DrawPrimitiveComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, const UPrimitiveComponent& Component, const FColor& Color, bool bUseWires = false) {}
		inline void DrawStaticMeshComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, const UStaticMeshComponent& Component, const FColor& Color, bool bUseWires = false) {}
		inline void StartAutomaticRecording(const UObject* Context) {}
		inline void RecheckAutomaticRecording(const UObject* Context) {}
		inline void StopAutomaticRecording(const UObject* Context) {}
	#endif 
}

#if !ENABLE_VISUAL_LOG
inline bool UVisualLoggerUtils::IsRecording() { return false; }
inline void UVisualLoggerUtils::StartRecording() {}
inline void UVisualLoggerUtils::StopRecording() {}
#endif
