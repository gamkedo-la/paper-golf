// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#ifndef PG_DEBUG_ENABLED
	#define PG_DEBUG_ENABLED !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#endif
