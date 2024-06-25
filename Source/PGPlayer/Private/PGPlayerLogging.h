// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if NO_LOGGING
	DECLARE_LOG_CATEGORY_EXTERN(LogPGPlayer, NoLogging, NoLogging);
#elif UE_BUILD_SHIPPING
	DECLARE_LOG_CATEGORY_EXTERN(LogPGPlayer, Warning, Warning);
#else
	DECLARE_LOG_CATEGORY_EXTERN(LogPGPlayer, Display, All);
#endif
