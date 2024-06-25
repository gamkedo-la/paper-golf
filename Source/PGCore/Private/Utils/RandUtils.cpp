// Copyright Game Salutes. All Rights Reserved.

#include "Utils/RandUtils.h"

#include <random>

unsigned RandUtils::GenerateSeed()
{
	std::random_device rd;

	return rd();
}
