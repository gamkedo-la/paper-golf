// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Utils/ObjectUtils.h"

FString PG::ObjectUtils::GetFullyQualifiedClassName(const UObject* Object)
{
    if (!Object)
    {
        return {};
    }
    const auto Class = Object->GetClass();

    if (!Class)
    {
        return {};
    }

    return Class->GetClassPathName().ToString();
}
