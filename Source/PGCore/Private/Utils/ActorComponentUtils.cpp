// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "Utils/ActorComponentUtils.h"

#include "Logging/LoggingUtils.h"
#include "PGCoreLogging.h"
#include "VisualLogger/VisualLogger.h"

#if !(NO_LOGGING)
void PG::ActorComponentUtils::LogFailedResult(const UClass* PawnClass, const UActorComponent& ActorComponent, const AActor* OwnerResult, const AController* Controller)
{
	// see if we are spectating and disable log message if so
	if (UE_LOG_ACTIVE(LogPGCore, Warning))
	{
		if (!Controller)
		{
			auto Pawn = Cast<const APawn>(OwnerResult);
			if (Pawn)
			{
				Controller = Pawn->GetController();
			}
		}

		if (!Controller || !Controller->IsInState(NAME_Spectating))
		{
			if (Controller && !Controller->GetPawn())
			{
				UE_VLOG_UELOG(ActorComponent.GetOwner(), LogPGCore, Log, TEXT("%s: Cannot get %s from Owner=%s;Controller=%s as Pawn is NULL"),
					*ActorComponent.GetName(),
					*LoggingUtils::GetName(PawnClass),
					*LoggingUtils::GetName(OwnerResult),
					*Controller->GetName()
				);
			}
			else
			{
				UE_VLOG_UELOG(ActorComponent.GetOwner(), LogPGCore, Warning, TEXT("%s: Cannot get %s from Owner=%s;Controller=%s;Pawn=%s"),
					*ActorComponent.GetName(),
					*LoggingUtils::GetName(PawnClass),
					*LoggingUtils::GetName(OwnerResult),
					*LoggingUtils::GetName(Controller),
					Controller ? *LoggingUtils::GetName(Controller->GetPawn()) : TEXT("[Controller=NULL]")
				);
			}
		}
	}

}
#endif
