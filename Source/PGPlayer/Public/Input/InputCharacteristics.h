// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include <atomic>

namespace PG
{
	class PGPLAYER_API FInputCharacteristics
	{
	public:
		static bool IsGamepadAvailable();
		static void SetGamepadAvailable(bool bAvailable);

	private:
		static std::atomic_bool bGamepadAvailable;
	};
}

#pragma region Inline Definitions

FORCEINLINE bool PG::FInputCharacteristics::IsGamepadAvailable()
{
	return bGamepadAvailable.load(std::memory_order::acquire);
}

FORCEINLINE void PG::FInputCharacteristics::SetGamepadAvailable(bool bAvailable)
{
	bGamepadAvailable.store(bAvailable, std::memory_order::release);
}

#pragma endregion Inline Definitions
