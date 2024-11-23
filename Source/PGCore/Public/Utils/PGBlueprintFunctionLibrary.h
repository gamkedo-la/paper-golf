// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include <concepts>

#include "PGBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class PGCORE_API UPGBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Paper Golf|Utils", meta = (DefaultToSelf = "WorldContextObject"))
	static bool IsRunningInEditor(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Paper Golf|Utils")
	static FString GetProjectVersion();

	/*
	* Wraps between [Min,Max].  This version of wrap with proper behavior.  FMath::Wrap has a bug currently.
	* See https://forums.unrealengine.com/t/fmath-wrap-behavior/266612.
	*
	* We use <code>std::remove_cvref_t<T></code> to remove any const/volatile qualifiers during type deduction so that caller doesn't need to specify template argument if one happens to be "const".
	*/
	template<typename T> requires std::is_arithmetic_v<std::remove_cvref_t<T>>
	static T WrapEx(T Value, T Min, T Max);

	/*
	* Wraps an integer betwen [Min,Max].  This version of wrap with proper behavior.  FMath::Wrap has a bug currently.
	* See https://forums.unrealengine.com/t/fmath-wrap-behavior/266612.
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "WrapEx (Integer)", Min = "0", Max = "100"), Category = "Math|Integer")
	static int32 Wrap(int32 Value, int32 Min, int32 Max);

	/*
	* Loads a text file contents as a string with the given name searching first from Content directory if a relative path is specified.
	*/
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "File Utils")
	static bool FileLoadString(const FString& FileName, FString& Contents);

	/*
* Loads a text file contents as a string with the given name searching first from Content directory if a relative path is specified.
*/
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "File Utils")
	static bool FileSaveString(const FString& File, const FString& Contents);
};

#pragma region Template Definitions

template<typename T> requires std::is_arithmetic_v<std::remove_cvref_t<T>>
T UPGBlueprintFunctionLibrary::WrapEx(T Value, T Min, T Max)
{
	if (Min == Max)
	{
		return Min;
	}

	// If integral type + 1 to get the proper range, and if floating point then use size as the values are not discrete
	const auto Size = [&]()
	{
		if constexpr (std::is_integral_v<std::remove_cvref_t<T>>)
		{
			return Max - Min + 1;
		}
		else
		{
			return Max - Max;
		}
	}();

	auto EndVal{ Value };

	while (EndVal < Min)
	{
		EndVal += Size;
	}

	while (EndVal > Max)
	{
		EndVal -= Size;
	}

	return EndVal;
}

#pragma endregion Template Definitions

#pragma region Inline Definitions

inline int32 UPGBlueprintFunctionLibrary::Wrap(int32 Value, int32 Min, int32 Max)
{
	return WrapEx(Value, Min, Max);
}

#pragma endregion Inline Definitions
