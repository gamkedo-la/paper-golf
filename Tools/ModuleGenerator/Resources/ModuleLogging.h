// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if NO_LOGGING
	DECLARE_LOG_CATEGORY_EXTERN(Log%ModuleName%, NoLogging, NoLogging);
#elif UE_BUILD_SHIPPING
	DECLARE_LOG_CATEGORY_EXTERN(Log%ModuleName%, Warning, Warning);
#else
	DECLARE_LOG_CATEGORY_EXTERN(Log%ModuleName%, Display, All);
#endif
