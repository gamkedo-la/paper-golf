// Copyright Game Salutes. All Rights Reserved.


#include "State/GolfPlayerState.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfPlayerState)

AGolfPlayerState::AGolfPlayerState()
{
	NetUpdateFrequency = 10.0f;
}

void AGolfPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGolfPlayerState, Shots);
	DOREPLIFETIME(AGolfPlayerState, bReadyForShot);
	DOREPLIFETIME(AGolfPlayerState, ScoreByHole);
	DOREPLIFETIME(AGolfPlayerState, bSpectatorOnly);
}

int32 AGolfPlayerState::GetTotalShots() const
{
	int32 Total{};
	for(auto HoleScore : ScoreByHole)
	{
		Total += HoleScore;
	}

	return Total;
}

void AGolfPlayerState::FinishHole()
{
	ScoreByHole.Add(Shots);
}

void AGolfPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	auto OtherPlayerState = Cast<AGolfPlayerState>(PlayerState);
	if (!IsValid(OtherPlayerState))
	{
		return;
	}

	ScoreByHole = OtherPlayerState->ScoreByHole;
	Shots = OtherPlayerState->Shots;
	bReadyForShot = OtherPlayerState->bReadyForShot;
	bSpectatorOnly = OtherPlayerState->bSpectatorOnly;
}
