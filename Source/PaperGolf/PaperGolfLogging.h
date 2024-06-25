// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if NO_LOGGING
	DECLARE_LOG_CATEGORY_EXTERN(LogPaperGolfGame, NoLogging, NoLogging);
#elif UE_BUILD_SHIPPING
	DECLARE_LOG_CATEGORY_EXTERN(LogPaperGolfGame, Warning, Warning);
#else
	DECLARE_LOG_CATEGORY_EXTERN(LogPaperGolfGame, Display, All);
#endif
