// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include <random>
#include <algorithm>
#include <concepts>
#include <iterator>
#include <numeric>

#include "CoreMinimal.h"

namespace RandUtils
{
	PGCORE_API unsigned GenerateSeed();

	template<typename Random, std::forward_iterator Iter>
	void ShuffleIndices(Iter Begin, Random& Rng, std::size_t Count);

	int32 RandSign();
}

#pragma region Template Definitions

namespace RandUtils
{
	template<typename Random, std::forward_iterator Iter>
	void ShuffleIndices(Iter BeginIt, Random& Rng, std::size_t Count)
	{
		const auto EndIt = std::next(BeginIt, Count);

		// 0, 1, 2, 3...
		std::iota(BeginIt, EndIt, 0);
		std::shuffle(BeginIt, EndIt, Rng);
	}
}

#pragma endregion Template Definitions

#pragma region Inline Definitions

FORCEINLINE int32 RandUtils::RandSign()
{
	return FMath::RandBool() ? 1 : -1;
}

#pragma endregion Inline Definitions