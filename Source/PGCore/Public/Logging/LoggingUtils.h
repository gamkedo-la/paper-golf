// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include <concepts>
#include <type_traits>

#include "Utils/StringUtils.h"

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