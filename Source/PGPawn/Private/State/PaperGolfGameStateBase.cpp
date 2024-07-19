// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "State/PaperGolfGameStateBase.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfGameStateBase)

void APaperGolfGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APaperGolfGameStateBase, CurrentHoleNumber);
	DOREPLIFETIME(APaperGolfGameStateBase, ActivePlayer);
}
