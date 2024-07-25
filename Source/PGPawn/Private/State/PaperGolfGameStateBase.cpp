// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "State/PaperGolfGameStateBase.h"
#include "Net/UnrealNetwork.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGPawnLogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfGameStateBase)

void APaperGolfGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APaperGolfGameStateBase, CurrentHoleNumber);
	DOREPLIFETIME(APaperGolfGameStateBase, ActivePlayer);
}

void APaperGolfGameStateBase::OnRep_CurrentHoleNumber()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnRep_CurrentHoleNumber - CurrentHoleNumber=%d"), *GetName(), CurrentHoleNumber);

	OnHoleChanged.Broadcast(CurrentHoleNumber);
}
