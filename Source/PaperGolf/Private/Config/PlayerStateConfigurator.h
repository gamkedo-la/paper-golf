// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

class UPlayerConfig;
class AGolfPlayerState;

namespace PG
{
	class FPlayerStateConfigurator
	{
	public:
		explicit FPlayerStateConfigurator(UPlayerConfig* PlayerConfig);

		void AssignToPlayer(AGolfPlayerState& GolfPlayerState);


	private:
		struct FAssignedColor
		{
			FLinearColor Color{};
			TWeakObjectPtr<AGolfPlayerState> PlayerState{};
		};

		void AssignColorToPlayer(AGolfPlayerState& GolfPlayerState);
		bool RecycleColors();
		void ResetAvailableColors();

		TArray<FLinearColor> AvailablePlayerColors{};
		TArray<FAssignedColor> AssignedColors{};
	};
}

