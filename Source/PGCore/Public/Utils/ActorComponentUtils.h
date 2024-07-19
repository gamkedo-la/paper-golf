// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Controller.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"

#include "Utils/ObjectUtils.h"

#include <concepts>


namespace PG
{
	class PGCORE_API ActorComponentUtils
	{
	public:
		template<std::derived_from<APawn> TPawn = APawn>
		static TPawn* GetOwnerPawn(const UActorComponent& ActorComponent);

		template<std::derived_from<UInterface> UInterfaceType, std::derived_from<AActor> TActor, typename TInterfaceType, typename ArrayAlloc>
		static void GetInterfaceImplementors(TActor* Actor, TArray<TInterfaceType*, ArrayAlloc>& OutImplementors);

	private:

		// Non-instantiable
		ActorComponentUtils() = delete;

	#if !(NO_LOGGING)
		static void LogFailedResult(const UClass* PawnClass, const UActorComponent& ActorComponent, const AActor* OwnerResult, const AController* Controller);
	#endif
	};
}

#pragma region Template Definitions

template<std::derived_from<APawn> TPawn>
TPawn* PG::ActorComponentUtils::GetOwnerPawn(const UActorComponent& ActorComponent)
{
	auto OwnerResult = ActorComponent.GetOwner();

	auto Pawn = Cast<TPawn>(OwnerResult);
	if (Pawn)
	{
		return Pawn;
	}

	auto Controller = Cast<AController>(OwnerResult);
	if (Controller)
	{
		Pawn = Cast<TPawn>(Controller->GetPawn());
	}

	if (Pawn)
	{
		return Pawn;
	}

#if !(NO_LOGGING)
	LogFailedResult(TPawn::StaticClass(), ActorComponent, OwnerResult, Controller);
#endif

	return nullptr;
}

template<std::derived_from<UInterface> UInterfaceType, std::derived_from<AActor> TActor, typename TInterfaceType, typename ArrayAlloc>
void PG::ActorComponentUtils::GetInterfaceImplementors(TActor* Actor, TArray<TInterfaceType*, ArrayAlloc>& OutImplementors)
{
	if (!Actor)
	{
		return;
	}

	if (auto ActorAsInterface = ObjectUtils::CastToInterface<UInterfaceType, TInterfaceType>(Actor); ActorAsInterface)
	{
		OutImplementors.Add(ActorAsInterface);
	}

	const auto ComponentsAsInterface = Actor->GetComponentsByInterface(UInterfaceType::StaticClass());
	for (UActorComponent* Component : ComponentsAsInterface)
	{
		if (auto ComponentAsInterface = Cast<TInterfaceType>(Component))
		{
			OutImplementors.Add(ComponentAsInterface);
		}
	}
}

#pragma endregion Template Definitions
