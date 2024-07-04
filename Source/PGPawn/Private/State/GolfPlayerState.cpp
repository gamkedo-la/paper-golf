// Copyright Game Salutes. All Rights Reserved.


#include "State/GolfPlayerState.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfPlayerState)

void AGolfPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGolfPlayerState, Shots);
	DOREPLIFETIME(AGolfPlayerState, bReadyForShot);
}
