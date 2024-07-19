// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#ifndef PG_DEBUG_ENABLED
	#define PG_DEBUG_ENABLED !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#endif
