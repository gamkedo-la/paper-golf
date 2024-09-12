// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include <concepts>
#include <type_traits>

#include "Utils/StringUtils.h"

#include "GameFramework/PlayerState.h"

namespace LoggingUtils
{
	template<PG::StringUtils::UEnumConcept T>
	FString GetName(T Value);

	/*
	 * Returns Object->GetName if not NULL and the literal "NULL" otherwise.
	*/
	template<PG::StringUtils::GetNameConcept T>
	FString GetName(const T* Object);

	template<PG::StringUtils::GetNameConcept T>
	FString GetName(const T& Object);

	template<>
	FString GetName<APlayerState>(const APlayerState* Object);

	template<>
	FString GetName<APlayerState>(const APlayerState& Object);

	auto GetBoolString(bool Result);

	template<std::integral T>
	auto Pluralize(T value);
}

#pragma region Inline Definitions

namespace LoggingUtils
{
	template<PG::StringUtils::UEnumConcept T>
	inline FString GetName(T Value)
	{
		return UEnum::GetDisplayValueAsText(Value).ToString();
	}

	template<PG::StringUtils::GetNameConcept T>
	inline FString GetName(const T* Object)
	{
		return (PG::StringUtils::ObjectName<T>{})(Object);
	}

	template<PG::StringUtils::GetNameConcept T>
	inline FString GetName(const T& Object)
	{
		return (PG::StringUtils::ObjectName<T>{})(Object);
	}

	template<PG::StringUtils::ConvertibleToUObject T>
	inline FString GetName(const TScriptInterface<T>& Object)
	{
		return (PG::StringUtils::UObjectInterfaceToString<T>{})(Object);
	}

	template<>
	inline FString GetName(const APlayerState* Object)
	{
		return Object ? GetName(*Object) : TEXT("NULL");
	}

	template<>
	inline FString GetName(const APlayerState& Object)
	{
		const auto& Name = Object.GetPlayerName();
		return !Name.IsEmpty() ? Name : Object.GetName();
	}

	inline auto GetBoolString(bool bResult)
	{
		return bResult ? TEXT("TRUE") : TEXT("FALSE");
	}

	template<std::integral T>
	inline auto Pluralize(T value)
	{
		return value != 1 ? TEXT("s") : TEXT("");
	}
}

#pragma endregion Inline Definitions
