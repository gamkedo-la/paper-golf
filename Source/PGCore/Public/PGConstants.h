// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#ifndef PG_DEBUG_ENABLED
	#define PG_DEBUG_ENABLED !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#endif

#ifndef PG_ALLOW_BOT_REPLACE_PLAYER
// TODO: Toggle once implemented
	#define PG_ALLOW_BOT_REPLACE_PLAYER 1
#endif

#ifndef PG_ALLOW_PLAYER_REPLACE_BOT
// TODO: Toggle once implemented
	#define PG_ALLOW_PLAYER_REPLACE_BOT 0
#endif

namespace PG
{
	inline constexpr int32 MaxPlayers = 4;
}
