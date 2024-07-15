// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include <compare>
#include <concepts>

#include <optional>

#include "CoreMinimal.h"

namespace PG::StringUtils
{
	template<typename T>
	concept UEnumConcept = std::is_enum_v<T> &&
		requires(T Value)
	{
		{
			UEnum::GetDisplayValueAsText(Value).ToString()
		} -> std::convertible_to<FString>;
	};

	template<typename Func, typename T>
	concept ToStringFunctorConcept = requires(const T t, Func f)
	{
		{
			f(t)
		} -> std::convertible_to<FString>;
	};

	template<typename T>
	concept ToStringConcept = requires(const T t)
	{
		{
			t.ToString()
		} -> std::convertible_to<FString>;
	};

	template<typename T>
	concept GetNameConcept = requires(const T t)
	{
		{
			t.GetName()
		} -> std::convertible_to<FString>;
	} || (requires(const T t)
	{
		{
			t->GetName()
		} -> std::convertible_to<FString>;
		// smart pointers are convertible to bool
	} && (std::convertible_to<T, bool> || (ToStringConcept<T> && requires(const T t)
	{
		// soft references have an "IsNull" function but we have to also check if it's valid to access name; otherwise, use ToString that prints the path
		{
			t.IsNull()
		} -> std::convertible_to<bool>;
		{
			t.IsValid()
		} -> std::convertible_to<bool>;
	}) || (requires(const T t)
	{
		// Weak ptr types
		{
			t.IsValid()
		} -> std::convertible_to<bool>;
	})));

	template<typename T> requires ToStringConcept<T>
	struct ObjectToString
	{
		FString operator()(const T* obj) const
		{
			return obj ? obj->ToString() : TEXT("NULL");
		}

		FString operator()(const T& obj) const
		{
			return obj.ToString();
		}
	};

	template<typename T> requires GetNameConcept<T>
	struct ObjectName
	{
		FString operator()(const T* obj) const
		{
			return obj ? obj->GetName() : TEXT("NULL");
		}

		FString operator()(const T& obj) const
		{
			// Check if the object is a smart pointer or a plain object that supports GetName via the GetNameConcept
			if constexpr (requires { obj.GetName(); })
			{
				return obj.GetName();
			}
			else if constexpr (requires { obj.IsNull(); })
			{
				if (obj.IsNull())
				{
					return  TEXT("NULL");
				}
				if (!obj.IsValid())
				{
					return TEXT("INVALID: ") + obj.ToString();
				}

				return obj->GetName();
			}
			else if constexpr (requires { obj.IsExplicitlyNull(); }) // WeakObjectPtr
			{
				if (obj.IsExplicitlyNull())
				{
					return TEXT("NULL");
				}
				if (!obj.IsValid())
				{
					return TEXT("INVALID");
				}

				return obj->GetName();
			}
			else if constexpr(std::convertible_to<T, bool>)
			{
				return obj ? obj->GetName() : TEXT("NULL");
			}
			else if constexpr (requires { obj.IsValid(); })
			{
				return obj.IsValid() ? obj->GetName() : TEXT("NULL");
			}
			else
			{
				static_assert(false, "Mismatch between GetNameConcept and this type");
			}
		}
	};

	template<typename T>
	concept ConvertibleToUObject = requires(T* t)
	{
		{
			Cast<UObject>(t)
		} -> std::convertible_to<UObject*>;
	};

	template<ConvertibleToUObject T>
	struct UObjectInterfaceToString
	{
		FString operator()(const T* InterfacePtr) const
		{
			auto obj = Cast<const UObject>(InterfacePtr);
			return obj ? obj->GetName() : TEXT("NULL");
		}

		FString operator()(const TScriptInterface<T>& InterfacePtr) const
		{
			auto obj = InterfacePtr.GetObject();
			return obj ? obj->GetName() : TEXT("NULL");
		}
	};

	template<typename T> requires UEnumConcept<T>
	struct EnumToString
	{
		FString operator()(const T& obj) const
		{
			return UEnum::GetDisplayValueAsText(obj).ToString();
		}
	};

	template<typename T>
	concept OptionalToStringConcept = ToStringConcept<T> || UEnumConcept<T>;

	template<ToStringConcept T>
	FString ToString(const T* Value);

	template<ToStringConcept T>
	FString ToString(const TScriptInterface<T>& Value);

	template<OptionalToStringConcept T>
	FString ToString(const std::optional<T>& Value);

	template<OptionalToStringConcept T>
	FString ToString(const TOptional<T>& Value);

	template<ToStringConcept T>
	FString ToString(const T& Value);

	template<UEnumConcept T>
	FString ToString(const T& Value);

	template<typename T> requires OptionalToStringConcept<T>
	struct OptionalToString
	{
		FString operator()(const std::optional<T>& OptionalObject) const
		{
			return OptionalObject ? ToString(*OptionalObject) : TEXT("NULL");
		}

		FString operator()(const TOptional<T>& OptionalObject) const
		{
			return OptionalObject ? ToString(*OptionalObject) : TEXT("NULL");
		}
	};
}

auto operator<=>(const FName& First, const FName& Second);

#pragma region Inline Definitions

inline auto operator<=>(const FName& First, const FName& Second)
{
	const auto Result = First.Compare(Second);

	if (Result == 0)
	{
		return std::strong_ordering::equal;
	}
	if (Result < 0)
	{
		return std::strong_ordering::less;
	}
	
	return std::strong_ordering::greater;
}

namespace PG::StringUtils
{
	template<ToStringConcept T>
	inline FString ToString(const T* Value)
	{
		return ObjectToString<T>{}(Value);
	}

	template<ToStringConcept T>
	inline FString ToString(const TScriptInterface<T>& Value)
	{
		return ObjectToString<T>{}(Value.GetInterface());
	}

	template<ToStringConcept T>
	inline FString ToString(const T& Value)
	{
		return ObjectToString<T>{}(Value);
	}

	template<UEnumConcept T>
	inline FString ToString(const T& Value)
	{
		return EnumToString<T>{}(Value);
	}

	template<OptionalToStringConcept T>
	inline FString ToString(const std::optional<T>& Value)
	{
		return OptionalToString<T>{}(Value);
	}

	template<OptionalToStringConcept T>
	inline FString ToString(const TOptional<T>& Value)
	{
		return OptionalToString<T>{}(Value);
	}
}

#pragma endregion Inline Definitions
