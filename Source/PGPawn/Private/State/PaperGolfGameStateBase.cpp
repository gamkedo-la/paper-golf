// Copyright Game Salutes. All Rights Reserved.


#include "State/PaperGolfGameStateBase.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfGameStateBase)

void APaperGolfGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APaperGolfGameStateBase, CurrentHole);
}
