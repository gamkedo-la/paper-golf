// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "Utils/RandUtils.h"

#include <random>

unsigned RandUtils::GenerateSeed()
{
	std::random_device rd;

	return rd();
}
